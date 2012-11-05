#!/bin/bash

export ARCH=arm
export CROSS_COMPILE=/opt/toolchains/arm-eabi-4.4.3/bin/arm-eabi-

make msm8660-perf_CELOX_JPN_NTT_defconfig
make
