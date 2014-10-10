#
# Makefile for the cypress-touchkey drivers.
#
ifdef CONFIG_TARGET_SERIES_DALI
obj-$(CONFIG_KEYPAD_CYPRESS_TOUCH)                += cypress-touchkey_dali.o
else ifdef CONFIG_KOR_MODEL_SHV_E160S
obj-$(CONFIG_KEYPAD_CYPRESS_TOUCH)                += cypress-touchkey_q1_skt.o
else ifdef CONFIG_KOR_MODEL_SHV_E160K
obj-$(CONFIG_KEYPAD_CYPRESS_TOUCH)                += cypress-touchkey_q1_skt.o
else
obj-$(CONFIG_KEYPAD_CYPRESS_TOUCH)                += cypress-touchkey.o
endif

obj-$(CONFIG_KEYPAD_CYPRESS_TOUCH)                += issp_driver_routines.o
obj-$(CONFIG_KEYPAD_CYPRESS_TOUCH)                += issp_main.o
obj-$(CONFIG_KEYPAD_CYPRESS_TOUCH)                += issp_routines.o
