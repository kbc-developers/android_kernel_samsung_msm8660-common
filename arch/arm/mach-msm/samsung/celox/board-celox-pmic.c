/* Copyright (c) 2011, The Linux Foundation. All rights reserved.
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
#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <linux/bootmem.h>
#include <linux/mfd/pmic8058.h>
#include <linux/pmic8058-othc.h>
#include <linux/mfd/pmic8901.h>
#include <mach/board-msm8660.h>
#include <linux/leds.h>
#include <linux/leds-pm8xxx.h>
#include <linux/msm_adc.h>
#include <linux/m_adcproc.h>
#include <asm/mach-types.h>
#include <asm/mach/mmc.h>
#include <asm/setup.h>
#include <mach/socinfo.h>
#include <mach/msm_bus_board.h>
#include <mach/board.h>
#include <mach/gpio.h>
#include <mach/mpp.h>
#include <mach/gpiomux.h>
#include <mach/restart.h>
#include "devices.h"
#include "board-msm8x60.h"

struct pm8xxx_mpp_init_info {
	unsigned			mpp;
	struct pm8xxx_mpp_config_data	config;
};

#define PM8XXX_GPIO_INIT(_gpio, _dir, _buf, _val, _pull, _vin, _out_strength, \
			_func, _inv, _disable) \
{ \
	.gpio	= _gpio, \
	.config	= { \
		.direction	= _dir, \
		.output_buffer	= _buf, \
		.output_value	= _val, \
		.pull		= _pull, \
		.vin_sel	= _vin, \
		.out_strength	= _out_strength, \
		.function	= _func, \
		.inv_int_pol	= _inv, \
		.disable_pin	= _disable, \
	} \
}

#define PM8058_MPP_INIT(_mpp, _type, _level, _control) \
{ \
	.mpp	= PM8058_MPP_PM_TO_SYS(_mpp), \
	.config	= { \
		.type		= PM8XXX_MPP_TYPE_##_type, \
		.level		= _level, \
		.control	= PM8XXX_MPP_##_control, \
	} \
}

#define PM8901_MPP_INIT(_mpp, _type, _level, _control) \
{ \
	.mpp	= PM8901_MPP_PM_TO_SYS(_mpp), \
	.config	= { \
		.type		= PM8XXX_MPP_TYPE_##_type, \
		.level		= _level, \
		.control	= PM8XXX_MPP_##_control, \
	} \
}

#define PM8XXX_MPP_INIT(_mpp, _type, _level, _control) \
{ \
	.mpp    = PM8058_MPP_PM_TO_SYS(_mpp), \
	.config = { \
		.type           = PM8XXX_MPP_TYPE_##_type, \
		.level          = _level, \
		.control        = PM8XXX_MPP_##_control, \
	} \
}

#if defined(CONFIG_VIDEO_MHL_V1) || defined(CONFIG_VIDEO_MHL_V2)
/*MHL pin configuration*/
	struct pm_gpio mhl_sel = {
		.direction		= PM_GPIO_DIR_OUT,
		.pull			= PM_GPIO_PULL_NO,
		.vin_sel		= 4,
		.function		= PM_GPIO_FUNC_NORMAL,
		.inv_int_pol	= 0,
	};
	struct pm_gpio mhl_wake_up = {
		.direction		= PM_GPIO_DIR_OUT,
		.pull			= PM_GPIO_PULL_DN,
		.vin_sel		= 2,
		.function		= PM_GPIO_FUNC_NORMAL,
		.inv_int_pol	= 0,
	};
	struct pm_gpio mhl_int = {
		.direction		= PM_GPIO_DIR_IN,
		.pull			= PM_GPIO_PULL_NO,
		.vin_sel		= 2,
		.function		= PM_GPIO_FUNC_NORMAL,
		.inv_int_pol	= 0,
	};
	struct pm_gpio mhl_rst = {
		.direction		= PM_GPIO_DIR_OUT,
		.pull			= PM_GPIO_PULL_NO,
		.vin_sel		= 4,
		.function		= PM_GPIO_FUNC_NORMAL,
		.inv_int_pol	= 0,
	};
#endif

#ifdef CONFIG_PMIC8058
#define PMIC_GPIO_SDC3_DET 22
#define PMIC_GPIO_TOUCH_DISC_INTR 5

#ifdef CONFIG_OPTICAL_GP2A
static struct pm_gpio ps_vout = {
	.direction 	 = PM_GPIO_DIR_IN,
	.pull			 = PM_GPIO_PULL_NO,
	.vin_sel		 = 2,
	.function		 = PM_GPIO_FUNC_NORMAL,
	.inv_int_pol	 = 0,
	};
#endif
#ifdef CONFIG_SENSORS_AK8975
static struct pm_gpio msense_nRST = {
	.direction      = PM_GPIO_DIR_IN,
	.pull           = PM_GPIO_PULL_NO,
	.vin_sel        = 2,
	.function       = PM_GPIO_FUNC_NORMAL,
	.inv_int_pol    = 0,
};
#endif
#ifdef CONFIG_GYRO_K3G
static struct pm_gpio gyro_int = {
	.direction      = PM_GPIO_DIR_IN,
	.pull           = PM_GPIO_PULL_DN,
	.vin_sel        = 2,
	.function       = PM_GPIO_FUNC_NORMAL,
	.inv_int_pol    = 0,
};
#endif
#ifdef CONFIG_SENSORS_AK8975
static struct pm_gpio accel_int = {
	.direction      = PM_GPIO_DIR_IN,
	.pull           = PM_GPIO_PULL_DN,
	.vin_sel        = 2,
	.function       = PM_GPIO_FUNC_NORMAL,
	.inv_int_pol    = 0,
};
#endif
#if defined(CONFIG_OPTICAL_TAOS)
static struct pm_gpio als_int = {		
	.direction	 = PM_GPIO_DIR_IN,		
	.pull			 = PM_GPIO_PULL_NO,		
	.vin_sel		 = 2,		
	.function		 = PM_GPIO_FUNC_NORMAL,		
	.inv_int_pol	 = 0,	
};
#endif

#if defined (CONFIG_SAMSUNG_JACK) || defined (CONFIG_SAMSUNG_EARJACK)
#if defined (CONFIG_KOR_MODEL_SHV_E110S)
static struct pm_gpio ear_det_new = {
		.direction      = PM_GPIO_DIR_IN,
		.pull           = PM_GPIO_PULL_NO,
		.vin_sel        = PM8058_GPIO_VIN_L5,//PM_GPIO_VIN_L5,//1.8v -> 2.85v  : rev >= 05
		.function       = PM_GPIO_FUNC_NORMAL,
		.inv_int_pol    = 0,
};
#else
static struct pm_gpio ear_det = {
	.direction      = PM_GPIO_DIR_IN,
	.pull           = PM_GPIO_PULL_NO,
	.vin_sel        = 2,
	.function       = PM_GPIO_FUNC_NORMAL,
	.inv_int_pol    = 0,
};
#endif
static struct pm_gpio short_sendend = {
	.direction      = PM_GPIO_DIR_IN,
	.pull           = PM_GPIO_PULL_NO,
	.vin_sel        = 2,
	.function       = PM_GPIO_FUNC_NORMAL,
	.inv_int_pol    = 0,
};

