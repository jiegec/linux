# SPDX-License-Identifier: GPL-2.0
dtb-$(CONFIG_SOC_ROCKET_CHIP_VCU128) += rocket-chip-vcu128-single-core.dtb
dtb-$(CONFIG_SOC_ROCKET_CHIP_VCU128) += rocket-chip-vcu128-dual-core.dtb
obj-$(CONFIG_BUILTIN_DTB) += $(addsuffix .o, $(dtb-y))

# Add padding to dts to allow U-Boot to add reserved memory region
DTC_FLAGS := -p 1024