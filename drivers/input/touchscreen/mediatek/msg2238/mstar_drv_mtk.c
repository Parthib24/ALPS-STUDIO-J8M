////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2014 MStar Semiconductor, Inc.
// All rights reserved.
//
// Unless otherwise stipulated in writing, any and all information contained
// herein regardless in any format shall remain the sole proprietary of
// MStar Semiconductor Inc. and be kept in strict confidence
// (??MStar Confidential Information??) by the recipient.
// Any unauthorized act including without limitation unauthorized disclosure,
// copying, use, reproduction, sale, distribution, modification, disassembling,
// reverse engineering and compiling of the contents of MStar Confidential
// Information is unlawful and strictly prohibited. MStar hereby reserves the
// rights to any and all damages, losses, costs and expenses resulting therefrom.
//
////////////////////////////////////////////////////////////////////////////////

/**
 *
 * @file    mstar_drv_mtk.c
 *
 * @brief   This file defines the interface of touch screen
 *
 *
 */

/*=============================================================*/
// INCLUDE FILE
/*=============================================================*/

#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/rtpm_prio.h>
#include <linux/wait.h>
#include <linux/time.h>
#include <linux/delay.h>

#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/namei.h>
#include <linux/vmalloc.h>

#include "tpd.h"
#include <linux/sysfs.h>
#include <linux/device.h>
#ifdef CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>
#include <linux/dma-mapping.h>
#include <linux/gpio.h>

#ifdef TIMER_DEBUG
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/module.h>
#endif //TIMER_DEBUG

#ifdef CONFIG_MTK_SENSOR_HUB_SUPPORT
#include <mach/md32_ipi.h>
#include <mach/md32_helper.h>
#endif //CONFIG_MTK_SENSOR_HUB_SUPPORT

#ifdef CONFIG_ENABLE_REGULATOR_POWER_ON
#include <linux/regulator/consumer.h>
#endif //CONFIG_ENABLE_REGULATOR_POWER_ON
#endif //CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD

#include "mstar_drv_platform_interface.h"
#include "mstar_drv_self_fw_control.h"
/*=============================================================*/
// CONSTANT VALUE DEFINITION
/*=============================================================*/

//#define MSG_TP_IC_NAME "msg2xxx" //"msg21xxA" or "msg22xx" or "msg26xxM" or "msg28xx" /* Please define the mstar touch ic name based on the mutual-capacitive ic or self capacitive ic that you are using */
#define I2C_BUS_ID   (1)       // i2c bus id : 0 or 1

#define TPD_OK (0)

/*=============================================================*/
// EXTERN VARIABLE DECLARATION
/*=============================================================*/

#ifdef CONFIG_TP_HAVE_KEY
extern int g_TpVirtualKey[];

#ifdef CONFIG_ENABLE_REPORT_KEY_WITH_COORDINATE
extern int g_TpVirtualKeyDimLocal[][4];
#endif //CONFIG_ENABLE_REPORT_KEY_WITH_COORDINATE
#endif //CONFIG_TP_HAVE_KEY

extern struct tpd_device *tpd;
extern int fix_tp_proc_info(void *tp_data, unsigned char data_len);
/*=============================================================*/
// LOCAL VARIABLE DEFINITION
/*=============================================================*/

/*
#if (defined(TPD_WARP_START) && defined(TPD_WARP_END))
static int tpd_wb_start_local[TPD_WARP_CNT] = TPD_WARP_START;
static int tpd_wb_end_local[TPD_WARP_CNT] = TPD_WARP_END;
#endif

#if (defined(TPD_HAVE_CALIBRATION) && !defined(TPD_CUSTOM_CALIBRATION))
static int tpd_calmat_local[8] = TPD_CALIBRATION_MATRIX;
static int tpd_def_calmat_local[8] = TPD_CALIBRATION_MATRIX;
#endif
*/
struct i2c_client *g_I2cClient = NULL;

//static int boot_mode = 0;

