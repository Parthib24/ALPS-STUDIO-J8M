#include <linux/kernel.h> //constant xx
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/wait.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/poll.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/errno.h>
#include <linux/time.h>
#include "kd_flashlight.h"
#include <asm/io.h>
#include <asm/uaccess.h>
#include "kd_camera_typedef.h"
//#include "kd_camera_hw.h"
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/version.h>
#include <mach/upmu_sw.h>
#include <mach/upmu_hw.h>
#include <linux/mutex.h>
#include <linux/i2c.h>
#include <linux/leds.h>
#include <linux/proc_fs.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/gpio.h>

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
#include <linux/mutex.h>
#else
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27)
#include <linux/semaphore.h>
#else
#include <asm/semaphore.h>
#endif
#endif

#ifdef CONFIG_COMPAT
#include <linux/fs.h>
#include <linux/compat.h>
#endif


/******************************************************************************
 * Debug configuration
******************************************************************************/
// availible parameter
// ANDROID_LOG_ASSERT
// ANDROID_LOG_ERROR
// ANDROID_LOG_WARNING
// ANDROID_LOG_INFO
// ANDROID_LOG_DEBUG
// ANDROID_LOG_VERBOSE
#define TAG_NAME "leds_strobe.c"
#define PK_DBG_NONE(fmt, arg...)    do {} while (0)
#define PK_DBG_FUNC(fmt, arg...)    pr_debug(TAG_NAME "%s: " fmt, __FUNCTION__ ,##arg)
#define PK_WARN(fmt, arg...)        pr_warning(TAG_NAME "%s: " fmt, __FUNCTION__ ,##arg)
#define PK_NOTICE(fmt, arg...)      pr_notice(TAG_NAME "%s: " fmt, __FUNCTION__ ,##arg)
#define PK_INFO(fmt, arg...)        pr_info(TAG_NAME "%s: " fmt, __FUNCTION__ ,##arg)
#define PK_TRC_FUNC(f)              pr_debug(TAG_NAME "<%s>\n", __FUNCTION__)
#define PK_TRC_VERBOSE(fmt, arg...) pr_debug(TAG_NAME fmt, ##arg)
#define PK_ERROR(fmt, arg...)       pr_err(TAG_NAME "%s: " fmt, __FUNCTION__ ,##arg)


#define DEBUG_LEDS_STROBE
#ifdef  DEBUG_LEDS_STROBE
	#define PK_DBG PK_DBG_FUNC
	#define PK_VER PK_TRC_VERBOSE
	#define PK_ERR PK_ERROR
#else
	#define PK_DBG(a,...)
	#define PK_VER(a,...)
	#define PK_ERR(a,...)
#endif

/******************************************************************************
 * local variables
******************************************************************************/

static DEFINE_SPINLOCK(g_strobeSMPLock); /* cotta-- SMP proection */


static u32 strobe_Res = 0;
static u32 strobe_Timeus = 0;
static BOOL g_strobe_On = 0;

static int g_duty=-1;
//static int gg_duty=-1;
static int g_timeOutTimeMs=0;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
static DEFINE_MUTEX(g_strobeSem);
#else
static DECLARE_MUTEX(g_strobeSem);
#endif


#define STROBE_DEVICE_ID 0x60


static int flashCur[] = {0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};


static struct work_struct workTimeOut;

/*****************************************************************************
Functions
*****************************************************************************/
//#define GPIO_ENF GPIO_CAMERA_FLASH_EN_PIN
//#define GPIO_ENT GPIO_CAMERA_FLASH_MODE_PIN


    /*CAMERA-FLASH-EN */


extern int iWriteRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u16 i2cId);
extern int iReadRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u8 * a_pRecvData, u16 a_sizeRecvData, u16 i2cId);
static void sub_work_timeOutFunc(struct work_struct *data);

struct platform_device *sub_flashPltFmDev = NULL;

struct pinctrl *sub_flashctl = NULL;
struct pinctrl_state *sub_flash_en_l = NULL;
struct pinctrl_state *sub_flash_en_h = NULL;
struct pinctrl_state *sub_flash_mode_en_l = NULL;
struct pinctrl_state *sub_flash_mode_en_h = NULL;
//add by xiaofei
struct pinctrl_state *aw3640_en_low = NULL;
struct pinctrl_state *aw3640_en_high = NULL;
//add by xiaofei
int sub_mtkflash_gpio_init(struct platform_device *pdev)
{
	int ret = 0;

	sub_flashctl = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR(sub_flashctl)) {
		PK_DBG("Cannot find flash pinctrl!");
		ret = PTR_ERR(sub_flashctl);
	}
	/*Cam0 Power/Rst Ping initialization */
	sub_flash_en_l = pinctrl_lookup_state(sub_flashctl, "sub_flash_en0");
	if (IS_ERR(sub_flash_en_l)) {
		ret = PTR_ERR(sub_flash_en_l);
		PK_DBG("%s : pinctrl err, flash_en0\n", __func__);
	}

	sub_flash_en_h = pinctrl_lookup_state(sub_flashctl, "sub_flash_en1");
	if (IS_ERR(sub_flash_en_h)) {
		ret = PTR_ERR(sub_flash_en_h);
		PK_DBG("%s : pinctrl err, flash_en1\n", __func__);
	}
