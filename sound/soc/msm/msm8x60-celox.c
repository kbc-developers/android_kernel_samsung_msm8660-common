/* Copyright (c) 2010-2011, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/init.h>
#include <linux/err.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/time.h>
#include <linux/wait.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/msm_audio.h>
#include <sound/core.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/pcm.h>
#include <sound/tlv.h>
#include <sound/initval.h>
#include <sound/control.h>
#include <sound/q6afe.h>
#include <asm/dma.h>
#include <asm/mach-types.h>
#include <mach/qdsp6v2/audio_dev_ctl.h>
#include <mach/qdsp6v2/q6voice.h>

#define LOOPBACK_ENABLE		0x1
#define LOOPBACK_DISABLE	0x0

#include "msm8x60-celox.h"
#include "msm8x60-pcm-celox.h"

static struct platform_device *msm_audio_snd_device;
struct msm_volume msm_vol_ctl;
EXPORT_SYMBOL(msm_vol_ctl);

#if defined (CONFIG_TARGET_LOCALE_USA)
bool dualmic_enabled = true;
EXPORT_SYMBOL(dualmic_enabled);
#endif

static struct snd_kcontrol_new snd_msm_controls[];

extern int voice_set_loopback_mode(int mode);
#ifdef CONFIG_SEC_EXTRA_VOLUME_SOL
extern int voice_set_extra_volume_mode(int mode);
#endif

char snddev_name[AUDIO_DEV_CTL_MAX_DEV][44];
#define MSM_MAX_VOLUME 0x2000
#define MSM_VOLUME_STEP ((MSM_MAX_VOLUME+17)/100) /* 17 added to avoid
							  more deviation */
static int device_index; /* Count of Device controls */
static int simple_control; /* Count of simple controls*/
static int src_dev;
static int dst_dev;
static int loopback_status;
static u32 opened_dev1 = -1;
static u32 opened_dev2 = -1;
static int last_active_rx_opened_dev = -1;
static int last_active_tx_opened_dev = -1;

static unsigned short copp_device[AFE_MAX_PORTS];

struct route_info {
	unsigned char playback[SESSION_DSP_COUNT][(AFE_MAX_PORTS + 7) / 8];
	unsigned char capture[SESSION_DSP_COUNT][(AFE_MAX_PORTS + 7) / 8];
	struct audio_client *audio_client[SESSION_DSP_COUNT][2];
	unsigned volume[SESSION_DSP_COUNT][2];
	int voice_rx, voice_tx;
	int voice_enable;
};

static struct route_info msm_route;

static void msm_route_init(void)
{
	int i;

	for (i = 0; i < AFE_MAX_PORTS; i++)
		copp_device[i] = DEVICE_IGNORE;

	memset(&msm_route, 0, sizeof(msm_route));
	for (i = 0; i < SESSION_DSP_COUNT; i++) {
		msm_route.volume[i][0] =
		msm_route.volume[i][1] = MSM_MAX_VOLUME;
	}
	msm_route.voice_rx = 0;
	msm_route.voice_tx = 0;
	msm_route.voice_enable = 0;
}

static int msm_route_get_dec(int session_id, int copp_id)
{
	return (msm_route.playback[session_id][copp_id / 8] & (1 << (copp_id & 7)));
}

static int msm_route_get_enc(int session_id, int copp_id)
{
	return (msm_route.capture[session_id][copp_id / 8] & (1 << (copp_id & 7)));
}

static int msm_route_put_dec(int session_id, int copp_id, int set)
{
	if (set)
		msm_route.playback[session_id][copp_id / 8] |= 1 << (copp_id & 7);
	else
		msm_route.playback[session_id][copp_id / 8] &= ~(1 << (copp_id & 7));

	return 0;
}

static int msm_route_put_enc(int session_id, int copp_id, int set)
{
	if (set)
		msm_route.capture[session_id][copp_id / 8] |= 1 << (copp_id & 7);
	else
		msm_route.capture[session_id][copp_id / 8] &= ~(1 << (copp_id & 7));

	return 0;
}

static int msm_set_volume(struct audio_client *audio_client,
					unsigned volume_l, unsigned volume_r)
{
	int rc = 0;

	if (audio_client) {
		if (volume_l != volume_r) {
			pr_debug("%s: call q6asm_set_lrgain\n", __func__);
			rc = q6asm_set_lrgain(audio_client,	volume_l, volume_r);
		} else {
			pr_debug("%s: call q6asm_set_volume\n", __func__);
			rc = q6asm_set_volume(audio_client, volume_l);
		}
		if (rc < 0) {
			pr_err("%s: Send Volume command failed"
					" rc=%d\n", __func__, rc);
		}
	}

	return rc;
}

static int msm_session_update(int dev_id, int set)
{
	struct msm_snddev_info *dev_info;
	int i;

	dev_info = audio_dev_ctrl_find_dev(dev_id);
	if (IS_ERR(dev_info))
		return PTR_ERR(dev_info);

	if (dev_info->capability & SNDDEV_CAP_RX) {
		for (i = 0; i < SESSION_DSP_COUNT; i++) {
			if (msm_route.audio_client[i][0] && msm_route_get_dec(i, dev_info->copp_id)) {
				msm_snddev_set_dec(msm_route.audio_client[i][0]->session,
					dev_info->copp_id, set, dev_info->sample_rate, dev_info->channel_mode);
			}
		}
	} else {
		for (i = 0; i < SESSION_DSP_COUNT; i++) {
			if (msm_route.audio_client[i][1] && msm_route_get_enc(i, dev_info->copp_id)) {
				msm_snddev_set_enc(msm_route.audio_client[i][1]->session,
					dev_info->copp_id, set, dev_info->sample_rate, dev_info->channel_mode);
			}
		}
	}

	return 0;
}

int msm_session_open(int session_id, int stream, struct audio_client *audio_client)
{
	struct msm_snddev_info *dev_info;
	int i;

	if (msm_route.audio_client[session_id][stream])
		return -EFAULT;

	msm_route.audio_client[session_id][stream] = audio_client;

	if (stream == SNDRV_PCM_STREAM_PLAYBACK) {
		msm_set_volume(audio_client,
			msm_route.volume[session_id][0],
			msm_route.volume[session_id][1]);
		for (i = 0; i < AFE_MAX_PORTS; i++) {
			if (msm_route_get_dec(session_id, i)) {
				dev_info = audio_dev_ctrl_find_dev(copp_device[i]);
				if (IS_ERR(dev_info))
					continue;
				msm_snddev_set_dec(audio_client->session, i, 1,
					dev_info->sample_rate, dev_info->channel_mode);
			}
		}
	} else {
		for (i = 0; i < AFE_MAX_PORTS; i++) {
			if (msm_route_get_enc(session_id, i)) {
				dev_info = audio_dev_ctrl_find_dev(copp_device[i]);
				if (IS_ERR(dev_info))
					continue;
				msm_snddev_set_enc(audio_client->session, i, 1,
					dev_info->sample_rate, dev_info->channel_mode);
			}
		}
	}

	return 0;
}

