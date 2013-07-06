#!/bin/bash

export BUILD_TARGET=AOSP
. sc05d.config

time ./_build-bootimg.sh $1
