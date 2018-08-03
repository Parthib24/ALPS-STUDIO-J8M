MTK_PLATFORM := $(subst ",,$(CONFIG_MTK_PLATFORM))
ccflags-y += -I$(srctree)/drivers/spi/mediatek/$(MTK_PLATFORM)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include

ccflags-$(CONFIG_FINGERPRINT_USING_EXT_LDO) += -DFINGERPRINT_USING_EXT_LDO
ccflags-$(CONFIG_FINGERPRINT_GX368_USING_32_BIT) += -DFINGERPRINT_GX368_USING_32_BIT
ccflags-$(CONFIG_FINGERPRINT_VGP1_POWER) += -DFINGERPRINT_VGP1_POWER
obj-y += gf_spi.o platform.o gf_common.o
