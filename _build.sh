#!/bin/bash

KERNEL_DIR=$PWD

BUILD_DEVICE=$1
BUILD_TARGET=$2
BUILD_SELECT=$3

cpoy_initramfs()
{
  if [ -d $INITRAMFS_TMP_DIR ]; then
    rm -rf $INITRAMFS_TMP_DIR  
  fi
  cp -a $INITRAMFS_SRC_DIR $(dirname $INITRAMFS_TMP_DIR)
  rm -rf $INITRAMFS_TMP_DIR/.git
  find $INITRAMFS_TMP_DIR -name .gitignore | xargs rm
}

if [ ! -n "$BUILD_DEVICE" ]; then
  echo ""
  read -p "select device? [SC05D/SC03D] " BUILD_DEVICE
fi

# device setting
if [ "$BUILD_DEVICE" = 'SC05D' ]; then
  AOSP_DEFCONFIG=kbc_sc05d_aosp_defconfig
  SAM_DEFCONFIG=kbc_sc05d_samsung_defconfig
  RECO_DEFCONFIG=kbc_sc05d_recovery_defconfig
  MULTI_DEFCONFIG=kbc_sc05d_multi_defconfig
  MOD_VERSION=sc05d_mod_version
  BOOT_RAMDISK_NAME=sc05d_boot_ramdisk
  RECOVERY_RAMDISK_NAME=sc05d_recovery_ramdisk
  KERNEL_BASE_ADDRESS=0x48000000
  KERNEL_RAMDISK_ADDRESS=0x49400000

elif [ "$BUILD_DEVICE" = 'SC03D' ]; then
  AOSP_DEFCONFIG=kbc_sc03d_aosp_defconfig
  SAM_DEFCONFIG=kbc_sc03d_samsung_defconfig
  RECO_DEFCONFIG=kbc_sc03d_recovery_defconfig
  MULTI_DEFCONFIG=kbc_sc03d_multi_defconfig
  MOD_VERSION=sc03d_mod_version
  BOOT_RAMDISK_NAME=sc03d_boot_ramdisk
  RECOVERY_RAMDISK_NAME=sc03d_recovery_ramdisk
  KERNEL_BASE_ADDRESS=0x40400000
  KERNEL_RAMDISK_ADDRESS=0x41800000

else
  echo "error: not found BUILD_DEVICE"
  exit -1
fi

if [ ! -n "$BUILD_TARGET" ]; then
  echo "error: not found BUILD_TARGET"
  exit -1
fi

# check target
case "$BUILD_TARGET" in
  "AOSP" ) BUILD_DEFCONFIG=$AOSP_DEFCONFIG ;;
  "SAM" ) BUILD_DEFCONFIG=$SAM_DEFCONFIG ;;
  "RECO" ) BUILD_DEFCONFIG=$RECO_DEFCONFIG ;;
  "MULTI" ) BUILD_DEFCONFIG=$MULTI_DEFCONFIG ;;
  * ) echo "error: not found BUILD_TARGET" && exit -1 ;;
esac

BIN_DIR=out/$BUILD_TARGET/bin
OBJ_DIR=out/$BUILD_TARGET/obj
mkdir -p $BIN_DIR
mkdir -p $OBJ_DIR

# generate LOCALVERSION
. $MOD_VERSION

# check and get compiler
. cross_compile

# set build env
export ARCH=arm
export CROSS_COMPILE=$BUILD_CROSS_COMPILE
export LOCALVERSION="-$BUILD_LOCALVERSION"

echo "=====> BUILD START $BUILD_KERNELVERSION-$BUILD_LOCALVERSION"

if [ ! -n "$3" ]; then
  echo ""
  read -p "select build? [(a)ll/(u)pdate/(i)mage default:update] " BUILD_SELECT
else
  BUILD_SELECT=$3
fi

# copy initramfs
if [ "$BUILD_TARGET" = 'RECO' ]; then
  INITRAMFS_SRC_DIR=../$RECOVERY_RAMDISK_NAME
  INITRAMFS_TMP_DIR=/tmp/$RECOVERY_RAMDISK_NAME
  IMAGE_NAME=recovery
else
  INITRAMFS_SRC_DIR=../$BOOT_RAMDISK_NAME
  INITRAMFS_TMP_DIR=/tmp/$BOOT_RAMDISK_NAME
  IMAGE_NAME=boot