static struct pm_gpio ear_micbiase = {
	.direction      = PM_GPIO_DIR_OUT,
	.pull           = PM_GPIO_PULL_NO,
	.out_strength   = PM_GPIO_STRENGTH_HIGH,
	.function       = PM_GPIO_FUNC_NORMAL,
	.inv_int_pol    = 0,
	.vin_sel        = 2,
	.output_buffer  = PM_GPIO_OUT_BUF_CMOS,
	.output_value   = 0,
};
#endif

#if defined(CONFIG_CHARGER_SMB328A) || defined(CONFIG_CHARGER_SMB136)
/* not used, set intput */
static struct pm_gpio chg_en = {
	.direction      = PM_GPIO_DIR_IN,
	.pull           = PM_GPIO_PULL_NO,
	.vin_sel        = 2,
	.function       = PM_GPIO_FUNC_NORMAL,
	.inv_int_pol    = 0,
};

/* not used */
static struct pm_gpio chg_stat = {
	.direction      = PM_GPIO_DIR_IN,
	.pull           = PM_GPIO_PULL_NO,
	.vin_sel        = 2,
	.function       = PM_GPIO_FUNC_NORMAL,
	.inv_int_pol    = 0,
};
#endif

int msm8x60_gpio_mpp_init(void)
{
	int i;
	int rc;
	struct pm8058_gpio_cfg {
		int                gpio;
		struct pm_gpio	   cfg;
	};

	struct pm8058_gpio_cfg gpio_cfgs[] = {
		{ /* FFA ethernet */
			PM8058_GPIO_PM_TO_SYS(6),
			{
				.direction      = PM_GPIO_DIR_IN,
				.pull           = PM_GPIO_PULL_DN,
				.vin_sel        = 2,
				.function       = PM_GPIO_FUNC_NORMAL,
				.inv_int_pol    = 0,
			},
		},
		{
			PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_SDC3_DET - 1),
			{
				.direction      = PM_GPIO_DIR_IN,
				.pull           = PM_GPIO_PULL_UP_30,
				.vin_sel        = 2,
				.function       = PM_GPIO_FUNC_NORMAL,
				.inv_int_pol    = 0,
			},
		},
		{ /* core&surf gpio expander */
			PM8058_GPIO_PM_TO_SYS(UI_INT1_N),
			{
				.direction      = PM_GPIO_DIR_IN,
				.pull           = PM_GPIO_PULL_NO,
				.vin_sel        = PM8058_GPIO_VIN_S3,
				.function       = PM_GPIO_FUNC_NORMAL,
				.inv_int_pol    = 0,
			},
		},
		{ /* docking gpio expander */
			PM8058_GPIO_PM_TO_SYS(UI_INT2_N),
			{
				.direction      = PM_GPIO_DIR_IN,
				.pull           = PM_GPIO_PULL_NO,
				.vin_sel        = PM8058_GPIO_VIN_S3,
				.function       = PM_GPIO_FUNC_NORMAL,
				.inv_int_pol    = 0,
			},
		},
		{ /* FHA/keypad gpio expanders */
			PM8058_GPIO_PM_TO_SYS(UI_INT3_N),
			{
				.direction      = PM_GPIO_DIR_IN,
				.pull           = PM_GPIO_PULL_NO,
				.vin_sel        = PM8058_GPIO_VIN_S3,
				.function       = PM_GPIO_FUNC_NORMAL,
				.inv_int_pol    = 0,
			},
		},
		{ /* Timpani Reset */
			PM8058_GPIO_PM_TO_SYS(20),
			{
				.direction	= PM_GPIO_DIR_OUT,
				.output_value	= 1,
				.output_buffer	= PM_GPIO_OUT_BUF_CMOS,
				.pull		= PM_GPIO_PULL_DN,
				.out_strength	= PM_GPIO_STRENGTH_HIGH,
				.function	= PM_GPIO_FUNC_NORMAL,
				.vin_sel	= 2,
				.inv_int_pol	= 0,
			}
		},
		{ /* PMIC ID interrupt */
			PM8058_GPIO_PM_TO_SYS(36),
			{
				.direction	= PM_GPIO_DIR_IN,
				.pull		= PM_GPIO_PULL_NO,
				.function	= PM_GPIO_FUNC_NORMAL,
				.vin_sel	= 2,
				.inv_int_pol	= 0,
			}
		},
#if defined(CONFIG_PN544_NFC)
		{
			PM8058_GPIO_PM_TO_SYS(7),
			{
				.direction	= PM_GPIO_DIR_IN,
				.pull			= PM_GPIO_PULL_DN,
				.vin_sel		= 2,
				.function		= PM_GPIO_FUNC_NORMAL,
				.inv_int_pol	= 0,
			}
		},
		{
			PM8058_GPIO_PM_TO_SYS(29),
			{
				.direction	= PM_GPIO_DIR_OUT,
				.pull			= PM_GPIO_PULL_NO,
				.vin_sel		= 2,
				.function		= PM_GPIO_FUNC_NORMAL,
				.inv_int_pol	= 0,
			}
		},
#endif	
		
	};

#if defined(CONFIG_TOUCHDISC_VTD518_SHINETSU) || \
		defined(CONFIG_TOUCHDISC_VTD518_SHINETSU_MODULE)
	struct pm_gpio touchdisc_intr_gpio_cfg = {
		.direction      = PM_GPIO_DIR_IN,
		.pull           = PM_GPIO_PULL_UP_1P5,
		.vin_sel        = 2,
		.function       = PM_GPIO_FUNC_NORMAL,
	};
#endif

#if defined(CONFIG_HAPTIC_ISA1200) || \
			defined(CONFIG_HAPTIC_ISA1200_MODULE)
	struct pm_gpio en_hap_gpio_cfg = {
		.direction      = PM_GPIO_DIR_OUT,
		.pull           = PM_GPIO_PULL_NO,
		.out_strength   = PM_GPIO_STRENGTH_HIGH,
		.function       = PM_GPIO_FUNC_NORMAL,
		.inv_int_pol    = 0,
		.vin_sel        = 2,
		.output_buffer  = PM_GPIO_OUT_BUF_CMOS,
		.output_value   = 0,
	};
