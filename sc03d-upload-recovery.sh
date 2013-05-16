#!/bin/bash

BINDIR=./out/SC03D/RECO/bin
MMCBLK_RECOVERY=/dev/block/mmcblk0p22

NOSU=$1

if [ "$NOSU" = "nosu" ]; then
  adb push $BINDIR/recovery.img /tmp/recovery.img
#  adb push $BINDIR/recovery.img /data/local/tmp/recovery.img
  adb shell "cat /tmp/recovery.img > $MMCBLK_RECOVERY"
#  adb shell "cat /data/local/tmp/recovery.img > $MMCBLK_RECOVERY"
  adb shell "rm /tmp/recovery.img"
  adb shell "sync;sync;sync;sleep 2; reboot recovery"
else
  adb push $BINDIR/recovery.img /data/local/tmp/recovery.img
  adb shell su -c "cat /data/local/tmp/recovery.img > $MMCBLK_RECOVERY"
  adb shell su -c "rm /data/local/tmp/recovery.img"
  adb shell su -c "sync;sync;sync;sleep 2; reboot recovery"
fi