#ifdef CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
#ifdef CONFIG_ENABLE_REGULATOR_POWER_ON
struct regulator *g_ReguVdd = NULL;
#endif //CONFIG_ENABLE_REGULATOR_POWER_ON
#endif //CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
int showflag = 1;

/*=============================================================*/
// FUNCTION DECLARATION
/*=============================================================*/

/*=============================================================*/
// FUNCTION DEFINITION
/*=============================================================*/
static struct class *mstar_class;
bool TP_gesture_Switch = false;
static ssize_t enable_mstar_show(struct class *dev, struct class_attribute *
attr, char *buf)
{
	return sprintf(buf, "%s:%d\n", "gesenable",TP_gesture_Switch);
}
static ssize_t enable_mstar_store(struct class *class, struct class_attribute 
*attr, const char *buf, size_t count)
{
	unsigned long state;
	ssize_t ret = -EINVAL;
	ret = kstrtoul(&buf[0], 10, &state);
	if(ret<0)
	{
			return -ENOBUFS;
	}
	else
	{
		if(state==true)
		{
			TP_gesture_Switch=true;				
		}
		else	
		{
			TP_gesture_Switch=false; 
		}						
	}	
	return count;
}
static CLASS_ATTR(gesenable, 00774, enable_mstar_show, enable_mstar_store);

/* probe function is used for matching and initializing input device */
static int /*__devinit*/ tpd_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int ret = 0;
#ifdef CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
#ifdef CONFIG_ENABLE_REGULATOR_POWER_ON
    //const char *vdd_name = "vdd";
   const char *vdd_name = "vgp1";
//    const char *vcc_i2c_name = "vcc_i2c";
#endif //CONFIG_ENABLE_REGULATOR_POWER_ON
#endif //CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD

    TPD_DMESG("TPD probe\n");   
    
    if (client == NULL)
    {
        TPD_DMESG("i2c client is NULL\n");
        return -1;
    }
    if (client != NULL)
    {
        TPD_DMESG("i2c client is OK\n");
     
    }
    g_I2cClient = client;
    
    MsDrvInterfaceTouchDeviceSetIicDataRate(g_I2cClient, 100000); // 100 KHz
     
#ifdef CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
#ifdef CONFIG_ENABLE_REGULATOR_POWER_ON
    //tpd->reg = regulator_get(tpd->tpd_dev, "vtouch");
	//ret = regulator_set_voltage(tpd->reg, 2800000, 2800000);	/*set 2.8v*/
   g_ReguVdd = regulator_get(&g_I2cClient->dev, vdd_name);
    
   tpd->reg = g_ReguVdd;
    
    ret = regulator_set_voltage(tpd->reg, 2800000, 2800000); 
    if (ret)
    {
        DBG("Could not set to 2800mv.\n");
    }
#endif //CONFIG_ENABLE_REGULATOR_POWER_ON
#endif //CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD

    ret = MsDrvInterfaceTouchDeviceProbe(g_I2cClient, id);
    if (ret == 0) // If probe is success, then enable the below flag.
    {
        tpd_load_status = 1;
    }    
	unsigned short pMajor=0;
	unsigned short pMinor=0;
	unsigned short fmver=0;
	unsigned char *ppVersion=NULL;
	unsigned char tp_info[512];
	int len,err;
	mstar_class = class_create(THIS_MODULE, "syna");
	err = class_create_file(mstar_class, &class_attr_gesenable);
	DrvFwCtrlGetCustomerFirmwareVersion(&pMajor, &pMinor, &ppVersion);