int msm_session_close(int session_id, int stream)
{
	if (!msm_route.audio_client[session_id][stream])
		return -EFAULT;

	msm_clear_session_id(
		msm_route.audio_client[session_id][stream]->session);

	msm_route.audio_client[session_id][stream] = NULL;

	return 0;
}

static int msm_scontrol_count_info(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 1;
	return 0;
}

static int msm_scontrol_count_get(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = simple_control;
	return 0;
}

static int msm_v_call_info(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 2; /* start, session_id */
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = SESSION_ID_BASE + MAX_VOC_SESSIONS;
	return 0;
}

static int msm_v_call_get(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = 0;
	ucontrol->value.integer.value[1] = 0;
	return 0;
}

static int msm_v_call_put(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	int start = ucontrol->value.integer.value[0];
	u32 session_id = ucontrol->value.integer.value[1];

	if ((session_id != 0) &&
		((session_id < SESSION_ID_BASE) ||
		 (session_id >= SESSION_ID_BASE + MAX_VOC_SESSIONS))) {
		pr_err("%s: Invalid session_id 0x%x\n", __func__, session_id);

		return -EINVAL;
	}

	if (start)
		broadcast_event(AUDDEV_EVT_START_VOICE, DEVICE_IGNORE,
							session_id);
	else
		broadcast_event(AUDDEV_EVT_END_VOICE, DEVICE_IGNORE,
							session_id);
	return 0;
}

static int msm_v_mute_info(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 3; /* dir, mute, session_id */
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = SESSION_ID_BASE + MAX_VOC_SESSIONS;
	return 0;
}

static int msm_v_mute_get(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = 0;
	ucontrol->value.integer.value[1] = 0;
	ucontrol->value.integer.value[2] = 0;
	return 0;
}

static int msm_v_mute_put(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	int dir = ucontrol->value.integer.value[0];
	int mute = ucontrol->value.integer.value[1];
	u32 session_id = ucontrol->value.integer.value[2];

	if ((session_id != 0) &&
		((session_id < SESSION_ID_BASE) ||
		 (session_id >= SESSION_ID_BASE + MAX_VOC_SESSIONS))) {
		pr_err("%s: Invalid session_id 0x%x\n", __func__, session_id);

		return -EINVAL;
	}

	return msm_set_voice_mute(dir, mute, session_id);
}

static int msm_v_volume_info(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 3; /* dir, volume, session_id */
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = SESSION_ID_BASE + MAX_VOC_SESSIONS;
	return 0;
}

static int msm_v_volume_get(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = 0;
	ucontrol->value.integer.value[1] = 0;
	ucontrol->value.integer.value[2] = 0;
	return 0;
}

static int msm_v_volume_put(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	int dir = ucontrol->value.integer.value[0];
	int volume = ucontrol->value.integer.value[1];
	u32 session_id = ucontrol->value.integer.value[2];

	if ((session_id != 0) &&
		((session_id < SESSION_ID_BASE) ||
		 (session_id >= SESSION_ID_BASE + MAX_VOC_SESSIONS))) {
		pr_err("%s: Invalid session_id 0x%x\n", __func__, session_id);

		return -EINVAL;
	}

	return msm_set_voice_vol(dir, volume, session_id);
}

static int msm_volume_info(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 2; /* Volume */
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = 16383;
	return 0;
}
static int msm_volume_get(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = 0;
	return 0;
}

static int msm_volume_put(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	int ret = 0;
	int session_id = ucontrol->value.integer.value[0];
	int volume = ucontrol->value.integer.value[1];
	int factor = ucontrol->value.integer.value[2];
	u64 session_mask = 0;

	if (factor > 10000)
		return -EINVAL;

	if ((volume < 0) || (volume/factor > 100))
		return -EINVAL;

	volume = (MSM_VOLUME_STEP * volume);

	/* Convert back to original decimal point by removing the 10-base factor
	* and discard the fractional portion
	*/

	volume = volume/factor;

	if (volume > MSM_MAX_VOLUME)
		volume = MSM_MAX_VOLUME;

	/* Only Decoder volume control supported */
	session_mask = (((u64)0x1) << session_id) << (MAX_BIT_PER_CLIENT * \
				((int)AUDDEV_CLNT_DEC-1));
	msm_vol_ctl.volume = volume;
	pr_debug("%s:session_id %d, volume %d", __func__, session_id, volume);
	broadcast_event(AUDDEV_EVT_STREAM_VOL_CHG, DEVICE_IGNORE,
							session_mask);

	return ret;
}

static int msm_voice_route(int rx_dev_id, int tx_dev_id, int set)
{
	int rc = 0;
	struct msm_snddev_info *rx_dev_info;
	struct msm_snddev_info *tx_dev_info;
	u64 session_mask;

	if (!set)
		return -EPERM;

	/* Rx Device Routing */
	rx_dev_info = audio_dev_ctrl_find_dev(rx_dev_id);

	if (IS_ERR(rx_dev_info)) {
		pr_err("%s:pass invalid dev_id\n", __func__);
		rc = PTR_ERR(rx_dev_info);
		return rc;
	}

	if (!(rx_dev_info->capability & SNDDEV_CAP_RX)) {
		pr_err("%s:First Dev is supposed to be RX\n", __func__);
		return -EFAULT;
	}

	pr_debug("%s:route cfg %d STREAM_VOICE_RX type\n",
		__func__, rx_dev_id);

	msm_set_voc_route(rx_dev_info, AUDIO_ROUTE_STREAM_VOICE_RX,
				rx_dev_id);

	session_mask =	((u64)0x1) << (MAX_BIT_PER_CLIENT * \
				((int)AUDDEV_CLNT_VOC-1));

	broadcast_event(AUDDEV_EVT_DEV_CHG_VOICE, rx_dev_id, session_mask);


	/* Tx Device Routing */
	tx_dev_info = audio_dev_ctrl_find_dev(tx_dev_id);

	if (IS_ERR(tx_dev_info)) {
		pr_err("%s:pass invalid dev_id\n", __func__);
		rc = PTR_ERR(tx_dev_info);
		return rc;
	}

	if (!(tx_dev_info->capability & SNDDEV_CAP_TX)) {
		pr_err("%s:Second Dev is supposed to be Tx\n", __func__);
		return -EFAULT;
	}

	pr_debug("%s:route cfg %d %d type\n",
		__func__, tx_dev_id, AUDIO_ROUTE_STREAM_VOICE_TX);

	msm_set_voc_route(tx_dev_info, AUDIO_ROUTE_STREAM_VOICE_TX,
				tx_dev_id);

	broadcast_event(AUDDEV_EVT_DEV_CHG_VOICE, tx_dev_id, session_mask);

	if (rx_dev_info->opened)
		broadcast_event(AUDDEV_EVT_DEV_RDY, rx_dev_id,	session_mask);

	if (tx_dev_info->opened)
		broadcast_event(AUDDEV_EVT_DEV_RDY, tx_dev_id, session_mask);

	return rc;
}

