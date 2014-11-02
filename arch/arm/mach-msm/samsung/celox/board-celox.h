/* Copyright (c) 2010-2013, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __ARCH_ARM_MACH_MSM_BOARD_CELOX_H
#define __ARCH_ARM_MACH_MSM_BOARD_CELOX_H

#include <linux/mfd/pmic8058.h>

#ifdef CONFIG_OPTICAL_GP2A
#define PMIC_GPIO_PS_VOUT	PM8058_GPIO(14) 	/* PMIC GPIO Number 14 */
#endif
#ifdef CONFIG_SENSORS_AK8975
#define PMIC_GPIO_MSENSE_RST	PM8058_GPIO(33) 	/* PMIC GPIO Number 33 */
#endif
#ifdef CONFIG_GYRO_K3G
#define PMIC_GPIO_GYRO_FIFO_INT	PM8058_GPIO(11) 	/* PMIC GPIO Number 11 */
#define MSM_GPIO_GYRO_FIFO_INT	102 	               /* MSM GPIO Number 102 */
#define PMIC_GPIO_GYRO_INT	PM8058_GPIO(12) 	/* PMIC GPIO Number 11 */
#endif
#ifdef CONFIG_SENSORS_AK8975
#define PMIC_GPIO_ACCEL_INT	PM8058_GPIO(10) 	/* PMIC GPIO Number 10 */
#define PMIC_GPIO_ACCEL_INT_11	PM8058_GPIO(11) 	/* PMIC GPIO Number 11 */
#endif
#ifdef CONFIG_KEYPAD_CYPRESS_TOUCH
#define PMIC_GPIO_TKEY_INT	PM8058_GPIO(13) 	/* PMIC GPIO Number 13 */
#endif
#if defined(CONFIG_VIDEO_MHL_V1) || defined(CONFIG_VIDEO_MHL_V2)
#define PMIC_GPIO_MHL_RST       PM8058_GPIO(15)
#define PMIC_GPIO_MHL_SEL	PM8058_GPIO(16)
#define PMIC_GPIO_MHL_INT_9	PM8058_GPIO(9)	/* PMIC GPIO Number 9 */
#define PMIC_GPIO_MHL_INT_31    PM8058_GPIO(31)
#define PMIC_GPIO_MHL_WAKE_UP	PM8058_GPIO(17)
#if defined(CONFIG_VIDEO_MHL_V1)
#define GPIO_MHL_I2C_SCL	65
#define GPIO_MHL_I2C_SDA	64
#elif defined(CONFIG_VIDEO_MHL_V2)
#define GPIO_MHL_RST PM8058_GPIO_PM_TO_SYS(PM8058_GPIO(15))
#define GPIO_MHL_SEL PM8058_GPIO_PM_TO_SYS(PM8058_GPIO(16))
#endif
#endif
/* Common PMIC GPIO ***************************/
#ifdef CONFIG_OPTICAL_GP2A
#define PMIC_GPIO_PS_VOUT	PM8058_GPIO(14) 	/* PMIC GPIO Number 14 */
#endif
#ifdef CONFIG_OPTICAL_GP2AP020A00F
#define PMIC_GPIO_ALS_INT	PM8058_GPIO(3) 	/* PMIC GPIO Number 3 */
#define PMIC_GPIO_ALS_INT_REV08	PM8058_GPIO(14) 	/* PMIC GPIO Number 3 */
#endif
#ifdef CONFIG_OPTICAL_TAOS
#define PMIC_GPIO_ALS_INT	PM8058_GPIO(14) 	/* PMIC GPIO Number 14 */
#endif
#ifdef CONFIG_KEYPAD_CYPRESS_TOUCH
#define GPIO_TKEY_I2C_SCL	157
#define GPIO_TKEY_I2C_SDA	156
#endif
#ifdef CONFIG_USB_SWITCH_FSA9480
#define PMIC_GPIO_TA_CURRENT_SEL PM8058_GPIO(18)
#endif

#if defined(CONFIG_SAMSUNG_JACK) || defined (CONFIG_SAMSUNG_EARJACK)
#define PMIC_GPIO_EAR_DET	PM8058_GPIO(27)  	/* PMIC GPIO Number 27 */
#define PMIC_GPIO_SHORT_SENDEND	PM8058_GPIO(28)  	/* PMIC GPIO Number 28 */
#define PMIC_GPIO_EAR_MICBIAS_EN PM8058_GPIO(29) /* PMIC GPIO Number 29  */
#endif