//add by xiaofei
	aw3640_en_low = pinctrl_lookup_state(sub_flashctl, "aw3640_en0");
	if (IS_ERR(aw3640_en_low)) {
		ret = PTR_ERR(aw3640_en_low);
		PK_DBG("%s : pinctrl err, flash_en0\n", __func__);
	}

	aw3640_en_high = pinctrl_lookup_state(sub_flashctl, "aw3640_en1");
	if (IS_ERR(aw3640_en_high)) {
		ret = PTR_ERR(aw3640_en_high);
		PK_DBG("%s : pinctrl err, flash_en1\n", __func__);
	}
//add by xiaofei

	sub_flash_mode_en_l = pinctrl_lookup_state(sub_flashctl, "sub_flash_mode_en0");
	if (IS_ERR(sub_flash_mode_en_l)) {
		ret = PTR_ERR(sub_flash_mode_en_l);
		PK_DBG("%s : pinctrl err, flash_mode_en0\n", __func__);
	}

	sub_flash_mode_en_h = pinctrl_lookup_state(sub_flashctl, "sub_flash_mode_en1");
	if (IS_ERR(sub_flash_mode_en_h)) {
		ret = PTR_ERR(sub_flash_mode_en_h);
		PK_DBG("%s : pinctrl err, flash_mode_en1\n", __func__);
	}

	return ret;
}









/* add by lisong start
--------------------------------------------
true flash(KTD265EJH-TR)
ENF:GPIO12
ENM(ENT):GPIO13
1)ENF-HIGH,ENM-LOW is flash-mode
2)ENF-LOW,ENM-HIGH is torch-mode
3)ENF-HIGH,ENM-HIGH is torch-mode
4)ENF-LOW,ENM-LOW is off

--------------------------------------------
fake flash
ENABLE_PIN:GPIO12
GPIO12-HIGH:on
GPIO12-LOW:off

add by lisong end */
int Sub_FL_Enable(void)
{
	u8 i;
	u8 step;

	if(g_duty==0)
	{
		pmic_set_register_value(PMIC_ISINK_CH0_EN,1);
		PK_DBG("@xiaofei_Sub_FL_Enable ,g_duty=%d,line=%d\n",g_duty,__LINE__);
		#if 1
		pinctrl_select_state(sub_flashctl, sub_flash_en_l);
		pinctrl_select_state(sub_flashctl, sub_flash_mode_en_h);
		#endif
		for(i=0; i<12; i++)
		{
			pinctrl_select_state(sub_flashctl, aw3640_en_high);
			udelay(10);
			pinctrl_select_state(sub_flashctl, aw3640_en_low);
			udelay(10);
		}
		pinctrl_select_state(sub_flashctl, aw3640_en_high);
	}
	else
	{
		pmic_set_register_value(PMIC_ISINK_CH0_EN,1);
		pmic_set_register_value(PMIC_ISINK_CH1_EN,1);
		//pmic_set_register_value(PMIC_ISINK_CH2_EN,1);no used
		PK_DBG("@xiaofei_Sub_FL_Enable ,g_duty=%d,line=%d\n",g_duty,__LINE__);
		#if 1
		pinctrl_select_state(sub_flashctl, sub_flash_en_h);
		pinctrl_select_state(sub_flashctl, sub_flash_mode_en_h);
		#endif
		/*add by xiaofei 1024 */
	

		for(i=0; i<4; i++)
		{
			pinctrl_select_state(sub_flashctl, aw3640_en_high);
			udelay(10);
			pinctrl_select_state(sub_flashctl, aw3640_en_low);
			udelay(10);
		}
		pinctrl_select_state(sub_flashctl, aw3640_en_high);
	}
	
	PK_DBG("@xiaofei_AW3640_FL_Enable ,g_duty=%d,step=%d,line=%d\n",g_duty,step,__LINE__);
    return 0;
}