static int msm_voice_info(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 3; /* Device */

	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = msm_snddev_devcount();
	return 0;
}

static int msm_voice_put(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	return msm_voice_route(
			ucontrol->value.integer.value[0],
			ucontrol->value.integer.value[1],
			ucontrol->value.integer.value[2]);
}

static int msm_voice_get(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = 0;
	/* TODO: query Device list */
	return 0;
}

static int msm_device_enable(int dev_id, int set)
{
	int rc = 0;
	struct msm_snddev_info *dev_info;
	struct msm_snddev_info *dst_dev_info;
	struct msm_snddev_info *src_dev_info;
	int tx_freq = 0;
	int rx_freq = 0;
	u32 set_freq = 0;

	dev_info = audio_dev_ctrl_find_dev(dev_id);
	if (IS_ERR(dev_info)) {
		pr_err("%s:pass invalid dev_id\n", __func__);
		rc = PTR_ERR(dev_info);
		return rc;
	}
	pr_info("%s:device %s set %d\n", __func__, dev_info->name, set);

	if (set) {
			#ifndef CONFIG_USA_MODEL_SGH_T989
		pr_info("Device %s Opened = %d\n", dev_info->name, dev_info->opened);
		#endif
		if (!dev_info->opened) {
#if defined (CONFIG_TARGET_LOCALE_USA)
			if(!strcmp(dev_info->name, "dualmic_handset_ef_tx"))
			{
				pr_info("%s : dualmic_enabled\n",__func__);
				dualmic_enabled = 1;
			}
#endif
			set_freq = dev_info->sample_rate;
			if (!msm_device_is_voice(dev_id)) {
				msm_get_voc_freq(&tx_freq, &rx_freq);
				if (dev_info->capability & SNDDEV_CAP_TX)
					set_freq = tx_freq;

				if (set_freq == 0)
					set_freq = dev_info->sample_rate;
			} else
				set_freq = dev_info->sample_rate;


			pr_err("%s:device freq =%d\n", __func__, set_freq);
			rc = dev_info->dev_ops.set_freq(dev_info, set_freq);
			if (rc < 0) {
				pr_err("%s:device freq failed!\n", __func__);
				return rc;
			}
			dev_info->set_sample_rate = rc;
			rc = 0;
			pr_info("Device trying to open : %s\n", dev_info->name);
			rc = dev_info->dev_ops.open(dev_info);
			if (rc < 0) {
/*[[Safeguard code for device open issue -START //balaji.k
	 This fix would work incase of EBUSY error when device is being opened & previous instance of device is not closed */
				if(rc == -EBUSY) {
					struct msm_snddev_info * last_dev_info = NULL;
					int closing_dev = -1;
					pr_err("DEV_BUSY: Ebusy Error %s : route_cfg.dev_id 1: %d\n", __func__, dev_id);

					//Closing the last active device after sending the broadcast
					if (dev_info->capability & SNDDEV_CAP_TX) {
						last_dev_info = audio_dev_ctrl_find_dev(last_active_tx_opened_dev);
						closing_dev = last_active_tx_opened_dev;
					}
					if (dev_info->capability & SNDDEV_CAP_RX) {
						last_dev_info = audio_dev_ctrl_find_dev(last_active_rx_opened_dev);
						closing_dev = last_active_rx_opened_dev;
					}

					// to fix exception error
					if (IS_ERR(last_dev_info)) {
						pr_err("last_dev:%s:pass invalid dev_id\n", __func__);
						rc = PTR_ERR(last_dev_info);
						return rc;
					}

					broadcast_event(AUDDEV_EVT_REL_PENDING,
						closing_dev,
						SESSION_IGNORE);
					pr_err("DEV_BUSY:closing Last active Open dev (%d)\n", closing_dev);
					rc = dev_info->dev_ops.close(last_dev_info);
					pr_err("DEV_BUSY: %s : route_cfg.dev_id 2: %d\n", __func__, dev_id);

					if (rc < 0) {
						pr_err("DEV_BUSY  : %s:Snd device failed close!\n", __func__);
						return rc;
					} else {
						// Device close is successful, so broadcasting release event.
						// Commented these as here we are closing the previous device
						//if(opened_dev1 == dev_id)
							opened_dev1 = -1;
						//else if(opened_dev2 == dev_id)
							opened_dev2 = -1;
						last_dev_info->opened= 0;
						broadcast_event(AUDDEV_EVT_DEV_RLS,
							closing_dev,
							SESSION_IGNORE);
					}

					dev_info = audio_dev_ctrl_find_dev(dev_id);
					if (IS_ERR(dev_info)) {
						pr_err("DEV_BUSY: %s:pass invalid dev_id\n", __func__);
						rc = PTR_ERR(dev_info);
						return rc;
					}

					pr_err("DEV_BUSY: Opening the Device Now %s : route_cfg.dev_id : %d\n", __func__, dev_id);

					// Opening the intended device
					rc = dev_info->dev_ops.open(dev_info);

					if(rc < 0) {
						pr_err("DEV_BUSY: %s, Device %d:Enabling %s failed\n", __func__, rc, dev_info->name);
						return rc;
					} else {
						// Maintaining the last Opened device- reqd for closing if EBUSY is encountered.
						if (dev_info->capability & SNDDEV_CAP_TX)
							last_active_tx_opened_dev = dev_id;
						else if(dev_info->capability & SNDDEV_CAP_RX)
							last_active_rx_opened_dev =  dev_id;

						printk("Last active Open Txdev (%d) and Rxdev(%d)\n", last_active_tx_opened_dev,  last_active_rx_opened_dev);
					}
				} else {
					pr_err("%s:Enabling %s failed\n",
						__func__, dev_info->name);
					return rc;
				}
		} else {
			// Maintaining the last Opened device- reqd for closing if EBUSY is encountered.
			if (dev_info->capability & SNDDEV_CAP_TX)
				last_active_tx_opened_dev = dev_id;
			else if(dev_info->capability & SNDDEV_CAP_RX)
			 	last_active_rx_opened_dev =  dev_id;
			printk("Last active Open Txdev (%d) and Rxdev(%d)\n", last_active_tx_opened_dev,  last_active_rx_opened_dev);
		}
//Safeguard code for device open issue -END]] //balaji.k
			dev_info->opened = 1;
			broadcast_event(AUDDEV_EVT_DEV_RDY, dev_id,
							SESSION_IGNORE);
			if ((dev_id == src_dev) ||
				(dev_id == dst_dev)) {
				dst_dev_info = audio_dev_ctrl_find_dev(
							dst_dev);
				if (IS_ERR(dst_dev_info)) {
					pr_err("dst_dev:%s:pass invalid"
						"dev_id\n", __func__);
					rc = PTR_ERR(dst_dev_info);
					return rc;
				}
				src_dev_info = audio_dev_ctrl_find_dev(
							src_dev);
				if (IS_ERR(src_dev_info)) {
					pr_err("src_dev:%s:pass invalid"
						"dev_id\n", __func__);
					rc = PTR_ERR(src_dev_info);
					return rc;
				}
				if ((dst_dev_info->opened) &&
					(src_dev_info->opened)) {
					pr_debug("%d: Enable afe_loopback\n",
							__LINE__);
					afe_loopback(LOOPBACK_ENABLE,
						   dst_dev_info->copp_id,
						   src_dev_info->copp_id);
					loopback_status = 1;
				}
			}
		}
	} else {
		if (dev_info->opened) {
#if defined (CONFIG_TARGET_LOCALE_USA)
			if((!strcmp(dev_info->name,"dualmic_handset_ef_tx"))&&(!strcmp(dev_info->name,"handset_call_rx")))
			{
				pr_debug("%s : dualmic_disabled\n",__func__);
				dualmic_enabled = 0;
			}
#endif
			broadcast_event(AUDDEV_EVT_REL_PENDING,
						dev_id,
						SESSION_IGNORE);
			rc = dev_info->dev_ops.close(dev_info);
			if (rc < 0) {
				pr_err("%s:Snd device failed close!\n",
					__func__);
				return rc;
			} else {
				dev_info->opened = 0;
				broadcast_event(AUDDEV_EVT_DEV_RLS,
					dev_id,
					SESSION_IGNORE);
			}
			if (loopback_status == 1) {
				if ((dev_id == src_dev) ||
					(dev_id == dst_dev)) {
					dst_dev_info = audio_dev_ctrl_find_dev(
								dst_dev);
					if (IS_ERR(dst_dev_info)) {
						pr_err("dst_dev:%s:pass invalid"
							"dev_id\n", __func__);
						rc = PTR_ERR(dst_dev_info);
						return rc;
					}
					src_dev_info = audio_dev_ctrl_find_dev(
								src_dev);
					if (IS_ERR(src_dev_info)) {
						pr_err("src_dev:%s:pass invalid"
							"dev_id\n", __func__);
						rc = PTR_ERR(src_dev_info);
						return rc;
					}
					pr_debug("%d: Disable afe_loopback\n",
						__LINE__);
					afe_loopback(LOOPBACK_DISABLE,
						   dst_dev_info->copp_id,
						   src_dev_info->copp_id);
					loopback_status = 0;
				}
			}
		}

	}
	return rc;
}

