ccflags-y += -I$(srctree)/drivers/misc/mediatek/hwmon/include
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include/mach/
ccflags-y += -I$(srctree)/drivers/spi/mediatek/mt6735

ccflags-$(CONFIG_FINGERPRINT_USING_EXT_LDO) += -DFINGERPRINT_USING_EXT_LDO
ccflags-$(CONFIG_FINGERPRINT_VGP1_POWER) += -DFINGERPRINT_VGP1_POWER
ccflags-$(CONFIG_FINGERPRINT_FPS998) += -DCONFIG_FINGERPRINT_FPS998

obj-y += cdfinger-mtk.o