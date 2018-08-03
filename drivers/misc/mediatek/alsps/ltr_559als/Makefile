ccflags-y += -I$(srctree)/drivers/misc/mediatek/alsps/inc
ccflags-y += -I$(srctree)/drivers/misc/mediatek/hwmon/include

ifeq ($(CONFIG_LTR559_PS_MEAS_RATE_LOW),y)
	ccflags-y += -DLTR559_PS_MEAS_RATE_LOW
endif

obj-y	:=  ltr559.o

