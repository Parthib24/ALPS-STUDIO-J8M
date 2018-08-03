

#ccflags-y +=-I$(srctree)/drivers/spi/mediatek/mt6797/

#MODFLAGS       =$(MODFLAGS)  -DMODULE -fno-pic  

#sunwave-objs := sunwavecorp.o   platform.o misce.o

#ifdef CONFIG_SUNWAVECORP_MT6797
#sunwave-objs +=  mt6797_dts.o
#obj-$(CONFIG_SUNWAVE_FINGERPRINT)   = sunwave.o mt6797_dts_plat.o
#endif

ccflags-y += -I$(srctree)/drivers/misc/mediatek/hwmon/include
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include/mach/
ccflags-y += -I$(srctree)/drivers/spi/mediatek/mt6735

obj-y += sunwavecorp.o   platform.o misce.o mt6797_dts.o mt6797_dts_plat.o