#!/bin/bash
#export CHIPSET_VENDOR=qcom
#export ENABLE_TAGS_ERROR=01

# Set Default Path
TOP_DIR=$PWD
#KERNEL_PATH=$TOP_DIR/kernel

export ARCH=arm
export CROSS_COMPILE=arm-eabi-
cd ..
CURRENT_DIR=$PWD
cd -
export CROSS_COMPILE_PATH="/opt/toolchains/arm-eabi-4.4.3/bin"
export PATH=$CROSS_COMPILE_PATH:$PATH
KERNEL_DIR=.
export KERNEL_DIR
echo "PTAH print"
echo $PATH

# TODO: Set toolchain and root filesystem path
#TOOLCHAIN="$TOP_DIR/toolchain/arm-eabi-4.4.3/bin/arm-eabi-"
#ROOTFS_PATH="$TOP_DIR/root"

# Copy Kernel Configuration File
#cp -f $KERNEL_PATH/arch/arm/configs/c1_defconfig $KERNEL_PATH/.config

#make -C $KERNEL_PATH oldconfig || exit -1
#make -C $KERNEL_PATH ARCH=arm CROSS_COMPILE=$TOOLCHAIN CONFIG_INITRAMFS_SOURCE="$ROOTFS_PATH" || exit -1


mkdir -p ./obj/KERNEL_OBJ/

#make O=$SEC_TARGET_DIR/obj/KERNEL_OBJ/ ARCH=arm CROSS_COMPILE=arm-eabi- "msm8660"$TARGET_REGON_OPERATOR"_defconfig"
#make O=$SEC_TARGET_DIR/obj/KERNEL_OBJ/ ARCH=arm CROSS_COMPILE=arm-eabi- headers_install
#make -j$SEC_CPU_JOB_NUM O=$SEC_TARGET_DIR/obj/KERNEL_OBJ/ ARCH=arm CROSS_COMPILE=arm-eabi-

make O=./obj/KERNEL_OBJ/ msm8660-perf_Q1_JPN_NTT_defconfig
make O=./obj/KERNEL_OBJ/ headers_install
make -j15 O=./obj/KERNEL_OBJ/ 

# Copy Kernel Image
cp -f ./obj/KERNEL_OBJ/arch/arm/boot/zImage .