#endif

#if defined(CONFIG_PMIC8058_OTHC) || defined(CONFIG_PMIC8058_OTHC_MODULE)
	struct pm8058_gpio_cfg line_in_gpio_cfg = {
			PM8058_GPIO_PM_TO_SYS(18),
			{
				.direction	= PM_GPIO_DIR_IN,
				.pull           = PM_GPIO_PULL_UP_1P5,
				.vin_sel        = 2,
				.function       = PM_GPIO_FUNC_NORMAL,
				.inv_int_pol    = 0,
			}
	};
#endif

#if defined(CONFIG_QS_S5K4E1)
		{
			struct pm8058_gpio_cfg qs_hc37_cam_pd_gpio_cfg = {
			PM8058_GPIO_PM_TO_SYS(26),
			{
				.direction		= PM_GPIO_DIR_OUT,
				.output_value	= 0,
				.output_buffer	= PM_GPIO_OUT_BUF_CMOS,
				.pull			= PM_GPIO_PULL_DN,
				.out_strength	= PM_GPIO_STRENGTH_HIGH,
				.function		= PM_GPIO_FUNC_NORMAL,
				.vin_sel		= 2,
				.inv_int_pol	= 0,
			}
		};
#endif

#if defined (CONFIG_SAMSUNG_JACK) || defined (CONFIG_SAMSUNG_EARJACK)
#if defined (CONFIG_KOR_MODEL_SHV_E110S)
		pr_info("%s PMIC_GPIO_EAR_DET : ear_det_new\n", __func__);
		rc = pm8xxx_gpio_config(PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_EAR_DET), &ear_det_new);
#else
	rc = pm8xxx_gpio_config(PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_EAR_DET), &ear_det);
#endif
	if (rc) {
		pr_err("%s PMIC_GPIO_EAR_DET config failed\n", __func__);
		return rc;
	}
	rc = pm8xxx_gpio_config(PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_SHORT_SENDEND), &short_sendend);
	if (rc) {
		pr_err("%s PMIC_GPIO_SHORT_SENDEND config failed\n", __func__);
		return rc;
	}
	rc = pm8xxx_gpio_config(PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_EAR_MICBIAS_EN), &ear_micbiase);
	if (rc) {
		pr_err("%s PMIC_GPIO_EAR_MICBIAS_EN config failed\n", __func__);
		return rc;
	}
pr_err("%s PMIC_GPIO_EAR_DET : OK \n", __func__);
	sec_jack_gpio_init();
#endif


#if defined(CONFIG_HAPTIC_ISA1200) || \
			defined(CONFIG_HAPTIC_ISA1200_MODULE)
	if (machine_is_msm8x60_fluid()) {
		rc = pm8xxx_gpio_config(
			PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_HAP_ENABLE),
			&en_hap_gpio_cfg);
		if (rc < 0) {
			pr_err("%s: pmic haptics gpio config failed\n",
							__func__);
		}
		rc = pm8xxx_gpio_config(
			PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_HAP_LDO_ENABLE),
			&en_hap_gpio_cfg);
		if (rc < 0) {
			pr_err("%s: pmic haptics ldo gpio config failed\n",
							__func__);
		}

	}
#endif

#if defined(CONFIG_TOUCHDISC_VTD518_SHINETSU) || \
		defined(CONFIG_TOUCHDISC_VTD518_SHINETSU_MODULE)
	if (machine_is_msm8x60_ffa() || machine_is_msm8x60_surf() ||
		machine_is_msm8x60_fusion() || machine_is_msm8x60_fusn_ffa()) {
		rc = pm8xxx_gpio_config(
			PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_TOUCH_DISC_INTR),
			&touchdisc_intr_gpio_cfg);
		if (rc < 0) {
			pr_err("%s: Touchdisc interrupt gpio config failed\n",
							__func__);
		}
	}
#endif

#if defined(CONFIG_PMIC8058_OTHC) || defined(CONFIG_PMIC8058_OTHC_MODULE)
	/* Line_in only for 8660 ffa & surf */
	if (machine_is_msm8x60_ffa() || machine_is_msm8x60_surf() ||
		machine_is_msm8x60_fusion() || machine_is_msm8x60_dragon() ||
		machine_is_msm8x60_fusn_ffa()) {
		rc = pm8xxx_gpio_config(line_in_gpio_cfg.gpio,
				&line_in_gpio_cfg.cfg);
		if (rc < 0) {
			pr_err("%s pmic line_in gpio config failed\n",
							__func__);
			return rc;
		}
	}
#endif

#if defined(CONFIG_QS_S5K4E1)
		/* qs_cam_hc37_cam_pd only for 8660 fluid qs camera*/
		if (machine_is_msm8x60_fluid()) {
			rc = pm8xxx_gpio_config(qs_hc37_cam_pd_gpio_cfg.gpio,
					&qs_hc37_cam_pd_gpio_cfg.cfg);
			if (rc < 0) {
				pr_err("%s pmic qs_hc37_cam_pd gpio config failed\n",
								__func__);
				return rc;
			}
		}
	}
#endif

	for (i = 0; i < ARRAY_SIZE(gpio_cfgs); ++i) {
		rc = pm8xxx_gpio_config(gpio_cfgs[i].gpio,
				&gpio_cfgs[i].cfg);
		if (rc < 0) {
			pr_err("%s pmic gpio config failed\n",
				__func__);
			return rc;
		}
	}

#if defined(CONFIG_VIDEO_MHL_V1) || defined(CONFIG_VIDEO_MHL_V2)
		rc = pm8xxx_gpio_config(PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_MHL_SEL), &mhl_sel);
		if (rc) {
			pr_err("%s PMIC_GPIO_MHL_SEL config failed\n", __func__);
			return rc;
		}
		rc = pm8xxx_gpio_config(PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_MHL_RST), &mhl_rst);
		if (rc) {
			pr_err("%s PMIC_GPIO_MHL_RST config failed\n", __func__);
			return rc;
		}
#if defined(CONFIG_KOR_MODEL_SHV_E110S)
	if (get_hw_rev() < 7)
		rc = pm8xxx_gpio_config(PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_MHL_INT_9), &mhl_int);
	else
		rc = pm8xxx_gpio_config(PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_MHL_INT_31), &mhl_int);
#elif defined(CONFIG_JPN_MODEL_SC_03D)
	if (get_hw_rev() < 5)
		rc = pm8xxx_gpio_config(PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_MHL_INT_9), &mhl_int);
	else
		rc = pm8xxx_gpio_config(PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_MHL_INT_31), &mhl_int);