static int msm_device_info(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 1; /* Device */

	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = 1;
	return 0;
}

static int msm_device_put(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	int dev_id, copp_id, set, rc;
	struct msm_snddev_info *dev_info;

	dev_id = ucontrol->id.numid - device_index;
	dev_info = audio_dev_ctrl_find_dev(dev_id);
	if (IS_ERR(dev_info)) {
		pr_err("%s:pass invalid dev_id\n", __func__);
		return PTR_ERR(dev_info);
	}

	copp_id = dev_info->copp_id;
	set = ucontrol->value.integer.value[0];

	if (set) {
		if (copp_device[copp_id] != dev_id) {
			if (copp_device[copp_id] != DEVICE_IGNORE) {
				msm_session_update(copp_device[copp_id], 0);
				rc = msm_device_enable(copp_device[copp_id], 0);
				if (rc < 0)
					return rc;
				copp_device[copp_id] = DEVICE_IGNORE;
			}
			rc = msm_device_enable(dev_id, 1);
			if (rc < 0)
				return rc;
			copp_device[copp_id] = dev_id;
			msm_session_update(dev_id, 1);
		}
	} else {
		if (copp_device[copp_id] == dev_id) {
			msm_session_update(dev_id, 0);
			rc = msm_device_enable(dev_id, 0);
			if (rc < 0)
				return rc;
			copp_device[copp_id] = DEVICE_IGNORE;
		}
	}

	return 0;
}

static int msm_device_get(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	int dev_id;
	struct msm_snddev_info *dev_info;

	dev_id = ucontrol->id.numid - device_index;
	dev_info = audio_dev_ctrl_find_dev(dev_id);

	if (IS_ERR(dev_info)) {
		pr_err("%s:pass invalid dev_id\n", __func__);
		return PTR_ERR(dev_info);
	}

	ucontrol->value.integer.value[0] = dev_info->opened ? 1 : 0;

	return 0;
}

static int msm_route_info(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 3; /* Device */

	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = msm_snddev_devcount();
	return 0;
}

static int msm_route_get(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = 0;
	/* TODO: query Device list */
	return 0;
}

