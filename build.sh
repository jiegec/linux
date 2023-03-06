#!/bin/sh
set -e
make ARCH=riscv CROSS_COMPILE=riscv64-linux-gnu- -j8 rocket-chip-vcu128_defconfig
make ARCH=riscv CROSS_COMPILE=riscv64-linux-gnu- -j8 all
cd arch/riscv/boot
mkimage -f image.its image.itb
ls -alh ../../../vmlinux