if(showflag == 1)
{
	if(pMajor==0x2){
		len=sprintf(tp_info,"TP IC:%s,TP MODULE:%s,TP I2C ADR:0x%x,SW FirmWare:%x.%x,Sample FirmWare:%x.%x","MSG2238","DIJIN",client->addr,pMajor,pMinor,SAMPLE_FWVER_DJ1_MAJOR,SAMPLE_FWVER_DJ1_MINOR);
	}
	else if(pMajor==0x01){
	len=sprintf(tp_info,"TP IC:%s,TP MODULE:%s,TP I2C ADR:0x%x,SW FirmWare:%x.%x,Sample FirmWare:%x.%x","MSG2238","HELITAI",client->addr,pMajor,pMinor,SAMPLE_FWVER_HELITAI_MAJOR,SAMPLE_FWVER_HELITAI_MINOR);
	}
	else if(pMajor==0x03){
	len=sprintf(tp_info,"TP IC:%s,TP MODULE:%s,TP I2C ADR:0x%x,SW FirmWare:%x.%x,Sample FirmWare:%x.%x","MSG2238","DIJIN",client->addr,pMajor,pMinor,SAMPLE_FWVER_DJ2_MAJOR,SAMPLE_FWVER_DJ2_MINOR);
	}
	else{
	len=sprintf(tp_info,"TP IC:%s,TP MODULE:%s,TP I2C ADR:0x%x,SW FirmWare:%x.%x,Sample FirmWare:%x.%x","UNKOWN","UNKOWN",NULL,0,0,0,0);
	}


	//len = sprintf(tp_info, "TP IC :%s,TP module :%s,TP I2C adr : 0x%x",  "MSG2238",module_name, client->addr);
//	len = sprintf(tp_info, "TP IC :%s",  "MSG2238");
	fix_tp_proc_info(tp_info, len);	
}

    TPD_DMESG("TPD probe done\n");
    showflag = 0;
    return TPD_OK;   
}

static int tpd_detect(struct i2c_client *client, struct i2c_board_info *info) 
{
    strcpy(info->type, TPD_DEVICE);    
//    strcpy(info->type, MSG_TP_IC_NAME);
    
    return TPD_OK;
}

static int /*__devexit*/ tpd_remove(struct i2c_client *client)
{   
    TPD_DEBUG("TPD removed\n");
    
    MsDrvInterfaceTouchDeviceRemove(client);
    
    return TPD_OK;
}


/* The I2C device list is used for matching I2C device and I2C device driver. */
static const struct i2c_device_id tpd_device_id[] =
{
    {"msg2xxx", 0},
    {}, /* should not omitted */ 
};

MODULE_DEVICE_TABLE(i2c, tpd_device_id);

#ifdef CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
const struct of_device_id touch_dt_match_table[] = {
    { .compatible = "mediatek,cap_touch",},
    {},
};

MODULE_DEVICE_TABLE(of, touch_dt_match_table);

static struct device_attribute *msg2xxx_attrs[] = {
#ifdef CONFIG_MTK_SENSOR_HUB_SUPPORT
	&dev_attr_tpd_scp_ctrl,
#endif //CONFIG_MTK_SENSOR_HUB_SUPPORT
};

#else
static struct i2c_board_info __initdata i2c_tpd = {I2C_BOARD_INFO("msg2xxx", (0x4C>>1))};
#endif //CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD

static struct i2c_driver tpd_i2c_driver = {

#ifdef CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
    .driver = {
        .name = "msg2xxx",
        .of_match_table = of_match_ptr(touch_dt_match_table),
    },
#else
    .driver = {
        .name = "msg2xxx",
    },
#endif //CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
    .probe = tpd_probe,
    .remove = tpd_remove,
    .id_table = tpd_device_id,
    .detect = tpd_detect,
};

