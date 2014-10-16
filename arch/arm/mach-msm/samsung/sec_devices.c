/*
 * sec_devices.c
 */

#include <linux/gpio.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <asm/system_info.h>
#include <mach/devices-lte.h>

/* setting whether the baseband is csfb or svlte2a. default: csfb */
static int target_baseband = 0;
static int __init target_baseband_setup(char *baseband)
{
	target_baseband = 0;

	if (!strcmp(baseband, "csfb"))
		target_baseband = 0;
	else if (!strcmp(baseband, "svlte2a"))
		target_baseband = 1;

	return 1;
}
__setup("androidboot.baseband=", target_baseband_setup);

unsigned int get_baseband(void)
{
	return target_baseband;
}

/* for hw revision */
static unsigned int HWREV;
static unsigned char is_checked_hwrev = 0;

static int __init hw_rev_setup(char *str)
{
	if (!str)
		return 0;

	printk("%s : HW rev value is intialized first\n", __func__);
	HWREV = simple_strtoul(str, &str, 0);
	printk("HW rev value : 0x%X\n", HWREV);

	is_checked_hwrev = 1;
	return 1;
}
__setup("hw_rev=", hw_rev_setup);

unsigned int get_hw_rev(void)
{
	int i = 0;                
	char str_rev[10];
	int return_value;
	unsigned int temp_rev = 0;

	if(!is_checked_hwrev) {
		printk("%s : HW rev value is intialized first\n", __func__);

		for(i = 0; i < 3; i++) {
			sprintf(str_rev, "HW_REV_%d",i);
			return_value = gpio_request(PM8058_GPIO_PM_TO_SYS(PM8058_GPIO(REV_GPIO_BASE+i)), str_rev);       
			return_value = gpio_direction_input(PM8058_GPIO_PM_TO_SYS(PM8058_GPIO(REV_GPIO_BASE+i)));
#if defined (CONFIG_KOR_MODEL_SHV_E160S) || defined(CONFIG_KOR_MODEL_SHV_E160K) || defined (CONFIG_KOR_MODEL_SHV_E160L) || defined (CONFIG_JPN_MODEL_SC_05D)
			temp_rev = temp_rev | (gpio_get_value_cansleep(PM8058_GPIO_PM_TO_SYS(PM8058_GPIO(REV_GPIO_BASE+i)))<< (2-i));
#else
			temp_rev = temp_rev | (gpio_get_value_cansleep(PM8058_GPIO_PM_TO_SYS(PM8058_GPIO(REV_GPIO_BASE+i)))<<i);
#endif
		}

#if defined (CONFIG_KOR_MODEL_SHV_E160S) || defined(CONFIG_KOR_MODEL_SHV_E160K) || defined (CONFIG_KOR_MODEL_SHV_E160L) || defined (CONFIG_JPN_MODEL_SC_05D)
		if (temp_rev == 0)
			temp_rev = 9;
		else if (temp_rev == 1)
			temp_rev = 10;
#endif

		sprintf(str_rev, "HW_REV_%d", 3);
		return_value = gpio_request(PM8058_GPIO_PM_TO_SYS(REV_PMIC_BASE), str_rev);
		return_value = gpio_direction_input(PM8058_GPIO_PM_TO_SYS(REV_PMIC_BASE));
		temp_rev = temp_rev | (gpio_get_value_cansleep(PM8058_GPIO_PM_TO_SYS(REV_PMIC_BASE))<<3);
#if defined (CONFIG_USA_MODEL_SGH_I717)
		for(i=0; i < (sizeof(hw_rev_map) / sizeof(hw_rev_mapping)); i++) {
			if(temp_rev == hw_rev_map[i].real_hw_rev) {
				temp_rev = hw_rev_map[i].hw_rev;
				break;
			}
		}
#endif

#if defined (CONFIG_KOR_MODEL_SHV_E160L)
		temp_rev += 4;
#endif
		printk("HW rev value : 0x%X\n", temp_rev);
		HWREV = temp_rev;

		is_checked_hwrev = 1;
	}
	return HWREV;
}
EXPORT_SYMBOL(get_hw_rev);
