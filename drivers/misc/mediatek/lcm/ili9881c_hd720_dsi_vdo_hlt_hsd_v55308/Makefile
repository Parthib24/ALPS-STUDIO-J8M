#
# Makefile for misc devices that really don't fit anywhere else.
#
#include $(srctree)/drivers/misc/mediatek/Makefile.custom
ccflags-y += -D$(shell echo $(CONFIG_ARCH_MTK_PROJECT)|tr 'a-z' 'A-Z')
obj-y += ili9881c_hd720_dsi_vdo_hlt_hsd_v55308.o
ccflags-$(CONFIG_MTK_LCM) += -I$(srctree)/drivers/misc/mediatek/lcm/inc