static int msm_route_put(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	int rc = 0;
	int enc_freq = 0;
	int requested_freq = 0;
	struct msm_audio_route_config route_cfg;
	struct msm_snddev_info *dev_info;
	int session_id = ucontrol->value.integer.value[0];
	int set = ucontrol->value.integer.value[2];
	u64 session_mask = 0;
	route_cfg.dev_id = ucontrol->value.integer.value[1];

	if (ucontrol->id.numid == 2)
		route_cfg.stream_type =	AUDIO_ROUTE_STREAM_PLAYBACK;
	else
		route_cfg.stream_type =	AUDIO_ROUTE_STREAM_REC;

	pr_debug("%s:route cfg %d %d type for popp %d\n",
		__func__, route_cfg.dev_id, route_cfg.stream_type, session_id);
	dev_info = audio_dev_ctrl_find_dev(route_cfg.dev_id);

	if (IS_ERR(dev_info)) {
		pr_err("%s:pass invalid dev_id\n", __func__);
		rc = PTR_ERR(dev_info);
		return rc;
	}
	if (route_cfg.stream_type == AUDIO_ROUTE_STREAM_PLAYBACK) {
		rc = msm_snddev_set_dec(session_id, dev_info->copp_id, set,
				dev_info->sample_rate, dev_info->channel_mode);
		session_mask =
			(((u64)0x1) << session_id) << (MAX_BIT_PER_CLIENT * \
				((int)AUDDEV_CLNT_DEC-1));
		if (!set) {
			if (dev_info->opened)
				broadcast_event(AUDDEV_EVT_DEV_RLS,
							route_cfg.dev_id,
							session_mask);
			dev_info->sessions &= ~(session_mask);
		} else {
			dev_info->sessions = dev_info->sessions | session_mask;
			if (dev_info->opened)
				broadcast_event(AUDDEV_EVT_DEV_RDY,
							route_cfg.dev_id,
							session_mask);
		}
	} else {

		rc = msm_snddev_set_enc(session_id, dev_info->copp_id, set,
				dev_info->sample_rate, dev_info->channel_mode);
		session_mask =
			(((u64)0x1) << session_id) << (MAX_BIT_PER_CLIENT * \
			((int)AUDDEV_CLNT_ENC-1));
		if (!set) {
			if (dev_info->opened)
				broadcast_event(AUDDEV_EVT_DEV_RLS,
							route_cfg.dev_id,
							session_mask);
			dev_info->sessions &= ~(session_mask);
		} else {
			dev_info->sessions = dev_info->sessions | session_mask;
			enc_freq = msm_snddev_get_enc_freq(session_id);
			requested_freq = enc_freq;
			if (enc_freq > 0) {
				rc = msm_snddev_request_freq(&enc_freq,
						session_id,
						SNDDEV_CAP_TX,
						AUDDEV_CLNT_ENC);
				pr_debug("%s:sample rate configured %d\
					sample rate requested %d \n",
					__func__, enc_freq, requested_freq);
				if ((rc <= 0) || (enc_freq != requested_freq)) {
					pr_debug("%s:msm_snddev_withdraw_freq\n",
						__func__);
					rc = msm_snddev_withdraw_freq
						(session_id,
						SNDDEV_CAP_TX, AUDDEV_CLNT_ENC);
					broadcast_event(AUDDEV_EVT_FREQ_CHG,
							route_cfg.dev_id,
							SESSION_IGNORE);
				}
			}
			if (dev_info->opened)
				broadcast_event(AUDDEV_EVT_DEV_RDY,
							route_cfg.dev_id,
							session_mask);
		}
	}

	if (rc < 0) {
		pr_err("%s:device could not be assigned!\n", __func__);
		return -EFAULT;
	}

	return rc;
}

static int msm_device_volume_info(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 2;
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = 100;
	return 0;
}

static int msm_device_volume_get(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	struct msm_snddev_info *dev_info;

	int dev_id = ucontrol->value.integer.value[0];

	dev_info = audio_dev_ctrl_find_dev(dev_id);
	ucontrol->value.integer.value[0] = dev_info->dev_volume;

	return 0;
}

static int msm_device_volume_put(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	int rc = -EPERM;
	struct msm_snddev_info *dev_info;

	int dev_id = ucontrol->value.integer.value[0];
	int volume = ucontrol->value.integer.value[1];

	pr_debug("%s:dev_id = %d, volume = %d\n", __func__, dev_id, volume);

	dev_info = audio_dev_ctrl_find_dev(dev_id);

	if (IS_ERR(dev_info)) {
		rc = PTR_ERR(dev_info);
		pr_err("%s: audio_dev_ctrl_find_dev failed. %ld \n",
			__func__, PTR_ERR(dev_info));
		return rc;
	}

	pr_debug("%s:dev_name = %s dev_id = %d, volume = %d\n",
			__func__, dev_info->name, dev_id, volume);

	if (dev_info->dev_ops.set_device_volume)
		rc = dev_info->dev_ops.set_device_volume(dev_info, volume);
	else {
		pr_info("%s : device %s does not support device volume "
				"control.", __func__, dev_info->name);
		return -EPERM;
	}

	return rc;
}

static int msm_reset_info(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 1;
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = 0;
	return 0;
}

static int msm_reset_get(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = 0;
	return 0;
}

static int msm_reset_put(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	pr_err("%s:Resetting all devices\n", __func__);
	return msm_reset_all_device();
}

static int msm_anc_info(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 1;
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = 1;
	return 0;
}

static int msm_anc_get(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = 0;
	return 0;
}

static int msm_anc_put(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	int rc = -EPERM;
	struct msm_snddev_info *dev_info;

	int dev_id = ucontrol->value.integer.value[0];
	int enable = ucontrol->value.integer.value[1];

	pr_debug("%s: dev_id = %d, enable = %d\n", __func__, dev_id, enable);
	dev_info = audio_dev_ctrl_find_dev(dev_id);

	if (IS_ERR(dev_info)) {
		rc = PTR_ERR(dev_info);
		pr_err("%s: audio_dev_ctrl_find_dev failed. %ld\n",
			__func__, PTR_ERR(dev_info));
		return rc;
	}

	if (dev_info->dev_ops.enable_anc) {
		rc = dev_info->dev_ops.enable_anc(dev_info, enable);
	} else {
		pr_info("%s : device %s does not support anc control.",
				 __func__, dev_info->name);
		return -EPERM;
	}

	return rc;
}

static int msm_loopback_info(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 3;
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max =  msm_snddev_devcount();
	return 0;
}

static int msm_loopback_get(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = 0;
	return 0;
}

static int msm_loopback_put(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	int rc = 0;
	struct msm_snddev_info *src_dev_info = NULL; /* TX device */
	struct msm_snddev_info *dst_dev_info = NULL; /* RX device */
	int dst_dev_id = ucontrol->value.integer.value[0];
	int src_dev_id = ucontrol->value.integer.value[1];
	int set = ucontrol->value.integer.value[2];

	pr_debug("%s: dst=%d :src=%d set=%d\n", __func__,
		   dst_dev_id, src_dev_id, set);

	dst_dev_info = audio_dev_ctrl_find_dev(dst_dev_id);
	if (IS_ERR(dst_dev_info)) {
		pr_err("dst_dev:%s:pass invalid dev_id\n", __func__);
		rc = PTR_ERR(dst_dev_info);
		return rc;
	}
	if (!(dst_dev_info->capability & SNDDEV_CAP_RX)) {
		pr_err("Destination device %d is not RX device\n",
			dst_dev_id);
		return -EFAULT;
	}

	src_dev_info = audio_dev_ctrl_find_dev(src_dev_id);
	if (IS_ERR(src_dev_info)) {
		pr_err("src_dev:%s:pass invalid dev_id\n", __func__);
		rc = PTR_ERR(src_dev_info);
		return rc;
	}
	if (!(src_dev_info->capability & SNDDEV_CAP_TX)) {
		pr_err("Source device %d is not TX device\n", src_dev_id);
		return -EFAULT;
	}

	if (set) {
		pr_debug("%s:%d:Enabling AFE_Loopback\n", __func__, __LINE__);
		src_dev = src_dev_id;
		dst_dev = dst_dev_id;
		loopback_status = 1;
		if ((dst_dev_info->opened) && (src_dev_info->opened))
			rc = afe_loopback(LOOPBACK_ENABLE,
					   dst_dev_info->copp_id,
					   src_dev_info->copp_id);
	} else {
		pr_debug("%s:%d:Disabling AFE_Loopback\n", __func__, __LINE__);
		src_dev = DEVICE_IGNORE;
		dst_dev = DEVICE_IGNORE;
		loopback_status = 0;
		rc = afe_loopback(LOOPBACK_DISABLE,
				   dst_dev_info->copp_id,
				   src_dev_info->copp_id);
	}
	return rc;
}
static int msm_device_mute_info(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 2;
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = msm_snddev_devcount();
	return 0;
}