static int tpd_local_init(void)
{  
   // TPD_DMESG("TPD init device driver (Built %s @ %s)\n", __DATE__, __TIME__);
/*
    // Software reset mode will be treated as normal boot
    boot_mode = get_boot_mode();
    if (boot_mode == 3) 
    {
        boot_mode = NORMAL_BOOT;    
    }
*/
    if (i2c_add_driver(&tpd_i2c_driver) != 0)
    {
        TPD_DMESG("unable to add i2c driver.\n");
         
        return -1;
    }
    
    if (tpd_load_status == 0) 
    {
        TPD_DMESG("add error touch panel driver.\n");

        //i2c_del_driver(&tpd_i2c_driver);
        return -1;
    }

#ifdef CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
    if (tpd_dts_data.use_tpd_button)
    {
        tpd_button_setting(tpd_dts_data.tpd_key_num, tpd_dts_data.tpd_key_local,
        tpd_dts_data.tpd_key_dim_local);
    }
#else
#ifdef CONFIG_TP_HAVE_KEY
#ifdef CONFIG_ENABLE_REPORT_KEY_WITH_COORDINATE     
    tpd_button_setting(4, g_TpVirtualKey, g_TpVirtualKeyDimLocal); //MAX_KEY_NUM
#endif //CONFIG_ENABLE_REPORT_KEY_WITH_COORDINATE  
#endif //CONFIG_TP_HAVE_KEY  
#endif //CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
/*
#if (defined(TPD_WARP_START) && defined(TPD_WARP_END))    
    TPD_DO_WARP = 1;
    memcpy(tpd_wb_start, tpd_wb_start_local, TPD_WARP_CNT*4);
    memcpy(tpd_wb_end, tpd_wb_start_local, TPD_WARP_CNT*4);
#endif 

#if (defined(TPD_HAVE_CALIBRATION) && !defined(TPD_CUSTOM_CALIBRATION))
    memcpy(tpd_calmat, tpd_def_calmat_local, 8*4);
    memcpy(tpd_def_calmat, tpd_def_calmat_local, 8*4);    
#endif  
*/
    TPD_DMESG("TPD init done %s, %d\n", __func__, __LINE__);  
//    tpd_type_cap = 1;
        
    return TPD_OK; 
}

#ifdef CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
static void tpd_resume(struct device *h)
#else
static void tpd_resume(struct early_suspend *h)
#endif //CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
{
    TPD_DMESG("TPD wake up\n");
    
    MsDrvInterfaceTouchDeviceResume(h);
    
    TPD_DMESG("TPD wake up done\n");
}

#ifdef CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
static void tpd_suspend(struct device *h)
#else
static void tpd_suspend(struct early_suspend *h)
#endif //CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
{
    TPD_DMESG("TPD enter sleep\n");

    MsDrvInterfaceTouchDeviceSuspend(h);

    TPD_DMESG("TPD enter sleep done\n");
} 

static struct tpd_driver_t tpd_device_driver = {
    .tpd_device_name = "msg2xxx",
    .tpd_local_init = tpd_local_init,
    .suspend = tpd_suspend,
    .resume = tpd_resume,
#ifdef CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
    .attrs = {
        .attr = msg2xxx_attrs,
        .num  = ARRAY_SIZE(msg2xxx_attrs),
    },
#else
#ifdef CONFIG_TP_HAVE_KEY
#ifdef CONFIG_ENABLE_REPORT_KEY_WITH_COORDINATE
     .tpd_have_button = 1,
#else
     .tpd_have_button = 0,
#endif //CONFIG_ENABLE_REPORT_KEY_WITH_COORDINATE        
#endif //CONFIG_TP_HAVE_KEY        
#endif //CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
};

static int __init tpd_driver_init(void) 
{
    TPD_DMESG("MStar touch panel driver init\n");

#ifdef CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
    tpd_get_dts_info();
#else
    i2c_register_board_info(I2C_BUS_ID, &i2c_tpd, 1);
#endif //CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
    if (tpd_driver_add(&tpd_device_driver) < 0)
    {
        TPD_DMESG("TPD add MStar TP driver failed\n");
    }
     
    return 0;
}
 
static void __exit tpd_driver_exit(void) 
{
    TPD_DMESG("touch panel driver exit\n");
    
    tpd_driver_remove(&tpd_device_driver);
}

module_init(tpd_driver_init);
module_exit(tpd_driver_exit);
MODULE_LICENSE("GPL");