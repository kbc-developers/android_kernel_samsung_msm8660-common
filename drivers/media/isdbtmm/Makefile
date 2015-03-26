#
# Makefile for the ISDB-Tmm tuner device drivers.
#

##	radio-mb86a35-objs	:= radio-mb86a35.o
##obj-m += radio-mb86a35s.o
##	CONFIG_RADIO_MB86A35=y
#
##	obj-$(CONFIG_RADIO_MB86A35) += radio-mb86a35.o
#
## Compile only the target include ISDB-Tmm configure ##
ifeq ($(CONFIG_ISDBTMM),y)
obj-$(CONFIG_ISDBTMM)+=radio-mb86a35s-dev.o \
					radio-mb86a35s.o

ifeq ($(CONFIG_DEBUG_DRIVER),y)
EXTRA_CFLAGS += -DDEBUG
endif
#EXTRA_CFLAGS += -DKERNEL_2635
EXTRA_CFLAGS += -DKERNEL_2638
#EXTRA_CFLAGS += -DFPGA
#
ifeq ($(ARCH),i386)
EXTRA_CFLAGS += -DDEBUG_I386
endif
endif
