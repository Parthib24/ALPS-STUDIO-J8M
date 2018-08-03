# Linux driver folder

LCM_DEFINES := $(shell echo $(CONFIG_CUSTOM_KERNEL_LCM) | tr a-z A-Z)
DEFINES += $(foreach LCM,$(LCM_DEFINES),$(LCM))
ccflags-y += $(addprefix -D, $(DEFINES))

ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/msg5846/
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include/
ccflags-y += -I$(srctree)/drivers/watchdog/mediatek/
ccflags-y += -I$(srctree)/drivers/watchdog/mediatek/wdt/
ccflags-y += -I$(srctree)/drivers/watchdog/mediatek/wdt/$(MTK_PLATFORM)/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/i2c/$(MTK_PLATFORM)/
#calm ==start; touchscreen fireware files be separated from driver files to project directory
#MTK_TARGET_PROJECT := $(subst ",,$(CONFIG_ARCH_MTK_PROJECT))
#ccflags-y += -I$(srctree)/drivers/misc/mediatek/mach/$(MTK_PLATFORM)/$(MTK_TARGET_PROJECT)/touchscreen/msg2238/fireware
#calm ==end;

obj-y +=  mstar_drv_mtk.o
obj-y +=  mstar_drv_common.o
obj-y +=  mstar_drv_fw_control.o
obj-y +=  mstar_drv_hotknot.o
obj-y +=  mstar_drv_hotknot_queue.o
obj-y +=  mstar_drv_ic_fw_porting_layer.o
obj-y +=  mstar_drv_jni_interface.o
obj-y +=  mstar_drv_main.o
obj-y +=  mstar_drv_mp_test.o
obj-y +=  mstar_drv_platform_interface.o
obj-y +=  mstar_drv_platform_porting_layer.o
obj-y +=  mstar_drv_utility_adaption.o

