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

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/i2c-gpio.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/gpio_event.h>
#include <linux/keyreset.h>
#include <asm/mach-types.h>
#include <linux/gpio.h>
#include <mach/gpio.h>

#include <linux/moduleparam.h>
#include <mach/devices-lte.h>
#include "devices-msm8x60.h"
#include "board-celox.h"

#include <linux/mfd/pmic8058.h>

#ifdef CONFIG_KEYPAD_CYPRESS_TOUCH
static struct i2c_board_info tkey_i2c_devices[] = {
	{
		I2C_BOARD_INFO("sec_touchkey", 0x20),
	},
};

static struct i2c_gpio_platform_data tkey_i2c_gpio_data = {
	.sda_pin = GPIO_TKEY_I2C_SDA,
	.scl_pin = GPIO_TKEY_I2C_SCL,
	.udelay = 0,
};

static struct platform_device tkey_i2c_gpio_device = {
        .name = "i2c-gpio",
        .id = MSM_TKEY_I2C_BUS_ID,
        .dev = {
		.platform_data = &tkey_i2c_gpio_data,
	}
};

static int __init tkey_device_init(void)
{
	struct regulator *TKEY_L12 = NULL;
	int rc;       
#if defined(CONFIG_KOR_MODEL_SHV_E110S) || defined (CONFIG_TARGET_LOCALE_USA)
	struct regulator *lvs2;
#endif

	gpio_tlmm_config(GPIO_CFG(156, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),1);
	gpio_tlmm_config(GPIO_CFG(157, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),1);

#if defined(CONFIG_KOR_MODEL_SHV_E110S) || defined (CONFIG_TARGET_LOCALE_USA)
	lvs2 = regulator_get(NULL, "8901_lvs2");

	rc = regulator_enable(lvs2);
        if(rc)	
		printk("[TKEY] %s: error enabling regulator\n", __func__);
        else 
		printk("[TKEY] %s: 8901_lvs2 On \n", __func__);

	regulator_put(lvs2);
#endif	

	TKEY_L12 = regulator_get(NULL, "8058_l12");

	if (IS_ERR(TKEY_L12)) {
		rc = PTR_ERR(TKEY_L12);
	    pr_err("%s: L12 get failed (%d)\n", __func__, rc);
		return rc;
	}

	rc = regulator_set_voltage(TKEY_L12, 3300000, 3300000);
	if (rc) {
		pr_err("%s: L12 set level failed (%d)\n", __func__, rc);
		return rc;
	}

	rc = regulator_enable(TKEY_L12);
	if (rc) {
		pr_err("%s: L12vreg enable failed (%d)\n", __func__, rc);
		return rc;
	}

	regulator_put(TKEY_L12);
	printk("[TKEY] %s: TKEY_L12 3.3 V Set \n", __func__);

	return 0;
}

#if defined(CONFIG_KOR_MODEL_SHV_E110S) || defined (CONFIG_TARGET_LOCALE_USA) || defined (CONFIG_JPN_MODEL_SC_03D)
int tkey_vdd_enable(int onoff)
{
	struct regulator *lvs2;	
	int ret;
	printk("[TKEY] %s: enter\n", __func__);

	lvs2 = regulator_get(NULL, "8901_lvs2");
	if (IS_ERR(lvs2))
		return -1;

	if(onoff) {
		ret = regulator_enable(lvs2);
		if (ret) {
			printk("%s: error enabling regulator\n", __func__);
		}
	} else {
		ret = regulator_disable(lvs2);
		if (ret) {
			printk("%s: error enabling regulator\n", __func__);
		}
	}

	regulator_put(lvs2);
	return 0;
}
EXPORT_SYMBOL(tkey_vdd_enable);

int tkey_led_vdd_enable(int onoff) 
{
	struct regulator *l12;
	int ret;
	printk("tkey_led_vdd_enable %d\n", onoff);
#if defined(CONFIG_EUR_MODEL_GT_I9210)
	if(get_hw_rev() >= 0x07)
#elif defined(CONFIG_USA_MODEL_SGH_T989) || defined (CONFIG_USA_MODEL_SGH_T769)
	if(get_hw_rev() >= 0x08)
#elif defined(CONFIG_USA_MODEL_SGH_I727)
	if(get_hw_rev() >= 0x0a)
#elif defined(CONFIG_USA_MODEL_SGH_I757) || defined(CONFIG_USA_MODEL_SGH_I577)
	if(true)
#else
	if(get_hw_rev() >= 0x04)
#endif
	{
		l12 = regulator_get(NULL, "8058_l12");
		if (IS_ERR(l12))
			return -1;

		if(onoff) {
			ret = regulator_set_voltage(l12, 3300000, 3300000);
			if (ret) {
				printk("%s: error setting voltage\n", __func__);
			}

			if(!regulator_is_enabled(l12)) {
				ret = regulator_enable(l12);
				if (ret) {
					printk("%s: error enabling regulator\n", __func__);
				}
			}
		} else {
			ret = regulator_set_voltage(l12, 3300000, 3300000);
			if (ret) {
				printk("%s: error setting voltage\n", __func__);
			}

			if(regulator_is_enabled(l12)) {
				ret = regulator_disable(l12);
				if (ret) {
					printk("%s: error enabling regulator\n", __func__);
				}
			}
		}
		regulator_put(l12);
	}
	return 0;
}
EXPORT_SYMBOL(tkey_led_vdd_enable);
#endif
#endif

int __init msm8x60_init_keypad(void)
{
#ifdef CONFIG_KEYPAD_CYPRESS_TOUCH
	int rc;
	struct pm_gpio tkey_int = {
		.direction      = PM_GPIO_DIR_IN,
		.pull           = PM_GPIO_PULL_NO,
		.vin_sel        = 2,
		.function       = PM_GPIO_FUNC_NORMAL,
		.inv_int_pol    = 0,
	};

	rc = pm8xxx_gpio_config(PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_TKEY_INT), &tkey_int);
	if (rc) {
		pr_err("%s PMIC_GPIO_TKEY_INT config failed\n", __func__);
		return rc;
	}

	i2c_register_board_info(MSM_TKEY_I2C_BUS_ID,
				tkey_i2c_devices, ARRAY_SIZE(tkey_i2c_devices));

	rc = platform_device_register(&tkey_i2c_gpio_device);
	if (rc) {
		pr_err("%s tkey_i2c_gpio_device device register failed\n", __func__);
		return rc;
	}

	tkey_device_init();
#endif

	return 0;
}
