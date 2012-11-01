/*
*
* drivers/media/isdbtmm/radio-mb86a35s.h
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

#include <mach/gpiomux.h>

#define	NODE_MAJOR		225	 /* Major No. */
#define	NODE_MINOR		0	 /* Minor No. */
#define NODE_PATHNAME 	"mmtun"  /* Device Name */

#define GPIO_SPI_CLKI	36
#define GPIO_SPI_DO		33
#define GPIO_SPI_DI		34
#define GPIO_SPI_CSI	35
#define GPIO_SPIC_XIRQ	26
#define GPIO_SPIS_XIRQ	27
#define GPIO_FRMLOCK	21
#define GPIO_XIRQ		124
#define GPIO_XRESET		126
#define GPIO_PWR_EN		130

#define POWER_ON		1
#define POWER_OFF		0

static uint32_t mb86a35_on_gpio_table[] = {
	GPIO_CFG(GPIO_PWR_EN, GPIOMUX_FUNC_GPIO,
		GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(GPIO_XRESET, GPIOMUX_FUNC_GPIO,
		GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(GPIO_XIRQ, GPIOMUX_FUNC_GPIO,
		GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
	GPIO_CFG(GPIO_FRMLOCK, GPIOMUX_FUNC_GPIO,
		GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(GPIO_SPIC_XIRQ, GPIOMUX_FUNC_GPIO,
		GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(GPIO_SPIS_XIRQ, GPIOMUX_FUNC_GPIO,
		GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(GPIO_SPI_DO, GPIOMUX_FUNC_1,
		GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(GPIO_SPI_DI, GPIOMUX_FUNC_1,
		GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(GPIO_SPI_CSI, GPIOMUX_FUNC_1,
		GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(GPIO_SPI_CLKI, GPIOMUX_FUNC_1,
		GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
};

static uint32_t mb86a35_off_gpio_table[] = {
	GPIO_CFG(GPIO_PWR_EN, GPIOMUX_FUNC_GPIO,
		GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	GPIO_CFG(GPIO_XRESET, GPIOMUX_FUNC_GPIO,
		GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	GPIO_CFG(GPIO_XIRQ, GPIOMUX_FUNC_GPIO,
		GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	GPIO_CFG(GPIO_FRMLOCK, GPIOMUX_FUNC_GPIO,
		GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	GPIO_CFG(GPIO_SPIC_XIRQ, GPIOMUX_FUNC_GPIO,
		GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	GPIO_CFG(GPIO_SPIS_XIRQ, GPIOMUX_FUNC_GPIO,
		GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	GPIO_CFG(GPIO_SPI_DO, GPIOMUX_FUNC_GPIO,
		GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	GPIO_CFG(GPIO_SPI_DI, GPIOMUX_FUNC_GPIO,
		GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	GPIO_CFG(GPIO_SPI_CSI, GPIOMUX_FUNC_GPIO,
		GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	GPIO_CFG(GPIO_SPI_CLKI, GPIOMUX_FUNC_GPIO,
		GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
};

extern int proc_init_module(void);
extern void proc_cleanup_module(void);
extern int mb86a35_open(struct inode *inode, struct file *filp);
extern int mb86a35_close(struct inode *inode, struct file *filp);
extern ssize_t mb86a35_read(struct file *filp, char *buf, size_t count, loff_t * f_pos);
extern int mb86a35_write(struct file *filp, const char __user * buf, size_t count, loff_t * f_pos);
extern long mb86a35_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
extern int mb86a35_mmap(struct file *filp, struct vm_area_struct *vma);
