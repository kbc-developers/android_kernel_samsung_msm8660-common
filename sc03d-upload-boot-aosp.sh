#!/bin/bash

BINDIR=./out/SC03D/AOSP/bin
MMCBLK_BOOT=/dev/block/mmcblk0p8

NOSU=$1

if [ "$NOSU" = "nosu" ]; then
  adb push $BINDIR/boot.img /tmp/boot.img
#  adb push $BINDIR/boot.img /data/local/tmp/boot.img
  adb shell "cat /tmp/boot.img > $MMCBLK_BOOT"
#  adb shell "cat /data/local/tmp/boot.img > $MMCBLK_BOOT"
  adb shell "rm /tmp/boot.img"
  adb shell "sync;sync;sync;sleep 2; reboot"
else
  adb push $BINDIR/boot.img /data/local/tmp/boot.img
  adb shell su -c "cat /data/local/tmp/boot.img > $MMCBLK_BOOT"
  adb shell su -c "rm /data/local/tmp/boot.img"
  adb shell su -c "sync;sync;sync;sleep 2; reboot"
fi