static int msm_device_mute_get(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	return 0;
}

static int msm_device_mute_put(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	int dev_id = ucontrol->value.integer.value[0];
	int mute = ucontrol->value.integer.value[1];
	struct msm_snddev_info *dev_info;
	int rc = 0;
	u16 gain = 0x2000;

	dev_info = audio_dev_ctrl_find_dev(dev_id);
	if (IS_ERR(dev_info)) {
		rc = PTR_ERR(dev_info);
		pr_err("%s: audio_dev_ctrl_find_dev failed. %ld\n",
			__func__, PTR_ERR(dev_info));
		return rc;
	}
	if (!(dev_info->capability & SNDDEV_CAP_TX)) {
		rc = -EINVAL;
		return rc;
	}
	if (mute)
		gain = 0;

	pr_debug("%s:dev_name = %s dev_id = %d, gain = %hX\n",
			__func__, dev_info->name, dev_id, gain);
	rc = afe_apply_gain(dev_info->copp_id, gain);
	if (rc < 0) {
		pr_err("%s : device %s not able to set device gain "
				"control.", __func__, dev_info->name);
		return rc;
	}
	pr_debug("Muting/Unmuting device id %d(%s)\n", dev_id, dev_info->name);

	return rc;
}

static int msm_voc_session_info(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 1;
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = SESSION_ID_BASE + MAX_VOC_SESSIONS;
	return 0;
}

static int msm_voice_session_get(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] =
					voice_get_session_id("Voice session");
	return 0;
}

static int msm_voip_session_get(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = voice_get_session_id("VoIP session");
	return 0;
}

static struct snd_kcontrol_new snd_dev_controls[AUDIO_DEV_CTL_MAX_DEV];

static int snd_dev_ctl_index(int idx)
{
	struct msm_snddev_info *dev_info;

	dev_info = audio_dev_ctrl_find_dev(idx);
	if (IS_ERR(dev_info)) {
		pr_err("%s:pass invalid dev_id\n", __func__);
		return PTR_ERR(dev_info);
	}
	if (sizeof(dev_info->name) <= 44)
		sprintf(&snddev_name[idx][0] , "%s", dev_info->name);

	snd_dev_controls[idx].iface = SNDRV_CTL_ELEM_IFACE_MIXER;
	snd_dev_controls[idx].access = SNDRV_CTL_ELEM_ACCESS_READWRITE;
	snd_dev_controls[idx].name = &snddev_name[idx][0];
	snd_dev_controls[idx].index = idx;
	snd_dev_controls[idx].info = msm_device_info;
	snd_dev_controls[idx].get = msm_device_get;
	snd_dev_controls[idx].put = msm_device_put;
	snd_dev_controls[idx].private_value = 0;
	return 0;
}

static int msm_loopback_mode_info(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 1;
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = 0;
	return 0;
}

static int msm_loopback_mode_get(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = 0;
	return 0;
}

static int msm_loopback_mode_put(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	int rc = 0;
	int loopback_mode = ucontrol->value.integer.value[0];

	pr_debug("%s:loopback_mode = %d\n", __func__, loopback_mode);
	voice_set_loopback_mode(loopback_mode);

	return rc;
}

#ifdef CONFIG_SEC_EXTRA_VOLUME_SOL
static int msm_extra_volume_mode_info(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 1;
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = 0;

	return 0;
}

static int msm_extra_volume_mode_get(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = 0;

	return 0;
}

static int msm_extra_volume_mode_put(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	int rc = 0;
	int extra_volume_mode = ucontrol->value.integer.value[0];

	pr_debug("%s:extra_volume_mode = %d\n", __func__, extra_volume_mode);
	voice_set_extra_volume_mode(extra_volume_mode);

	return rc;
}
#endif

#define MSM_EXT(xname, fp_info, fp_get, fp_put, addr) \
{ .iface = SNDRV_CTL_ELEM_IFACE_MIXER, \
  .access = SNDRV_CTL_ELEM_ACCESS_READWRITE, \
  .name = xname, \
  .info = fp_info,\
  .get = fp_get, .put = fp_put, \
  .private_value = addr, \
}

/* If new controls are to be added which would be constant across the
 * different targets, please add to the structure
 * snd_msm_controls. Please do not add any controls to the structure
 * snd_msm_secondary_controls defined below unless they are msm8x60
 * specific.
 */

static struct snd_kcontrol_new snd_msm_controls[] = {
	MSM_EXT("Count", msm_scontrol_count_info, msm_scontrol_count_get, \
						NULL, 0),
	MSM_EXT("Stream", msm_route_info, msm_route_get, \
						 msm_route_put, 0),
	MSM_EXT("Record", msm_route_info, msm_route_get, \
						 msm_route_put, 0),
	MSM_EXT("Voice", msm_voice_info, msm_voice_get, \
						 msm_voice_put, 0),
	MSM_EXT("Volume", msm_volume_info, msm_volume_get, \
						 msm_volume_put, 0),
	MSM_EXT("VoiceVolume", msm_v_volume_info, msm_v_volume_get, \
						 msm_v_volume_put, 0),
	MSM_EXT("VoiceMute", msm_v_mute_info, msm_v_mute_get, \
						 msm_v_mute_put, 0),
	MSM_EXT("Voice Call", msm_v_call_info, msm_v_call_get, \
						msm_v_call_put, 0),
	MSM_EXT("Device_Volume", msm_device_volume_info,
			msm_device_volume_get, msm_device_volume_put, 0),
	MSM_EXT("Reset", msm_reset_info,
			msm_reset_get, msm_reset_put, 0),
	MSM_EXT("ANC", msm_anc_info, msm_anc_get, msm_anc_put, 0),
	MSM_EXT("Device_Mute", msm_device_mute_info,
			msm_device_mute_get, msm_device_mute_put, 0),
};