fi
echo ""
echo "=====> copy initramfs"
cpoy_initramfs


# make start
if [ "$BUILD_SELECT" = 'all' -o "$BUILD_SELECT" = 'a' ]; then
  echo ""
  echo "=====> cleaning"
  make clean
  cp -f ./arch/arm/configs/$BUILD_DEFCONFIG $OBJ_DIR/.config
  make -C $PWD O=$OBJ_DIR oldconfig || exit -1
fi

if [ "$BUILD_SELECT" != 'image' -a "$BUILD_SELECT" != 'i' ]; then
  echo ""
  echo "=====> build start"
  if [ -e make.log ]; then
    mv make.log make_old.log
  fi
  nice -n 10 make O=$OBJ_DIR -j12 2>&1 | tee make.log
fi

# check compile error
COMPILE_ERROR=`grep 'error:' ./make.log`
if [ "$COMPILE_ERROR" ]; then
  echo ""
  echo "=====> ERROR"
  grep 'error:' ./make.log
  exit -1
fi

# *.ko replace
find -name '*.ko' -exec cp -av {} $INITRAMFS_TMP_DIR/lib/modules/ \;

echo ""
echo "=====> CREATE RELEASE IMAGE"
# clean release dir
if [ `find $BIN_DIR -type f | wc -l` -gt 0 ]; then
  rm -rf $BIN_DIR/*
fi
mkdir -p $BIN_DIR

# copy zImage
cp $OBJ_DIR/arch/arm/boot/zImage $BIN_DIR/kernel
echo "----- Making uncompressed $IMAGE_NAME ramdisk ------"
./release-tools/mkbootfs $INITRAMFS_TMP_DIR > $BIN_DIR/ramdisk-$IMAGE_NAME.cpio
echo "----- Making $IMAGE_NAME ramdisk ------"
./release-tools/minigzip < $BIN_DIR/ramdisk-$IMAGE_NAME.cpio > $BIN_DIR/ramdisk-$IMAGE_NAME.img
echo "----- Making $IMAGE_NAME image ------"
./release-tools/mkbootimg --cmdline "androidboot.hardware=qcom user_debug=31 zcache" --kernel $BIN_DIR/kernel  --ramdisk $BIN_DIR/ramdisk-$IMAGE_NAME.img --base $KERNEL_BASE_ADDRESS --pagesize 2048 --ramdiskaddr $KERNEL_RAMDISK_ADDRESS --output $BIN_DIR/$IMAGE_NAME.img

# create odin image
cd $BIN_DIR
tar cf $BUILD_LOCALVERSION-$IMAGE_NAME-odin.tar $IMAGE_NAME.img
md5sum -t $BUILD_LOCALVERSION-$IMAGE_NAME-odin.tar >> $BUILD_LOCALVERSION-$IMAGE_NAME-odin.tar
mv $BUILD_LOCALVERSION-$IMAGE_NAME-odin.tar $BUILD_LOCALVERSION-$IMAGE_NAME-odin.tar.md5
echo "  $BIN_DIR/$BUILD_LOCALVERSION-$IMAGE_NAME-odin.tar.md5"

# create cwm image
if [ -d tmp ]; then
  rm -rf tmp
fi
mkdir -p ./tmp/META-INF/com/google/android
cp $IMAGE_NAME.img ./tmp/
cp $KERNEL_DIR/release-tools/update-binary ./tmp/META-INF/com/google/android/
sed -e "s/@VERSION/$BUILD_LOCALVERSION/g" $KERNEL_DIR/release-tools/$BUILD_DEVICE/updater-script-$IMAGE_NAME.sed > ./tmp/META-INF/com/google/android/updater-script
cd tmp && zip -rq ../cwm.zip ./* && cd ../
SIGNAPK_DIR=$KERNEL_DIR/release-tools/signapk
java -jar $SIGNAPK_DIR/signapk.jar $SIGNAPK_DIR/testkey.x509.pem $SIGNAPK_DIR/testkey.pk8 cwm.zip $BUILD_LOCALVERSION-$IMAGE_NAME-signed.zip
rm cwm.zip
rm -rf tmp
echo "  $BIN_DIR/$BUILD_LOCALVERSION-$IMAGE_NAME-signed.zip"

cd $KERNEL_DIR
echo ""
echo "=====> BUILD COMPLETE $BUILD_KERNELVERSION-$BUILD_LOCALVERSION"
exit 0
