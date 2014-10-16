 /* Copyright (c) 2010, Code Aurora Forum. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of Code Aurora Forum, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#ifndef __ARCH_ARM_MACH_MSM_DEVICES_LTE_H
#define __ARCH_ARM_MACH_MSM_DEVICES_LTE_H

#include <mach/board-msm8660.h>

#define REV_GPIO_BASE 34
#if defined (CONFIG_USA_MODEL_SGH_I727) || defined(CONFIG_USA_MODEL_SGH_I757) || defined(CONFIG_USA_MODEL_SGH_I577)
#define REV_PMIC_BASE PM8058_GPIO(26)
#else
#define REV_PMIC_BASE PM8058_GPIO(32)
#endif

#if defined (CONFIG_USA_MODEL_SGH_I717)
typedef struct _hw_rev_mapping {
	unsigned int real_hw_rev;
	unsigned int hw_rev;
} hw_rev_mapping;

static const hw_rev_mapping hw_rev_map[] = {
	{ .real_hw_rev = 0x05, .hw_rev = 0x00 },
	{ .real_hw_rev = 0x03, .hw_rev = 0x01 },
	{ .real_hw_rev = 0x02, .hw_rev = 0x02 },
	{ .real_hw_rev = 0x04, .hw_rev = 0x03 },
	{ .real_hw_rev = 0x06, .hw_rev = 0x04 },
	{ .real_hw_rev = 0x07, .hw_rev = 0x05 },
	{ .real_hw_rev = 0x08, .hw_rev = 0x07 },
	{ .real_hw_rev = 0x09, .hw_rev = 0x0B },
	{ .real_hw_rev = 0x0A, .hw_rev = 0x0C },
};
#endif

unsigned int get_baseband(void);
unsigned int get_hw_rev(void);

#ifdef CONFIG_USB_ANDROID_SAMSUNG_COMPOSITE
/* soonyong.cho : Define samsung product id and config string.
 *                Sources such as 'android.c' and 'devs.c' refered below define
 */
#define SAMSUNG_VENDOR_ID		0x04e8

#ifdef CONFIG_USB_ANDROID_SAMSUNG_ESCAPE
	/* USE DEVGURU HOST DRIVER */
	/* 0x6860 : MTP(0) + MS Composite (UMS) */
	/* 0x685E : UMS(0) + MS Composite (ADB) */
#define SAMSUNG_KIES_PRODUCT_ID		0x685E	/* UMS(0) + MS Composite */
#define SAMSUNG_DEBUG_PRODUCT_ID	0x685E	/* UMS(0) + MS Composite (ADB) */
#define SAMSUNG_UMS_PRODUCT_ID		0x685B  /* UMS Only */
#define SAMSUNG_MTP_PRODUCT_ID		0x685C  /* MTP Only */
#ifdef CONFIG_USB_ANDROID_SAMSUNG_RNDIS_WITH_MS_COMPOSITE
#define SAMSUNG_RNDIS_PRODUCT_ID	0x6861  /* RNDIS(0,1) + UMS (2) + MS Composite */
#else
#define SAMSUNG_RNDIS_PRODUCT_ID	0x6863  /* RNDIS only */
#endif
#else 
/* USE MCCI HOST DRIVER */
#define SAMSUNG_KIES_PRODUCT_ID		0x6877	/* Shrewbury ACM+MTP*/
#define SAMSUNG_DEBUG_PRODUCT_ID	0x681C	/* Shrewbury ACM+UMS+ADB*/
#define SAMSUNG_UMS_PRODUCT_ID		0x681D
#define SAMSUNG_MTP_PRODUCT_ID		0x68A9
#define SAMSUNG_RNDIS_PRODUCT_ID	0x6881
#endif
#define ANDROID_DEBUG_CONFIG_STRING	 "ACM + UMS + ADB (Debugging mode)"
#define ANDROID_KIES_CONFIG_STRING	 "ACM + UMS (SAMSUNG KIES mode)"
#define ANDROID_UMS_CONFIG_STRING	 "UMS Only (Not debugging mode)"
#define ANDROID_MTP_CONFIG_STRING	 "MTP Only (Not debugging mode)"
#ifdef CONFIG_USB_ANDROID_SAMSUNG_RNDIS_WITH_MS_COMPOSITE
#define ANDROID_RNDIS_CONFIG_STRING	 "RNDIS + UMS (Not debugging mode)"
#else
#define ANDROID_RNDIS_CONFIG_STRING	 "RNDIS Only (Not debugging mode)"
#endif

/* Refered from S1, P1 */
#define USBSTATUS_UMS				0x0
#define USBSTATUS_SAMSUNG_KIES 		0x1
#define USBSTATUS_MTPONLY			0x2
#define USBSTATUS_ASKON				0x4
#define USBSTATUS_VTP				0x8
#define USBSTATUS_ADB				0x10
#define USBSTATUS_DM				0x20
#define USBSTATUS_ACM				0x40
#define USBSTATUS_SAMSUNG_KIES_REAL		0x80
#endif /* CONFIG_USB_ANDROID_SAMSUNG_COMPOSITE */

#endif
