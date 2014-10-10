subdir-ccflags-$(CONFIG_ARCH_MSM) += -Iarch/arm/mach-msm

-include $(src)/Makefile.board

obj-$(CONFIG_BROADCOM_WIFI) += broadcom_wlan.o
obj-$(CONFIG_BT_BCM4330) += board-bluetooth-bcm4330.o
obj-$(CONFIG_USB_SWITCH_FSA9480)   += sec_switch.o
obj-$(CONFIG_SEC_DEBUG)	+= sec_debug.o sec_getlog.o sec_gaf.o
obj-$(CONFIG_SEC_LOG_LAST_KMSG) += sec_last_log.o