#elif defined(CONFIG_EUR_MODEL_GT_I9210)
	if (get_hw_rev() < 0x8)
		rc = pm8xxx_gpio_config(PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_MHL_INT_9), &mhl_int);
	else
		rc = pm8xxx_gpio_config(PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_MHL_INT_31), &mhl_int);
#elif defined (CONFIG_USA_MODEL_SGH_I727)
		if (get_hw_rev() < 0x0b)
		rc = pm8xxx_gpio_config(PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_MHL_INT_9), &mhl_int);
	else
		rc = pm8xxx_gpio_config(PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_MHL_INT_31), &mhl_int);
#elif defined (CONFIG_USA_MODEL_SGH_T989) || defined (CONFIG_USA_MODEL_SGH_T769)
		if (get_hw_rev() < 0x0e)
		rc = pm8xxx_gpio_config(PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_MHL_INT_9), &mhl_int);
	else
		rc = pm8xxx_gpio_config(PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_MHL_INT_31), &mhl_int);
#elif defined(CONFIG_USA_MODEL_SGH_I757) || defined(CONFIG_USA_MODEL_SGH_I577)
	rc = pm8xxx_gpio_config(PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_MHL_INT_31), &mhl_int);
#else
	rc = pm8xxx_gpio_config(PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_MHL_INT_9), &mhl_int);
#endif
		if (rc) {
			pr_err("%s PMIC_GPIO_MHL_INT config failed\n", __func__);
			return rc;
		}
	rc = pm8xxx_gpio_config(PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_MHL_WAKE_UP), &mhl_wake_up);
	if (rc) {
		pr_err("%s PMIC_GPIO_MHL_WAKE_UP config failed\n", __func__);
		return rc;
	}
#endif
#ifdef CONFIG_OPTICAL_GP2A 
	rc = pm8xxx_gpio_config(PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_PS_VOUT), &ps_vout);
	if (rc) {
		pr_err("%s PMIC_GPIO_PS_VOUT config failed\n", __func__);
		return rc;
	}
#endif
#if defined(CONFIG_OPTICAL_TAOS)	
	rc = pm8xxx_gpio_config(PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_ALS_INT), &als_int);	
	if (rc) {		
		pr_err("%s PMIC_GPIO_ALS_INT config failed\n", __func__);		
		return rc;	
	}
#endif
#ifdef CONFIG_SENSORS_AK8975
	rc = pm8xxx_gpio_config(PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_MSENSE_RST), &msense_nRST);
	if (rc) {
		pr_err("%s PMIC_GPIO_MSENSE_RST config failed\n", __func__);
			return rc;
		}
	gpio_set_value_cansleep(PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_MSENSE_RST), 0);	
#endif

#ifdef CONFIG_GYRO_K3G
#if defined (CONFIG_KOR_MODEL_SHV_E110S)
	if( get_hw_rev() >= 0x07 ){
		gpio_tlmm_config(GPIO_CFG(MSM_GPIO_GYRO_FIFO_INT, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_8MA), GPIO_CFG_ENABLE);
		gpio_set_value_cansleep(MSM_GPIO_GYRO_FIFO_INT, 0);
	}
	else

#endif    
	rc = pm8xxx_gpio_config(PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_GYRO_FIFO_INT), &gyro_int);
	if (rc) {
		pr_err("%s PMIC_GPIO_GYRO_FIFO_INT config failed\n", __func__);
		return rc;
	}
	gpio_set_value_cansleep(PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_GYRO_FIFO_INT), 0);	
	
	rc = pm8xxx_gpio_config(PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_GYRO_INT), &gyro_int);
	if (rc) {
		pr_err("%s PMIC_GPIO_GYRO_INT config failed\n", __func__);
		return rc;
	}
	gpio_set_value_cansleep(PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_GYRO_INT), 0);	
#endif // CONFIG_GYRO_K3G

#ifdef CONFIG_SENSORS_AK8975
	{
		rc = pm8xxx_gpio_config(PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_ACCEL_INT), &accel_int);
		if (rc) {
			pr_err("%s PMIC_GPIO_ACCEL_INT config failed\n", __func__);
			return rc;
		}
		gpio_set_value_cansleep(PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_ACCEL_INT), 0);		
	}
#endif


#if defined(CONFIG_OPTICAL_GP2AP020A00F)
	rc = pm8xxx_gpio_config(PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_ALS_INT), &als_int);
	if (rc) {
		pr_err("%s PMIC_GPIO_ALS_INT config failed\n", __func__);
		return rc;
	}
#endif

#if defined(CONFIG_CHARGER_SMB328A) || defined(CONFIG_CHARGER_SMB136)
	rc = pm8xxx_gpio_config(PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_CHG_EN), &chg_en);
	if (rc) {
		pr_err("%s PMIC_GPIO_CHG_EN config failed\n", __func__);
		return rc;
	}
	
	rc = pm8xxx_gpio_config(PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_CHG_STAT), &chg_stat);
	if (rc) {
		pr_err("%s PMIC_GPIO_CHG_STAT config failed\n", __func__);
		return rc;
	}
#endif

	return 0;
}

static const unsigned int ffa_keymap[] = {
	KEY(0, 0, KEY_SEARCH),
	KEY(0, 1, KEY_BACK),
	KEY(0, 2, KEY_MENU),		
	KEY(0, 3, KEY_HOME),
	KEY(0, 4, KEY_VOLUMEDOWN),
	KEY(0, 5, KEY_VOLUMEUP),
};

static const unsigned int dragon_keymap[] = {
	KEY(0, 0, KEY_MENU),
	KEY(0, 2, KEY_1),
	KEY(0, 3, KEY_4),
	KEY(0, 4, KEY_7),

	KEY(1, 0, KEY_UP),
	KEY(1, 1, KEY_LEFT),
	KEY(1, 2, KEY_DOWN),
	KEY(1, 3, KEY_5),
	KEY(1, 4, KEY_8),

	KEY(2, 0, KEY_HOME),
	KEY(2, 1, KEY_REPLY),
	KEY(2, 2, KEY_2),
	KEY(2, 3, KEY_6),
	KEY(2, 4, KEY_0),

	KEY(3, 0, KEY_VOLUMEUP),
	KEY(3, 1, KEY_RIGHT),
	KEY(3, 2, KEY_3),
	KEY(3, 3, KEY_9),
	KEY(3, 4, KEY_SWITCHVIDEOMODE),

	KEY(4, 0, KEY_VOLUMEDOWN),
	KEY(4, 1, KEY_BACK),
	KEY(4, 2, KEY_CAMERA),
	KEY(4, 3, KEY_KBDILLUMTOGGLE),
};

static struct matrix_keymap_data ffa_keymap_data = {
	.keymap_size	= ARRAY_SIZE(ffa_keymap),
	.keymap		= ffa_keymap,
};

