/*
*
* drivers/media/isdbtmm/radio-mb86a35s.c
*
* isdbtmm driver
*
* Copyright (C) (2012, Samsung Electronics)
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation version 2.
*
* This program is distributed "as is" WITHOUT ANY WARRANTY of any
* kind, whether express or implied; without even the implied warranty
* of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
*/

#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/string.h>

#include <linux/types.h>
#include <linux/fcntl.h>

/* for delay(sleep) */
#include <linux/delay.h>

/* for mutex */
#include <linux/mutex.h>

/*using copy to user */
#include <linux/uaccess.h>
#include <linux/clk.h>
#include <linux/mm.h>
#include <linux/slab.h>

#include <linux/workqueue.h>
#include <linux/irq.h>
#include <asm/mach/irq.h>
#include <linux/interrupt.h>
#include <linux/vmalloc.h>

#include <linux/io.h>
#include <mach/gpio.h>

static struct class *mb86a35_class;

#include "radio-mb86a35s.h"


static int mb86a35_com_open(struct inode *inode, struct file *filp)
{
	int ret = 0;
	
	ret = mb86a35_open(inode, filp);

	return ret;
}

static int mb86a35_com_release(struct inode *inode, struct file *filp)
{
	int ret = 0;
	
	ret = mb86a35_close(inode, filp);

	return ret;
}

static ssize_t mb86a35_com_read(struct file *filp, char *buf, size_t count, loff_t * f_pos)
{
	int ret = 0;
	
	ret = mb86a35_read(filp, buf, count, f_pos);

	return ret;
}


static int mb86a35_com_write(struct file *filp, const char __user * buf, size_t count, loff_t * f_pos)
{
	int ret = 0;

	ret = mb86a35_write(filp, buf, count, f_pos);
		 
	return ret;
}

static long mb86a35_com_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int ret = 0;

	ret = mb86a35_ioctl(filp, cmd, arg);
		 
	return ret;
}

static int mb86a35_com_mmap(struct file *filp, struct vm_area_struct *vma)
{
	int ret = 0;

	ret = mb86a35_mmap(filp, vma);
		 
	return ret;
}

static const struct file_operations mb86a35_com_ctl_fops = {
	.owner          = THIS_MODULE,
	.open           = mb86a35_com_open,
	.write          = mb86a35_com_write,
	.read           = mb86a35_com_read,
	.unlocked_ioctl = mb86a35_com_ioctl,
	.release		= mb86a35_com_release,
	.mmap           = mb86a35_com_mmap,
};

static int mb86a35_com_probe(struct platform_device *pdev)
{
	int ret;
	struct device *mb86a35_dev;

	ret = register_chrdev(NODE_MAJOR, NODE_PATHNAME, &mb86a35_com_ctl_fops);
	if (ret < 0)
		printk("register_chrdev(MB86A35_DEV) failed!\n");

	mb86a35_class = class_create(THIS_MODULE, NODE_PATHNAME);
	
	if (IS_ERR(mb86a35_class)) {
		unregister_chrdev(NODE_MAJOR, NODE_PATHNAME);
		class_destroy(mb86a35_class);
		printk("class_create failed!\n");

		return -EFAULT;
	}

	mb86a35_dev = device_create(mb86a35_class, 
							NULL,
							MKDEV(NODE_MAJOR, NODE_MINOR),
							NULL, 
							NODE_PATHNAME);
				
	if (IS_ERR(mb86a35_dev)) {
		printk("device_create failed!\n");

		unregister_chrdev(NODE_MAJOR, NODE_PATHNAME);
		class_destroy(mb86a35_class);

		return -EFAULT;
	}
	proc_init_module();

	return 0;
}

static int mb86a35_com_remove(struct platform_device *pdev)
{
	return 0;
}

static int mb86a35_com_suspend(struct platform_device *pdev, pm_message_t mesg)
{
	return 0;
}

static int mb86a35_com_resume(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver mb86a35_driver = {
	.probe	 = mb86a35_com_probe,
	.remove  = mb86a35_com_remove,
	.suspend = mb86a35_com_suspend,
	.resume  = mb86a35_com_resume,
	.driver  = {
			.owner	= THIS_MODULE,
			.name = "mb86a35s"
	},
};

void mb86a35_com_gpio(unsigned long arg)
{
	int cnt, ret;

	if (arg) {
		
		for (cnt = 0; cnt < ARRAY_SIZE(mb86a35_on_gpio_table); cnt++) {
			ret = gpio_tlmm_config(mb86a35_on_gpio_table[cnt], GPIO_CFG_ENABLE);
			if (ret) {
				printk("mb86a35_com_gpio_on failed");
				break;
			}
		}
	
		/* TMM_PWR_EN HIGH */
		gpio_set_value(GPIO_PWR_EN, 1);

		/* 14ms sleep */
		usleep_range(14000, 14000);
		
		/* TMM_XRESET HIGH */
		gpio_set_value(GPIO_XRESET, 1);
		
		/* 1.5ms sleep */
		usleep_range(1500, 1500);

	}
	else {
		
		/* TMM_XRESET LOW */
		gpio_set_value(GPIO_XRESET, 0);
		
		/* TMM_PWR_EN LOW */
		gpio_set_value(GPIO_PWR_EN, 0);
		
		for (cnt = 0; cnt < ARRAY_SIZE(mb86a35_off_gpio_table); cnt++) {
			ret = gpio_tlmm_config(mb86a35_off_gpio_table[cnt], GPIO_CFG_ENABLE);
			if (ret){
				printk("mb86a35_com_gpio_off failed");
				break;
			}
		}


	}

	return;
}

static int __init mb86a35_com_init(void)
{
	int ret;

	ret = platform_driver_register(&mb86a35_driver);

	if (ret)
	{
		printk("MB86A35 module init : Error = %x\n", ret);
		return ret; 
	}
	
	return 0;
}

static void __exit mb86a35_com_exit(void)
{

	/* POWER OFF */
	mb86a35_com_gpio(POWER_OFF);

	unregister_chrdev(NODE_MAJOR, NODE_PATHNAME);
	device_destroy(mb86a35_class, MKDEV(NODE_MAJOR, NODE_MINOR));
	class_destroy(mb86a35_class);

	platform_driver_unregister(&mb86a35_driver);

	proc_cleanup_module();
}

module_init(mb86a35_com_init);
module_exit(mb86a35_com_exit);

MODULE_AUTHOR("Samsung");
MODULE_DESCRIPTION("ISDBTMM Driver");
MODULE_LICENSE("GPL v2");

