#!/bin/bash

export BUILD_TARGET=AOSP
. sc01e.config

time ./_build-bootimg.sh $1