static struct pm8xxx_keypad_platform_data ffa_keypad_data = {
	.input_name		= "ffa-keypad",
	.input_phys_device	= "ffa-keypad/input0",
	.num_rows		= 1,
	.num_cols		= 6,
	.rows_gpio_start	= PM8058_GPIO_PM_TO_SYS(8),
	.cols_gpio_start	= PM8058_GPIO_PM_TO_SYS(0),
	.debounce_ms		= 15,
	.scan_delay_ms		= 32,
	.row_hold_ns            = 91500,
	.wakeup			= 1,
	.keymap_data		= &ffa_keymap_data,
};

static struct matrix_keymap_data dragon_keymap_data = {
	.keymap_size = ARRAY_SIZE(dragon_keymap),
	.keymap = dragon_keymap,
};

static struct pm8xxx_keypad_platform_data dragon_keypad_data = {
	.input_name = "dragon-keypad",
	.input_phys_device = "dragon-keypad/input0",
	.num_rows = 6,
	.num_cols = 5,
	.rows_gpio_start	= PM8058_GPIO_PM_TO_SYS(8),
	.cols_gpio_start	= PM8058_GPIO_PM_TO_SYS(0),
	.debounce_ms		= 15,
	.scan_delay_ms = 32,
	.row_hold_ns = 91500,
	.wakeup = 1,
	.keymap_data = &dragon_keymap_data,
};

static const unsigned int fluid_keymap[] = {
	KEY(0, 0, KEY_FN_F1),	 /* LS - PUSH1 */
	KEY(0, 1, KEY_UP),	 /* NAV - UP */
	KEY(0, 2, KEY_LEFT),	 /* NAV - LEFT */
	KEY(0, 3, KEY_VOLUMEDOWN), /* Shuttle SW_UP */

	KEY(1, 0, KEY_FN_F2),	 /* LS - PUSH2 */
	KEY(1, 1, KEY_RIGHT),    /* NAV - RIGHT */
	KEY(1, 2, KEY_DOWN),     /* NAV - DOWN */
	KEY(1, 3, KEY_VOLUMEUP),

	KEY(2, 3, KEY_ENTER),     /* SW_PUSH key */

	KEY(4, 0, KEY_CAMERA_FOCUS), /* RS - PUSH1 */
	KEY(4, 1, KEY_UP),	  /* USER_UP */
	KEY(4, 2, KEY_LEFT),	  /* USER_LEFT */
	KEY(4, 3, KEY_HOME),	  /* Right switch: MIC Bd */
	KEY(4, 4, KEY_FN_F3),	  /* Reserved MIC */

	KEY(5, 0, KEY_CAMERA), /* RS - PUSH2 */
	KEY(5, 1, KEY_RIGHT),	  /* USER_RIGHT */
	KEY(5, 2, KEY_DOWN),	  /* USER_DOWN */
	KEY(5, 3, KEY_BACK),	  /* Left switch: MIC */
	KEY(5, 4, KEY_MENU),	  /* Center switch: MIC */
};

static struct matrix_keymap_data fluid_keymap_data = {
	.keymap_size	= ARRAY_SIZE(fluid_keymap),
	.keymap		= fluid_keymap,
};

static struct pm8xxx_keypad_platform_data fluid_keypad_data = {
	.input_name		= "fluid-keypad",
	.input_phys_device	= "fluid-keypad/input0",
	.num_rows		= 6,
	.num_cols		= 5,
	.rows_gpio_start	= PM8058_GPIO_PM_TO_SYS(8),
	.cols_gpio_start	= PM8058_GPIO_PM_TO_SYS(0),
	.debounce_ms		= 15,
	.scan_delay_ms		= 32,
	.row_hold_ns            = 91500,
	.wakeup			= 1,
	.keymap_data		= &fluid_keymap_data,
};

static struct pm8xxx_vibrator_platform_data pm8058_vib_pdata = {
	.initial_vibrate_ms  = 500,
	.level_mV = 3000,
	.max_timeout_ms = 15000,
};

static struct pm8xxx_rtc_platform_data pm8058_rtc_pdata = {
	.rtc_write_enable       = false,
	.rtc_alarm_powerup	= false,
};

static struct pm8xxx_pwrkey_platform_data pm8058_pwrkey_pdata = {
	.pull_up		= 1,
	.kpd_trigger_delay_us   = 15625,
	.wakeup			= 1,
};

#define PM8058_LINE_IN_DET_GPIO	PM8058_GPIO_PM_TO_SYS(18)

static struct othc_accessory_info othc_accessories[]  = {
	{
		.accessory = OTHC_SVIDEO_OUT,
		.detect_flags = OTHC_MICBIAS_DETECT | OTHC_SWITCH_DETECT
							| OTHC_ADC_DETECT,
		.key_code = SW_VIDEOOUT_INSERT,
		.enabled = false,
		.adc_thres = {
				.min_threshold = 20,
				.max_threshold = 40,
			},
	},
	{
		.accessory = OTHC_ANC_HEADPHONE,
		.detect_flags = OTHC_MICBIAS_DETECT | OTHC_GPIO_DETECT |
							OTHC_SWITCH_DETECT,
		.gpio = PM8058_LINE_IN_DET_GPIO,
		.active_low = 1,
		.key_code = SW_HEADPHONE_INSERT,
		.enabled = true,
	},
	{
		.accessory = OTHC_ANC_HEADSET,
		.detect_flags = OTHC_MICBIAS_DETECT | OTHC_GPIO_DETECT,
		.gpio = PM8058_LINE_IN_DET_GPIO,
		.active_low = 1,
		.key_code = SW_HEADPHONE_INSERT,
		.enabled = true,
	},
	{
		.accessory = OTHC_HEADPHONE,
		.detect_flags = OTHC_MICBIAS_DETECT | OTHC_SWITCH_DETECT,
		.key_code = SW_HEADPHONE_INSERT,
		.enabled = true,
	},
	{
		.accessory = OTHC_MICROPHONE,
		.detect_flags = OTHC_GPIO_DETECT,
		.gpio = PM8058_LINE_IN_DET_GPIO,
		.active_low = 1,
		.key_code = SW_MICROPHONE_INSERT,
		.enabled = true,
	},
	{
		.accessory = OTHC_HEADSET,
		.detect_flags = OTHC_MICBIAS_DETECT,
		.key_code = SW_HEADPHONE_INSERT,
		.enabled = true,
	},
};

static struct othc_switch_info switch_info[] = {
	{
		.min_adc_threshold = 0,
		.max_adc_threshold = 100,
		.key_code = KEY_PLAYPAUSE,
	},
	{
		.min_adc_threshold = 100,
		.max_adc_threshold = 200,
		.key_code = KEY_REWIND,
	},
	{
		.min_adc_threshold = 200,
		.max_adc_threshold = 500,
		.key_code = KEY_FASTFORWARD,
	},
};