#if defined(CONFIG_CHARGER_SMB328A) || defined(CONFIG_CHARGER_SMB136)
#define PMIC_GPIO_CHG_EN	PM8058_GPIO(23)
#define PMIC_GPIO_CHG_STAT	PM8058_GPIO(24)
#endif

#if defined(CONFIG_PN544_NFC)
#define PMIC_GPIO_NFC_IRQ		PM8058_GPIO_PM_TO_SYS(7) //8
#define PMIC_GPIO_NFC_EN 		PM8058_GPIO_PM_TO_SYS(29) //30
#define GPIO_NFC_FIRM		71 
#endif

#if defined (CONFIG_OPTICAL_GP2A) || defined(CONFIG_OPTICAL_TAOS)
#define SENSOR_ALS_SCL   		139
#define SENSOR_ALS_SDA   		138
#endif

#ifdef CONFIG_SENSORS_YDA165
/* Audio AMP Driver GPIO */
#define GPIO_AMP_I2C_SCL	154
#define GPIO_AMP_I2C_SDA	155
#endif

#if defined(CONFIG_TDMB) || defined(CONFIG_TDMB_MODULE)
#define GPIO_TDMB_EN    130
#define GPIO_TDMB_RST   126
#define GPIO_TDMB_INT   128
#define GPIO_TDMB_SPI_MOSI	  33
#define GPIO_TDMB_SPI_MISO	  34
#define GPIO_TDMB_SPI_CS 	  35
#define GPIO_TDMB_SPI_CLK	  36

enum {
	TDMB_PMIC_CLK_INIT,
	TDMB_PMIC_CLK_ON,
	TDMB_PMIC_CLK_OFF,
};
#endif

#if defined(CONFIG_TOUCHSCREEN_QT602240) || defined(CONFIG_TOUCHSCREEN_MXT768E) \
	|| defined(CONFIG_TOUCHSCREEN_MELFAS)
#define TOUCHSCREEN_IRQ		125  
#define TSP_SDA			43
#define TSP_SCL			44
#endif

#ifdef CONFIG_MSM_CAMERA
#define	GPIO_CAM_IO_EN		37	
#define GPIO_ISP_INT		49
#define	GPIO_CAM_MAIN_RST	50
#define	GPIO_CAM_SUB_RST	41
#define	GPIO_CAM_SUB_EN		42
#endif

#ifdef CONFIG_BROADCOM_WIFI
int __init brcm_wlan_init(void);
#endif

#ifdef CONFIG_BATTERY_SEC
extern unsigned int sec_get_lpm_mode(void);
#endif

extern struct regulator_init_data msm_saw_regulator_pdata_s0;
extern struct regulator_init_data msm_saw_regulator_pdata_s1;
extern struct rpm_regulator_platform_data msm8x60_rpm_regulator_early_pdata __devinitdata;
extern struct rpm_regulator_platform_data msm8x60_rpm_regulator_pdata __devinitdata;
extern struct platform_device msm8x60_8901_mpp_vreg __devinitdata;
extern struct pm8901_vreg_pdata pm8901_regulator_pdata[];
extern int pm8901_regulator_pdata_len;
extern struct platform_device msm_adc_device;

int __init msm8x60_init_keypad(void);
int msm8x60_multi_sdio_init(void);
#ifdef CONFIG_FB_MSM_HDMI_MSM_PANEL
int hdmi_enable_5v(int on);
#endif

void __init msm8x60_init_mmc(void);
void __init msm8x60_init_fb(void);
void __init msm8x60_init_pmic(void);
void __init msm8x60_gpio_mpp_init(void);
void msm8x60_init_gpiomux(void);
void __init msm8x60_allocate_fb_region(void);
void __init msm8x60_surf_rpm_fixup(void);
void msm8x60_pmic_regulator_force_off(void);
void __init msm8x60_pm8901_gpio_mpp_init(void);
void msm8x60_mdp_writeback(struct memtype_reserve *reserve_table);

#define _GET_REGULATOR(var, name) do {					\
	if (var == NULL) {						\
		var = regulator_get(NULL, name);			\
		if (IS_ERR(var)) {					\
			pr_err("'%s' regulator not found, rc=%ld\n",	\
				name, PTR_ERR(var));			\
			var = NULL;					\
		}							\
	}								\
} while (0)

#endif