int Sub_FL_Disable(void)
{
	if(!sub_flashctl){
	PK_DBG("sub_flashctl is NULL!!!!!\n");
	return -1;	
	}
	if(!sub_flash_en_l){
		PK_DBG("flash_en_l is NULL!!!!!\n");
		return -1;	
	}
	if(!sub_flash_mode_en_l){
			PK_DBG("flash_mode_en_l is NULL!!!!!\n");
		return -1;	
	}
	//add by xiaofei
	if(!aw3640_en_low){
		PK_DBG("flash_en_l is NULL!!!!!\n");
		return -1;
	}
	
	pinctrl_select_state(sub_flashctl, aw3640_en_low);
	msleep(5);
	//add by xiaofei
	//mt_set_gpio_out(GPIO_ENT,GPIO_OUT_ZERO);
	//mt_set_gpio_out(GPIO_ENF,GPIO_OUT_ZERO);
	pinctrl_select_state(sub_flashctl, sub_flash_en_l);
	pinctrl_select_state(sub_flashctl, sub_flash_mode_en_l);
	PK_DBG(" FL_Disable line=%d\n",__LINE__);
	
pmic_set_register_value(PMIC_ISINK_CH0_EN,0);
pmic_set_register_value(PMIC_ISINK_CH1_EN,0);
pmic_set_register_value(PMIC_ISINK_CH2_EN,0);
	PK_DBG(" Sub_FL_Disable line=%d\n",__LINE__);
    return 0;
}

int Sub_FL_dim_duty(kal_uint32 duty)
{
	g_duty=duty;
	//gg_duty=duty;
	PK_DBG(" Sub_FL_dim_duty line=%d\n",__LINE__);
    return 0;
}


int Sub_FL_Init(void)
{
	PK_DBG("FL_INit is in!!!!!!\n");
//		platform_driver_register(&flashlight_driver);
		sub_flashPltFmDev = get_flashlight_platform_device();
		PK_DBG("find is ok!!!!!!!!!");
		if(!sub_flashPltFmDev){
				PK_DBG("find flashlight node fail!!!!!!!");
				return -1;
		}
		PK_DBG("sub_flashPltFmDev is not null!!!!!!!!!\n");
		if(sub_mtkflash_gpio_init(sub_flashPltFmDev)){
			PK_DBG("init flashlight GPIO fail!!!!!!!\n");
			return -1;
		}
		PK_DBG("before FL_disable!!!!!!");

	pmic_set_register_value(PMIC_RG_DRV_32K_CK_PDN,0x0); // Disable power down  
	pmic_set_register_value(PMIC_RG_DRV_ISINK0_CK_PDN,0);
	pmic_set_register_value(PMIC_RG_DRV_ISINK0_CK_CKSEL,0);
	pmic_set_register_value(PMIC_ISINK_CH0_MODE,0);
	pmic_set_register_value(PMIC_ISINK_CH0_STEP,5);//24mA 
	pmic_set_register_value(PMIC_RG_ISINK0_DOUBLE_EN,0x1);
	pmic_set_register_value(PMIC_ISINK_DIM0_DUTY,31);
	pmic_set_register_value(PMIC_ISINK_DIM0_FSEL,0);


	pmic_set_register_value(PMIC_RG_DRV_ISINK1_CK_PDN,0);
	pmic_set_register_value(PMIC_RG_DRV_ISINK1_CK_CKSEL,0);
	pmic_set_register_value(PMIC_ISINK_CH1_MODE,0);
	pmic_set_register_value(PMIC_ISINK_CH1_STEP,5);//24mA 
	pmic_set_register_value(PMIC_RG_ISINK1_DOUBLE_EN,0x1);
	pmic_set_register_value(PMIC_ISINK_DIM1_DUTY,31);
	pmic_set_register_value(PMIC_ISINK_DIM1_FSEL,0);

				FL_Disable();
	

    INIT_WORK(&workTimeOut, sub_work_timeOutFunc);
    PK_DBG(" Sub_FL_Init line=%d\n",__LINE__);
    return 0;
}


int Sub_FL_Uninit(void)
{
	Sub_FL_Disable();
    return 0;
}

/*****************************************************************************
User interface
*****************************************************************************/

static void sub_work_timeOutFunc(struct work_struct *data)
{
    Sub_FL_Disable();
    PK_DBG("ledTimeOut_callback\n");
    //printk(KERN_ALERT "work handler function./n");
}



enum hrtimer_restart subledTimeOutCallback(struct hrtimer *timer)
{
    schedule_work(&workTimeOut);
    return HRTIMER_NORESTART;
}
static struct hrtimer g_timeOutTimer;
void subtimerInit(void)
{
	g_timeOutTimeMs=1500; //1s
	hrtimer_init( &g_timeOutTimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL );
	g_timeOutTimer.function=subledTimeOutCallback;

}