static struct othc_n_switch_config switch_config = {
	.voltage_settling_time_ms = 0,
	.num_adc_samples = 3,
	.adc_channel = CHANNEL_ADC_HDSET,
	.switch_info = switch_info,
	.num_keys = ARRAY_SIZE(switch_info),
	.default_sw_en = true,
	.default_sw_idx = 0,
};

static struct hsed_bias_config hsed_bias_config = {
	/* HSED mic bias config info */
	.othc_headset = OTHC_HEADSET_NO,
	.othc_lowcurr_thresh_uA = 100,
	.othc_highcurr_thresh_uA = 600,
	.othc_hyst_prediv_us = 7800,
	.othc_period_clkdiv_us = 62500,
	.othc_hyst_clk_us = 121000,
	.othc_period_clk_us = 312500,
	.othc_wakeup = 1,
};

static struct othc_hsed_config hsed_config_1 = {
	.hsed_bias_config = &hsed_bias_config,
	/*
	 * The detection delay and switch reporting delay are
	 * required to encounter a hardware bug (spurious switch
	 * interrupts on slow insertion/removal of the headset).
	 * This will introduce a delay in reporting the accessory
	 * insertion and removal to the userspace.
	 */
	.detection_delay_ms = 1500,
	/* Switch info */
	.switch_debounce_ms = 1500,
	.othc_support_n_switch = false,
	.switch_config = &switch_config,
	.ir_gpio = -1,
	/* Accessory info */
	.accessories_support = true,
	.accessories = othc_accessories,
	.othc_num_accessories = ARRAY_SIZE(othc_accessories),
};

static struct othc_regulator_config othc_reg = {
	.regulator	 = "8058_l5",
	.max_uV		 = 2850000,
	.min_uV		 = 2850000,
};

/* MIC_BIAS0 is configured as normal MIC BIAS */
static struct pmic8058_othc_config_pdata othc_config_pdata_0 = {
	.micbias_select = OTHC_MICBIAS_0,
	.micbias_capability = OTHC_MICBIAS,
	.micbias_enable = OTHC_SIGNAL_OFF,
	.micbias_regulator = &othc_reg,
};

#if (defined(CONFIG_SAMSUNG_JACK) || defined (CONFIG_SAMSUNG_EARJACK)) && (defined(CONFIG_TARGET_LOCALE_KOR) || defined (CONFIG_TARGET_LOCALE_USA)) 
/* MIC_BIAS1 is configured as normal MIC BIAS */
static struct pmic8058_othc_config_pdata othc_config_pdata_1 = {
	.micbias_select = OTHC_MICBIAS_1,
	.micbias_capability = OTHC_MICBIAS,
	.micbias_enable = OTHC_SIGNAL_OFF,
	.micbias_regulator = &othc_reg,
};
#else
/* MIC_BIAS1 is configured as HSED_BIAS for OTHC */
static struct pmic8058_othc_config_pdata othc_config_pdata_1 = {
	.micbias_select = OTHC_MICBIAS_1,
	.micbias_capability = OTHC_MICBIAS_HSED,
	.micbias_enable = OTHC_SIGNAL_PWM_TCXO,
	.micbias_regulator = &othc_reg,
	.hsed_config = &hsed_config_1,
	.hsed_name = "8660_handset",
};
#endif

/* MIC_BIAS2 is configured as normal MIC BIAS */
static struct pmic8058_othc_config_pdata othc_config_pdata_2 = {
	.micbias_select = OTHC_MICBIAS_2,
	.micbias_capability = OTHC_MICBIAS,
	.micbias_enable = OTHC_SIGNAL_OFF,
	.micbias_regulator = &othc_reg,
};


static void __init msm8x60_init_pm8058_othc(void)
{
	int i;

	if (SOCINFO_VERSION_MAJOR(socinfo_get_version()) == 2 ||
		machine_is_msm8x60_fluid() || machine_is_msm8x60_fusion() ||
		machine_is_msm8x60_fusn_ffa()) {
		/* 3-switch headset supported only by V2 FFA and FLUID */
		hsed_config_1.accessories_adc_support = true,
		/* ADC based accessory detection works only on V2 and FLUID */
		hsed_config_1.accessories_adc_channel = CHANNEL_ADC_HDSET,
		hsed_config_1.othc_support_n_switch = true;
	}

	/* IR GPIO is absent on FLUID */
	if (machine_is_msm8x60_fluid())
		hsed_config_1.ir_gpio = -1;

	for (i = 0; i < ARRAY_SIZE(othc_accessories); i++) {
		if (machine_is_msm8x60_fluid()) {
			switch (othc_accessories[i].accessory) {
			case OTHC_ANC_HEADPHONE:
			case OTHC_ANC_HEADSET:
				othc_accessories[i].gpio = GPIO_HEADSET_DET_N;
				break;
			case OTHC_MICROPHONE:
				othc_accessories[i].enabled = false;
				break;
			case OTHC_SVIDEO_OUT:
				othc_accessories[i].enabled = true;
				hsed_config_1.video_out_gpio = GPIO_HS_SW_DIR;
				break;
			}
		}
	}
}

static int pm8058_pwm_config(struct pwm_device *pwm, int ch, int on)
{
	struct pm_gpio pwm_gpio_config = {
		.direction      = PM_GPIO_DIR_OUT,
		.output_buffer  = PM_GPIO_OUT_BUF_CMOS,
		.output_value   = 0,
		.pull           = PM_GPIO_PULL_NO,
		.vin_sel        = PM8058_GPIO_VIN_VPH,
		.out_strength   = PM_GPIO_STRENGTH_HIGH,
		.function       = PM_GPIO_FUNC_2,
	};

	int rc = -EINVAL;
	int id, mode, max_mA;

	id = mode = max_mA = 0;
	switch (ch) {
	case 0:
	case 1:
	case 2:
		if (on) {
			id = 24 + ch;
			rc = pm8xxx_gpio_config(PM8058_GPIO_PM_TO_SYS(id - 1),
							&pwm_gpio_config);
			if (rc)
				pr_err("%s: pm8xxx_gpio_config(%d): rc=%d\n",
					__func__, id, rc);
		}
		break;

	case 6:
		id = PM_PWM_LED_FLASH;
		mode = PM_PWM_CONF_PWM1;
		max_mA = 300;
		break;

	case 7:
		id = PM_PWM_LED_FLASH1;
		mode = PM_PWM_CONF_PWM1;
		max_mA = 300;
		break;

	default:
		break;
	}

	if (ch >= 6 && ch <= 7) {
		if (!on) {
			mode = PM_PWM_CONF_NONE;
			max_mA = 0;
		}
		rc = pm8058_pwm_config_led(pwm, id, mode, max_mA);
		if (rc)
			pr_err("%s: pm8058_pwm_config_led(ch=%d): rc=%d\n",
			       __func__, ch, rc);
	}
	return rc;

}

