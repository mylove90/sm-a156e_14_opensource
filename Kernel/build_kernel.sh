#!/bin/bash

cd kernel-5.15
python2 scripts/gen_build_config.py --kernel-defconfig a15x_defconfig -m user -o ../out/target/product/a15x/obj/KERNEL_OBJ/build.config

export ARCH=arm64
export PLATFORM_VERSION=13
export CROSS_COMPILE="aarch64-linux-gnu-"
export CROSS_COMPILE_COMPAT="arm-linux-gnueabi-"
export OUT_DIR="../out/target/product/a15x/obj/KERNEL_OBJ"
export DIST_DIR="../out/target/product/a15x/obj/KERNEL_OBJ"
export BUILD_CONFIG="../out/target/product/a15x/obj/KERNEL_OBJ/build.config"

cd ../kernel
./build/build.sh DO_NOT_STRIP_MODULES=0