static struct snd_kcontrol_new snd_msm_secondary_controls[] = {
	MSM_EXT("Sound Device Loopback", msm_loopback_info,
			msm_loopback_get, msm_loopback_put, 0),
	MSM_EXT("VoiceVolume Ext",
			  msm_v_volume_info, msm_v_volume_get, msm_v_volume_put, 0),
	MSM_EXT("VoiceMute Ext",
			msm_v_mute_info, msm_v_mute_get, msm_v_mute_put, 0),
	MSM_EXT("Voice Call Ext",
			msm_v_call_info, msm_v_call_get, msm_v_call_put, 0),
	MSM_EXT("Voice session",
			msm_voc_session_info, msm_voice_session_get, NULL, 0),
	MSM_EXT("VoIP session",
			msm_voc_session_info, msm_voip_session_get, NULL, 0),
	MSM_EXT("Loopback Mode",
		msm_loopback_mode_info, msm_loopback_mode_get, msm_loopback_mode_put, 0),
#ifdef CONFIG_SEC_EXTRA_VOLUME_SOL
	MSM_EXT("Extra Volume Mode",
		msm_extra_volume_mode_info, msm_extra_volume_mode_get, msm_extra_volume_mode_put, 0),
#endif
};

static int msm_s_volume_info(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 2;
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = MSM_MAX_VOLUME;
	return 0;
}

static int msm_s_volume_get(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	int session_id = kcontrol->private_value;

	ucontrol->value.integer.value[0] =
		msm_route.volume[session_id][0];
	ucontrol->value.integer.value[1] =
		msm_route.volume[session_id][1];
	return 0;
}

static int msm_s_volume_put(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	int session_id = kcontrol->private_value;
	int rc = 0;

	if ((msm_route.volume[session_id][0] !=
			ucontrol->value.integer.value[0]) ||
		(msm_route.volume[session_id][1] !=
			ucontrol->value.integer.value[1])) {
		if (msm_route.audio_client[session_id][SNDRV_PCM_STREAM_PLAYBACK]) {
			rc = msm_set_volume(
				msm_route.audio_client[session_id][SNDRV_PCM_STREAM_PLAYBACK],
				ucontrol->value.integer.value[0],
				ucontrol->value.integer.value[1]);
		}
		msm_route.volume[session_id][0] =
			ucontrol->value.integer.value[0];
		msm_route.volume[session_id][1] =
			ucontrol->value.integer.value[1];
	}

	return rc;
}

#define ROUTE_ELEM_ENCODE(session_id, copp_id) ((session_id) << 16 | (copp_id))
#define ROUTE_ELEM_DECODE_SESSION_ID(value) (((value) >> 16) & 0xffff)
#define ROUTE_ELEM_DECODE_COPP_ID(value) ((value) & 0xffff)

static int msm_s_route_info(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 1;
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = 1;
	return 0;
}

static int msm_s_route_get_rx(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = msm_route_get_dec(
		ROUTE_ELEM_DECODE_SESSION_ID(kcontrol->private_value),
		ROUTE_ELEM_DECODE_COPP_ID(kcontrol->private_value));
	return 0;
}

static int msm_s_route_get_tx(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = msm_route_get_enc(
		ROUTE_ELEM_DECODE_SESSION_ID(kcontrol->private_value),
		ROUTE_ELEM_DECODE_COPP_ID(kcontrol->private_value));
	return 0;
}

static int msm_s_route_put_rx(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	return msm_route_put_dec(
				ROUTE_ELEM_DECODE_SESSION_ID(kcontrol->private_value),
				ROUTE_ELEM_DECODE_COPP_ID(kcontrol->private_value),
				ucontrol->value.integer.value[0]);
}

static int msm_s_route_put_tx(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	return msm_route_put_enc(
				ROUTE_ELEM_DECODE_SESSION_ID(kcontrol->private_value),
				ROUTE_ELEM_DECODE_COPP_ID(kcontrol->private_value),
				ucontrol->value.integer.value[0]);
}

static int msm_voice_get_rx(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = msm_route.voice_rx;
	return 0;
}

static int msm_voice_get_tx(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = msm_route.voice_tx;
	return 0;
}

static int msm_voice_put_rx(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	msm_route.voice_rx = ucontrol->value.integer.value[0];
	return 0;
}

static int msm_voice_put_tx(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	msm_route.voice_tx = ucontrol->value.integer.value[0];
	return 0;
}

static int msm_voice_rxtx_info(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 1; /* Device */

	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = msm_snddev_devcount();
	return 0;
}

static int msm_voice_route_get(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = msm_route.voice_enable;
	return 0;
}

static int msm_voice_route_put(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	int enable = !!ucontrol->value.integer.value[0];

	if (msm_route.voice_enable != enable) {
		if (enable) {
			int rc = msm_voice_route(msm_route.voice_rx,
									 msm_route.voice_tx, 1);
			if (rc < 0)
				return rc;
		}
		msm_route.voice_enable = enable;
	}
	return 0;
}

static struct snd_kcontrol_new snd_msm_extend_controls[] = {
	MSM_EXT("DSP Audio 0 Playback Volume",
			msm_s_volume_info, msm_s_volume_get, msm_s_volume_put,
			SESSION_DSP_AUDIO_0),
	MSM_EXT("DSP Audio 1 Playback Volume",
			msm_s_volume_info, msm_s_volume_get, msm_s_volume_put,
			SESSION_DSP_AUDIO_1),
	MSM_EXT("DSP Audio 2 Playback Volume",
			msm_s_volume_info, msm_s_volume_get, msm_s_volume_put,
			SESSION_DSP_AUDIO_2),
	MSM_EXT("deep-buffer-playback",
			msm_s_route_info, msm_s_route_get_rx, msm_s_route_put_rx,
			ROUTE_ELEM_ENCODE(SESSION_DSP_AUDIO_0, IDX_PRIMARY_I2S_RX)),
	MSM_EXT("deep-buffer-playback bt-sco",
			msm_s_route_info, msm_s_route_get_rx, msm_s_route_put_rx,
			ROUTE_ELEM_ENCODE(SESSION_DSP_AUDIO_0, IDX_PCM_RX)),
	MSM_EXT("deep-buffer-playback hdmi",
			msm_s_route_info, msm_s_route_get_rx, msm_s_route_put_rx,
			ROUTE_ELEM_ENCODE(SESSION_DSP_AUDIO_0, IDX_HDMI_RX)),
	MSM_EXT("low-latency-playback",
			msm_s_route_info, msm_s_route_get_rx, msm_s_route_put_rx,
			ROUTE_ELEM_ENCODE(SESSION_DSP_AUDIO_1, IDX_PRIMARY_I2S_RX)),
	MSM_EXT("low-latency-playback bt-sco",
			msm_s_route_info, msm_s_route_get_rx, msm_s_route_put_rx,
			ROUTE_ELEM_ENCODE(SESSION_DSP_AUDIO_1, IDX_PCM_RX)),
	MSM_EXT("low-latency-playback hdmi",
			msm_s_route_info, msm_s_route_get_rx, msm_s_route_put_rx,
			ROUTE_ELEM_ENCODE(SESSION_DSP_AUDIO_1, IDX_HDMI_RX)),
	MSM_EXT("multi-channel-playback hdmi",
			msm_s_route_info, msm_s_route_get_rx, msm_s_route_put_rx,
			ROUTE_ELEM_ENCODE(SESSION_DSP_AUDIO_2, IDX_HDMI_RX)),
	MSM_EXT("audio-record",
			msm_s_route_info, msm_s_route_get_tx, msm_s_route_put_tx,
			ROUTE_ELEM_ENCODE(SESSION_DSP_AUDIO_0, IDX_PRIMARY_I2S_TX)),
	MSM_EXT("audio-record bt-sco",
			msm_s_route_info, msm_s_route_get_tx, msm_s_route_put_tx,
			ROUTE_ELEM_ENCODE(SESSION_DSP_AUDIO_0, IDX_PCM_TX)),
	MSM_EXT("low-latency-record",
			msm_s_route_info, msm_s_route_get_tx, msm_s_route_put_tx,
			ROUTE_ELEM_ENCODE(SESSION_DSP_AUDIO_1, IDX_PRIMARY_I2S_TX)),
	MSM_EXT("low-latency-record bt-sco",
			msm_s_route_info, msm_s_route_get_tx, msm_s_route_put_tx,
			ROUTE_ELEM_ENCODE(SESSION_DSP_AUDIO_1, IDX_PCM_TX)),
	MSM_EXT("voice-call",
			msm_s_route_info, msm_voice_route_get, msm_voice_route_put, 0),
	MSM_EXT("voice-rx",
			msm_voice_rxtx_info, msm_voice_get_rx, msm_voice_put_rx, 0),
	MSM_EXT("voice-tx",
			msm_voice_rxtx_info, msm_voice_get_tx, msm_voice_put_tx, 0),
};