static int sub_strobe_ioctl(MUINT32 cmd, MUINT32 arg)
{
	int i4RetValue = 0;
	int ior_shift;
	int iow_shift;
	int iowr_shift;
	ior_shift = cmd - (_IOR(FLASHLIGHT_MAGIC,0, int));
	iow_shift = cmd - (_IOW(FLASHLIGHT_MAGIC,0, int));
	iowr_shift = cmd - (_IOWR(FLASHLIGHT_MAGIC,0, int));
	PK_DBG("sub_strobe_ioctl() line=%d ior_shift=%d, iow_shift=%d iowr_shift=%d arg=%d\n",__LINE__, ior_shift, iow_shift, iowr_shift, arg);
    switch(cmd)
    {

		case FLASH_IOC_SET_TIME_OUT_TIME_MS:
			PK_DBG("FLASH_IOC_SET_TIME_OUT_TIME_MS: %d\n",arg);
			g_timeOutTimeMs=arg;
		break;


    	case FLASH_IOC_SET_DUTY :
    		PK_DBG("FLASHLIGHT_DUTY: %d\n",arg);
    		Sub_FL_dim_duty(arg);
    		break;


    	case FLASH_IOC_SET_STEP:
    		PK_DBG("FLASH_IOC_SET_STEP: %d\n",arg);

    		break;

    	case FLASH_IOC_SET_ONOFF :
    		PK_DBG("FLASHLIGHT_ONOFF: %d\n",arg);
    		if(arg==1)
    		{
				if(g_timeOutTimeMs!=0)
	            {
	            	ktime_t ktime;
					ktime = ktime_set( 0, g_timeOutTimeMs*1000000 );
					hrtimer_start( &g_timeOutTimer, ktime, HRTIMER_MODE_REL );
	            }
    			Sub_FL_Enable();
    		}
    		else
    		{
    			Sub_FL_Disable();
				hrtimer_cancel( &g_timeOutTimer );
    		}
    		break;
		default :
    		PK_DBG(" No such command \n");
    		i4RetValue = -EPERM;
    		break;
    }
    return i4RetValue;
}




static int sub_strobe_open(void *pArg)
{
    int i4RetValue = 0;
    PK_DBG("sub_strobe_open line=%d\n", __LINE__);

	if (0 == strobe_Res)
	{
	    Sub_FL_Init();
		subtimerInit();
	}
	PK_DBG("sub_strobe_open line=%d\n", __LINE__);
	spin_lock_irq(&g_strobeSMPLock);


    if(strobe_Res)
    {
        PK_ERR(" busy!\n");
        i4RetValue = -EBUSY;
    }
    else
    {
        strobe_Res += 1;
    }


    spin_unlock_irq(&g_strobeSMPLock);
    PK_DBG("sub_strobe_open line=%d\n", __LINE__);

    return i4RetValue;

}


static int sub_strobe_release(void *pArg)
{
    PK_DBG(" sub_strobe_release\n");

    if (strobe_Res)
    {
        spin_lock_irq(&g_strobeSMPLock);

        strobe_Res = 0;
        strobe_Timeus = 0;

        /* LED On Status */
        g_strobe_On = FALSE;

        spin_unlock_irq(&g_strobeSMPLock);

    	Sub_FL_Uninit();
    }

    PK_DBG(" Done\n");

    return 0;

}


FLASHLIGHT_FUNCTION_STRUCT	sub_constantFlashlightFunc=
{
	sub_strobe_open,
	sub_strobe_release,
	sub_strobe_ioctl
};


MUINT32 subStrobeInit(PFLASHLIGHT_FUNCTION_STRUCT *pfFunc)
{
    if (pfFunc != NULL)
    {
        *pfFunc = &sub_constantFlashlightFunc;
    }
    return 0;
}



/* LED flash control for high current capture mode*/
#if 0
ssize_t strobe_VDIrq(void)
{

    return 0;
}

EXPORT_SYMBOL(strobe_VDIrq);
#endif


#if 1//add by lisong for test start
struct class *sub_flashlight_class;
struct device *sub_flashlight_dev;



static ssize_t sub_flashlight_enable_store(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    int enable = 0;
    if(buf != NULL && size != 0)
    {
        enable = (int)simple_strtoul(buf, NULL, 0);
    }
    if (enable)
    {
        Sub_FL_Init();
        mdelay(10);
        Sub_FL_Enable();
    }
    else
    {
        Sub_FL_Disable();
    }
    return size;
}
static DEVICE_ATTR(sub_flashlight_enable, 0644, NULL, sub_flashlight_enable_store);

static int __init sub_flashlight_init(void)  
{		
	sub_flashlight_class = class_create(THIS_MODULE, "sub_flashlight");
	sub_flashlight_dev = device_create(sub_flashlight_class,NULL, 0, NULL,  "sub_flashlight");
    device_create_file(sub_flashlight_dev, &dev_attr_sub_flashlight_enable);
	return 0;
}
static void __exit sub_flashlight_exit(void)
{
	return;
}



//-----------------------------------------
module_init(sub_flashlight_init);
module_exit(sub_flashlight_exit);


MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("sub_flashlight");
MODULE_AUTHOR("lisong <song.li@ragentek.com>");
#endif//add by lisonglisong for test end