static struct pm8058_pwm_pdata pm8058_pwm_data = {
	.config		= pm8058_pwm_config,
};

#define PM8058_GPIO_INT           88

static struct pmic8058_led pmic8058_flash_leds[] = {
	[0] = {
		.name		= "camera:flash0",
		.max_brightness = 15,
		.id		= PMIC8058_ID_FLASH_LED_0,
	},
	[1] = {
		.name		= "camera:flash1",
		.max_brightness = 15,
		.id		= PMIC8058_ID_FLASH_LED_1,
	},
};

static struct pmic8058_leds_platform_data pm8058_flash_leds_data = {
	.num_leds = ARRAY_SIZE(pmic8058_flash_leds),
	.leds	= pmic8058_flash_leds,
};

static struct pmic8058_led pmic8058_dragon_leds[] = {
	[0] = {
		/* RED */
		.name		= "led_drv0",
		.max_brightness = 15,
		.id		= PMIC8058_ID_LED_0,
	},/* 300 mA flash led0 drv sink */
	[1] = {
		/* Yellow */
		.name		= "led_drv1",
		.max_brightness = 15,
		.id		= PMIC8058_ID_LED_1,
	},/* 300 mA flash led0 drv sink */
	[2] = {
		/* Green */
		.name		= "led_drv2",
		.max_brightness = 15,
		.id		= PMIC8058_ID_LED_2,
	},/* 300 mA flash led0 drv sink */
	[3] = {
		.name		= "led_psensor",
		.max_brightness = 15,
		.id		= PMIC8058_ID_LED_KB_LIGHT,
	},/* 300 mA flash led0 drv sink */
};

static struct pmic8058_leds_platform_data pm8058_dragon_leds_data = {
	.num_leds = ARRAY_SIZE(pmic8058_dragon_leds),
	.leds	= pmic8058_dragon_leds,
};

static struct pmic8058_led pmic8058_fluid_flash_leds[] = {
	[0] = {
		.name		= "led:drv0",
		.max_brightness = 15,
		.id		= PMIC8058_ID_FLASH_LED_0,
	},/* 300 mA flash led0 drv sink */
	[1] = {
		.name		= "led:drv1",
		.max_brightness = 15,
		.id		= PMIC8058_ID_FLASH_LED_1,
	},/* 300 mA flash led1 sink */
	[2] = {
		.name		= "led:drv2",
		.max_brightness = 20,
		.id		= PMIC8058_ID_LED_0,
	},/* 40 mA led0 sink */
	[3] = {
		.name		= "keypad:drv",
		.max_brightness = 15,
		.id		= PMIC8058_ID_LED_KB_LIGHT,
	},/* 300 mA keypad drv sink */
};

static struct pmic8058_leds_platform_data pm8058_fluid_flash_leds_data = {
	.num_leds = ARRAY_SIZE(pmic8058_fluid_flash_leds),
	.leds	= pmic8058_fluid_flash_leds,
};

static struct pmic8058_charger_data pmic8058_charger_dragon = {
	.charger_data_valid = true,
	.max_source_current = 1800,
	.charger_type = CHG_TYPE_AC,
};

static struct pmic8058_charger_data pmic8058_charger_ffa_surf = {
	.charger_data_valid = false,
};

static struct pm8xxx_misc_platform_data pm8058_misc_pdata = {
	.priority		= 0,
};

static struct pm8xxx_irq_platform_data pm8058_irq_pdata = {
	.irq_base		= PM8058_IRQ_BASE,
	.devirq			= MSM_GPIO_TO_INT(PM8058_GPIO_INT),
	.irq_trigger_flag	= IRQF_TRIGGER_LOW,
};

static struct pm8xxx_gpio_platform_data pm8058_gpio_pdata = {
	.gpio_base	= PM8058_GPIO_PM_TO_SYS(0),
};

static struct pm8xxx_mpp_platform_data pm8058_mpp_pdata = {
	.mpp_base	= PM8058_MPP_PM_TO_SYS(0),
};

static struct pm8058_platform_data pm8058_platform_data = {
	.irq_pdata		= &pm8058_irq_pdata,
	.gpio_pdata		= &pm8058_gpio_pdata,
	.mpp_pdata		= &pm8058_mpp_pdata,
	.rtc_pdata		= &pm8058_rtc_pdata,
	.pwrkey_pdata		= &pm8058_pwrkey_pdata,
	.othc0_pdata		= &othc_config_pdata_0,
	.othc1_pdata		= &othc_config_pdata_1,
	.othc2_pdata		= &othc_config_pdata_2,
	.pwm_pdata		= &pm8058_pwm_data,
	.misc_pdata		= &pm8058_misc_pdata,
#ifdef CONFIG_SENSORS_MSM_ADC
	.xoadc_pdata		= &pm8058_xoadc_pdata,
#endif
};

static struct msm_ssbi_platform_data msm8x60_ssbi_pm8058_pdata __devinitdata = {
	.controller_type = MSM_SBI_CTRL_PMIC_ARBITER,
	.slave	= {
		.name			= "pm8058-core",
		.platform_data		= &pm8058_platform_data,
	},
};
#endif

#ifdef CONFIG_PMIC8901
#define PM8901_GPIO_INT           91

static struct pm8xxx_misc_platform_data pm8901_misc_pdata = {
	.priority		= 1,
};

static struct pm8xxx_irq_platform_data pm8901_irq_pdata = {
	.irq_base		= PM8901_IRQ_BASE,
	.devirq			= MSM_GPIO_TO_INT(PM8901_GPIO_INT),
	.irq_trigger_flag	= IRQF_TRIGGER_LOW,
};

static struct pm8xxx_mpp_platform_data pm8901_mpp_pdata = {
	.mpp_base		= PM8901_MPP_PM_TO_SYS(0),
};

static struct pm8901_platform_data pm8901_platform_data = {
	.irq_pdata		= &pm8901_irq_pdata,
	.mpp_pdata		= &pm8901_mpp_pdata,
	.misc_pdata		= &pm8901_misc_pdata,
};

static struct msm_ssbi_platform_data msm8x60_ssbi_pm8901_pdata __devinitdata = {
	.controller_type = MSM_SBI_CTRL_PMIC_ARBITER,
	.slave	= {
		.name = "pm8901-core",
		.platform_data = &pm8901_platform_data,
	},
};
#endif /* CONFIG_PMIC8901 */