static int msm_new_mixer(struct snd_soc_codec *codec)
{
	unsigned int idx;
	int err;
	int dev_cnt;

	msm_route_init();

	strcpy(codec->card->snd_card->mixername, "MSM Mixer");
	for (idx = 0; idx < ARRAY_SIZE(snd_msm_controls); idx++) {
		err = snd_ctl_add(codec->card->snd_card,
				snd_ctl_new1(&snd_msm_controls[idx],
					NULL));
		if (err < 0)
			pr_err("%s:ERR adding ctl\n", __func__);
	}

	for (idx = 0; idx < ARRAY_SIZE(snd_msm_secondary_controls); idx++) {
		err = snd_ctl_add(codec->card->snd_card,
			snd_ctl_new1(&snd_msm_secondary_controls[idx],
			NULL));
		if (err < 0)
			pr_err("%s:ERR adding secondary ctl\n", __func__);
	}
	dev_cnt = msm_snddev_devcount();

	for (idx = 0; idx < dev_cnt; idx++) {
		if (!snd_dev_ctl_index(idx)) {
			err = snd_ctl_add(codec->card->snd_card,
				snd_ctl_new1(&snd_dev_controls[idx],
					NULL));
			if (err < 0)
				pr_err("%s:ERR adding ctl\n", __func__);
		} else
			return 0;
	}
	simple_control = ARRAY_SIZE(snd_msm_controls)
			+ ARRAY_SIZE(snd_msm_secondary_controls);
	device_index = simple_control + 1;

	for (idx = 0; idx < ARRAY_SIZE(snd_msm_extend_controls); idx++) {
		err = snd_ctl_add(codec->card->snd_card,
			snd_ctl_new1(&snd_msm_extend_controls[idx],
			NULL));
		if (err < 0)
			pr_err("%s:ERR adding extended ctl\n", __func__);
	}

	return 0;
}

static int msm_soc_dai_init(
	struct snd_soc_pcm_runtime *rtd)
{

	int ret = 0;
	struct snd_soc_codec *codec = rtd->codec;

	ret = msm_new_mixer(codec);
	if (ret < 0)
		pr_err("%s: ALSA MSM Mixer Fail\n", __func__);

	return ret;
}

static struct snd_soc_dai_link msm_dai[] = {
{
	.name = "MSM DSP Audio 0",
	.stream_name = "DSP Audio 0",
	.cpu_dai_name = "msm-cpu-dai.0",
	.platform_name = "msm-dsp-audio.0",
	.codec_name = "msm-codec-dai.0",
	.codec_dai_name = "msm-codec-dai",
	.init   = &msm_soc_dai_init,
},
{
	.name = "MSM DSP Audio 1",
	.stream_name = "DSP Audio 1",
	.cpu_dai_name = "msm-cpu-dai.1",
	.platform_name = "msm-dsp-audio.1",
	.codec_name = "msm-codec-dai.1",
	.codec_dai_name = "msm-codec-dai",
},
{
	.name = "MSM DSP Audio 2",
	.stream_name = "DSP Audio 2",
	.cpu_dai_name = "msm-cpu-dai.2",
	.platform_name = "msm-dsp-audio.2",
	.codec_name = "msm-codec-dai.2",
	.codec_dai_name = "msm-codec-dai",
},
{
	.name = "MSM Voice Call",
	.stream_name = "Voice Call",
	.cpu_dai_name = "msm-cpu-dai.3",
	.platform_name = "msm-pcm-voice",
	.codec_name = "msm-codec-dai.3",
	.codec_dai_name = "msm-codec-dai",
},
#ifdef CONFIG_MSM_8x60_VOIP
{
	.name = "MSM Primary Voip",
	.stream_name = "MVS",
	.cpu_dai_name = "mvs-cpu-dai.0",
	.platform_name = "msm-mvs-audio.0",
	.codec_name = "mvs-codec-dai.0",
	.codec_dai_name = "mvs-codec-dai",
},
#endif
};

static struct snd_soc_card snd_soc_card_msm = {
	.name		= "msm-audio",
	.dai_link	= msm_dai,
	.num_links = ARRAY_SIZE(msm_dai),
};

static int __init msm_audio_init(void)
{
	int ret;

	msm_audio_snd_device = platform_device_alloc("soc-audio", -1);
	if (!msm_audio_snd_device)
		return -ENOMEM;

	platform_set_drvdata(msm_audio_snd_device, &snd_soc_card_msm);
	ret = platform_device_add(msm_audio_snd_device);
	if (ret) {
		platform_device_put(msm_audio_snd_device);
		return ret;
	}

	src_dev = DEVICE_IGNORE;
	dst_dev = DEVICE_IGNORE;

	return ret;
}

static void __exit msm_audio_exit(void)
{
	platform_device_unregister(msm_audio_snd_device);
}

module_init(msm_audio_init);
module_exit(msm_audio_exit);

MODULE_DESCRIPTION("PCM module");
MODULE_LICENSE("GPL v2");
