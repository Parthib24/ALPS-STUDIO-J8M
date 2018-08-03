ccflags-y += -I$(srctree)/drivers/misc/mediatek/hwmon/include
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include/mach/
ccflags-y += -I$(srctree)/drivers/spi/mediatek/mt6735
#ccflags-y += -D$(shell echo $(ARCH_MTK_PROJECT) | tr 'a-z' 'A-Z')

ccflags-$(CONFIG_FINGERPRINT_USING_EXT_LDO) += -DFINGERPRINT_USING_EXT_LDO
ccflags-$(CONFIG_FINGERPRINT_GX368_USING_32_BIT) += -DFINGERPRINT_GX368_USING_32_BIT

obj-y	+= gf-milanf.o  gf-common.o  