#ifdef CONFIG_SENSORS_MSM_ADC
static struct adc_access_fn xoadc_fn = {
	pm8058_xoadc_select_chan_and_start_conv,
	pm8058_xoadc_read_adc_code,
	pm8058_xoadc_get_properties,
	pm8058_xoadc_slot_request,
	pm8058_xoadc_restore_slot,
	pm8058_xoadc_calibrate,
};

static struct msm_adc_channels msm_adc_channels_data[] = {
	{"head_set", CHANNEL_ADC_HDSET, 0, &xoadc_fn, CHAN_PATH_TYPE9,
		ADC_CONFIG_TYPE2, ADC_CALIB_CONFIG_TYPE2, scale_default},
	{"light_lux", CHANNEL_ADC_LIGHT_LUX, 0, &xoadc_fn, CHAN_PATH_TYPE7,
		ADC_CONFIG_TYPE2, ADC_CALIB_CONFIG_TYPE2, scale_default},
	{"vichg", CHANNEL_ADC_CHG_MONITOR, 0, &xoadc_fn, CHAN_PATH_TYPE10,
		ADC_CONFIG_TYPE2, ADC_CALIB_CONFIG_TYPE2, scale_xtern_chgr_cur},
	{"batt_id", CHANNEL_ADC_BATT_ID, 0, &xoadc_fn, CHAN_PATH_TYPE6,
		ADC_CONFIG_TYPE2, ADC_CALIB_CONFIG_TYPE2, scale_default},
#if defined (CONFIG_TARGET_LOCALE_USA)
	{"batt_therm", CHANNEL_ADC_BATT_THERM, 0, &xoadc_fn, CHAN_PATH_TYPE8,
		ADC_CONFIG_TYPE2, ADC_CALIB_CONFIG_TYPE2, scale_default},
#else
	{"batt_therm", CHANNEL_ADC_BATT_THERM, 0, &xoadc_fn, CHAN_PATH_TYPE8,
		ADC_CONFIG_TYPE2, ADC_CALIB_CONFIG_TYPE2, scale_sec_settherm},
#endif
	{"pmic_therm", CHANNEL_ADC_PMIC_THERM, 0, &xoadc_fn, CHAN_PATH_TYPE12,
		ADC_CONFIG_TYPE2, ADC_CALIB_CONFIG_TYPE2, scale_pmic_therm},
	{"xo_therm", CHANNEL_ADC_XOTHERM, 0, &xoadc_fn, CHAN_PATH_TYPE_NONE,
		ADC_CONFIG_TYPE2, ADC_CALIB_CONFIG_TYPE2, scale_sec_settherm},
	{"xo_therm_4K", CHANNEL_ADC_XOTHERM_4K, 0, &xoadc_fn, CHAN_PATH_TYPE_NONE,
		ADC_CONFIG_TYPE1, ADC_CALIB_CONFIG_TYPE2, scale_sec_settherm}
};

static char *msm_adc_fluid_device_names[] = {
	"ADS_ADC1",
	"ADS_ADC2",
};

static struct msm_adc_platform_data msm_adc_pdata = {
	.channel = msm_adc_channels_data,
	.target_hw = MSM_8x60,
	.num_chan_supported = ARRAY_SIZE(msm_adc_channels_data),
};

struct platform_device msm_adc_device = {
	.name   = "msm_adc",
	.id = -1,
	.dev = {
		.platform_data = &msm_adc_pdata,
	},
};
#endif

void __init msm8x60_init_pmic(void)
{
	uint32_t soc_platform_version = socinfo_get_platform_version();
#ifdef CONFIG_USB_EHCI_MSM_72K
	struct pm8xxx_mpp_config_data hsusb_phy_mpp = {
		.type		= PM8XXX_MPP_TYPE_D_OUTPUT,
		.level		= PM8901_MPP_DIG_LEVEL_L5,
		.control	= PM8XXX_MPP_DOUT_CTRL_HIGH,
	};
#endif

	pmic_reset_irq = PM8058_IRQ_BASE + PM8058_RESOUT_IRQ;

#if defined(CONFIG_PMIC8058_OTHC) || defined(CONFIG_PMIC8058_OTHC_MODULE)
	msm8x60_init_pm8058_othc();
#endif

	msm_device_ssbi_pmic1.dev.platform_data =
				&msm8x60_ssbi_pm8058_pdata;
	msm_device_ssbi_pmic2.dev.platform_data =
				&msm8x60_ssbi_pm8901_pdata;
	pm8901_platform_data.num_regulators = pm8901_regulator_pdata_len;

	/* configure pmic */
	if (machine_is_msm8x60_fluid()) {
		pm8058_platform_data.keypad_pdata = &fluid_keypad_data;
		pm8058_platform_data.leds_pdata = &pm8058_fluid_flash_leds_data;
	} else if (machine_is_msm8x60_dragon()) {
		pm8058_platform_data.keypad_pdata = &dragon_keypad_data;
		pm8058_platform_data.leds_pdata = &pm8058_dragon_leds_data;
		pm8058_platform_data.charger_pdata = &pmic8058_charger_dragon;
	} else {
		pm8058_platform_data.keypad_pdata = &ffa_keypad_data;
		pm8058_platform_data.leds_pdata = &pm8058_flash_leds_data;
		pm8058_platform_data.charger_pdata = &pmic8058_charger_ffa_surf;
	}

	if (machine_is_msm8x60_ffa() || machine_is_msm8x60_fusn_ffa() ||
		machine_is_msm8x60_dragon()) {
		pm8058_platform_data.vibrator_pdata = &pm8058_vib_pdata;
	}

	/* if csfb, enable rtc write */
	if (get_baseband() == 0) {
		pm8058_rtc_pdata.rtc_write_enable = true;
	}

#ifdef CONFIG_USB_EHCI_MSM_72K
	/*
	 * Drive MPP2 pin HIGH for PHY to generate ID interrupts on 8660
	 * fluid
	 */
	if (machine_is_msm8x60_fluid())
		pm8xxx_mpp_config(PM8901_MPP_PM_TO_SYS(1), &hsusb_phy_mpp);
#endif

#ifdef CONFIG_SENSORS_MSM_ADC
	if (machine_is_msm8x60_fluid()) {
		msm_adc_pdata.dev_names = msm_adc_fluid_device_names;
		msm_adc_pdata.num_adc = ARRAY_SIZE(msm_adc_fluid_device_names);
		if (SOCINFO_VERSION_MAJOR(soc_platform_version) < 3)
			msm_adc_pdata.gpio_config = APROC_CONFIG;
		else
			msm_adc_pdata.gpio_config = MPROC_CONFIG;
	}
#endif
}
