#!/bin/bash

export BUILD_TARGET=AOSP
. sc03d.config

time ./_build-bootimg.sh $1
