
#include <linux/kernel.h>
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
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/version.h>

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
#include <linux/mutex.h>
#else
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27)
#include <linux/semaphore.h>
#else
#include <asm/semaphore.h>
#endif
#endif

#include "mt_gpio.h"
#define GPIO_FLASHLIGHT_EN (8 | 0x80000000)


/******************************************************************************
 * Debug configuration
******************************************************************************/
/* availible parameter */
/* ANDROID_LOG_ASSERT */
/* ANDROID_LOG_ERROR */
/* ANDROID_LOG_WARNING */
/* ANDROID_LOG_INFO */
/* ANDROID_LOG_DEBUG */
/* ANDROID_LOG_VERBOSE */
#define TAG_NAME "[sub_strobe.c]"
#define PK_DBG_NONE(fmt, arg...)    do {} while (0)
#define PK_DBG_FUNC(fmt, arg...)    pr_debug(TAG_NAME "%s: " fmt, __func__ , ##arg)
#define PK_WARN(fmt, arg...)        pr_warn(TAG_NAME "%s: " fmt, __func__ , ##arg)
#define PK_NOTICE(fmt, arg...)      pr_notice(TAG_NAME "%s: " fmt, __func__ , ##arg)
#define PK_INFO(fmt, arg...)        pr_info(TAG_NAME "%s: " fmt, __func__ , ##arg)
#define PK_TRC_FUNC(f)              pr_debug(TAG_NAME "<%s>\n", __func__)
#define PK_TRC_VERBOSE(fmt, arg...) pr_debug(TAG_NAME fmt, ##arg)
#define PK_ERROR(fmt, arg...)       pr_err(TAG_NAME "%s: " fmt, __func__ , ##arg)

#define DEBUG_LEDS_STROBE
#ifdef DEBUG_LEDS_STROBE
#define PK_DBG PK_DBG_FUNC
#define PK_VER PK_TRC_VERBOSE
#define PK_ERR PK_ERROR
#else
#define PK_DBG(a, ...)
#define PK_VER(a, ...)
#define PK_ERR(a, ...)
#endif
static DEFINE_SPINLOCK(g_strobeSMPLock); /* cotta-- SMP proection */
static u32 strobe_Res = 0;
static u32 strobe_Timeus = 0;
static BOOL g_strobe_On = 0;
static int duty_sub = 0;
static int g_timeOutTimeMs=0;
//extern struct i2c_client *KTD2683_i2c_client ;
extern struct i2c_client *KTD2687_i2c_client ;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
static DEFINE_MUTEX(g_strobeSem);
#else
static DECLARE_MUTEX(g_strobeSem);
#endif
extern int flash_read(struct i2c_client *client, u8 addr,u8* data);
extern char flash_read2(struct i2c_client *client, u8 addr);
extern int flash_write(struct i2c_client *client, u8 addr,u8 data);
static char get_led3_torch_value (int curr)
{
	char ret = 0;
	ret = (char)(curr*64/500 -1) << 2;
	printk("the sub_torch value = %d\n",ret >> 2);
	return ret;
}
static int sub_FL_Enable(void)
{	
	flash_write(KTD2687_i2c_client, 0x01,0x0A);
	/*
	if (duty_sub == 0)
		flash_write(KTD2687_i2c_client, 0x01,0x0A);
	else 
		flash_write(KTD2687_i2c_client, 0x01,0x0e);*/
	return 0;
}
static int sub_FL_dim_duty(int arg)
{
	
	duty_sub = arg;
	if (duty_sub == 0)
		flash_write(KTD2687_i2c_client, 0x06,0x32);
	else 
		flash_write(KTD2687_i2c_client, 0x06,0xfe);
	return 0;
}

static int sub_FL_Init(void)
{
	printk("set led2 to standby mode \n");
	flash_write(KTD2687_i2c_client, 0x01,0x00);
	return 0;
}
static int sub_FL_Disable(void)
{
	printk("set led3 to off mode \n");
	flash_write(KTD2687_i2c_client, 0x01,0x00);
	return 0;
}

int sub_FL_Uninit(void)
{
	sub_FL_Disable();
    return 0;
}

static struct work_struct workTimeOut;
static void work_timeOutFunc(struct work_struct *data);
static void work_timeOutFunc(struct work_struct *data)
{
    sub_FL_Disable();
    PK_DBG("ledTimeOut_callback\n");
    //printk(KERN_ALERT "work handler function./n");
}



static enum hrtimer_restart ledTimeOutCallback(struct hrtimer *timer)
{
    schedule_work(&workTimeOut);
    return HRTIMER_NORESTART;
}
static struct hrtimer g_timeOutTimer;
static void timerInit(void)
{
	g_timeOutTimeMs=1000; //1s
	hrtimer_init( &g_timeOutTimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL );
	g_timeOutTimer.function=ledTimeOutCallback;

}
static int sub_strobe_ioctl(unsigned int cmd, unsigned long arg)
{
	PK_DBG("sub flashlight ioctl");
	int i4RetValue = 0;
	int ior_shift;
	int iow_shift;
	int iowr_shift;
	ior_shift = cmd - (_IOR(FLASHLIGHT_MAGIC,0, int));
	iow_shift = cmd - (_IOW(FLASHLIGHT_MAGIC,0, int));
	iowr_shift = cmd - (_IOWR(FLASHLIGHT_MAGIC,0, int));
	PK_DBG("constant_flashlight_ioctl() line=%d ior_shift=%d, iow_shift=%d iowr_shift=%d arg=%d\n",__LINE__, ior_shift, iow_shift, iowr_shift, arg);
    switch(cmd)
    {

		case FLASH_IOC_SET_TIME_OUT_TIME_MS:
			PK_DBG("FLASH_IOC_SET_TIME_OUT_TIME_MS: %d\n",arg);
			g_timeOutTimeMs=arg;
		break;


    	case FLASH_IOC_SET_DUTY :
    		PK_DBG("FLASHLIGHT_DUTY: %d\n",arg);
    		sub_FL_dim_duty(arg);
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
	            
    			sub_FL_Enable();
    		}
    		else
    		{
    			
    			sub_FL_Disable();
					hrtimer_cancel( &g_timeOutTimer );
    		}
    		break;
			default :
    		PK_DBG(" No such command \n");
    		i4RetValue = -EPERM;
    		break;
    }
    return i4RetValue;
	return 0;
}

static int sub_strobe_open(void *pArg)
{
	mt_set_gpio_mode(GPIO_FLASHLIGHT_EN,GPIO_MODE_00);  // gpio mode
  mt_set_gpio_pull_enable(GPIO_FLASHLIGHT_EN,GPIO_PULL_ENABLE);
  mt_set_gpio_dir(GPIO_FLASHLIGHT_EN,GPIO_DIR_OUT); // output
  mt_set_gpio_out(GPIO_FLASHLIGHT_EN,GPIO_OUT_ONE); // low
  
	PK_DBG("sub dummy open");
	 int i4RetValue = 0;
    PK_DBG("constant_flashlight_open line=%d\n", __LINE__);

	if (0 == strobe_Res)
	{
	    sub_FL_Init();
			timerInit();
	}
	PK_DBG("constant_flashlight_open line=%d\n", __LINE__);
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
    PK_DBG("constant_flashlight_open line=%d\n", __LINE__);

    return i4RetValue;

	return 0;

}

static int sub_strobe_release(void *pArg)
{
	PK_DBG("sub dummy release");
	 if (strobe_Res)
    {
        spin_lock_irq(&g_strobeSMPLock);

        strobe_Res = 0;
        strobe_Timeus = 0;

        /* LED On Status */
        g_strobe_On = FALSE;

        spin_unlock_irq(&g_strobeSMPLock);

    	sub_FL_Uninit();
    }

    PK_DBG(" Done\n");
	mt_set_gpio_mode(GPIO_FLASHLIGHT_EN,GPIO_MODE_00);  // gpio mode
  mt_set_gpio_pull_enable(GPIO_FLASHLIGHT_EN,GPIO_PULL_DISABLE);
  mt_set_gpio_dir(GPIO_FLASHLIGHT_EN,GPIO_DIR_OUT); // output
  mt_set_gpio_out(GPIO_FLASHLIGHT_EN,GPIO_OUT_ZERO); // low
	return 0;

}

FLASHLIGHT_FUNCTION_STRUCT subStrobeFunc = {
	sub_strobe_open,
	sub_strobe_release,
	sub_strobe_ioctl
};


MUINT32 subStrobeInit(PFLASHLIGHT_FUNCTION_STRUCT *pfFunc)
{
	if (pfFunc != NULL)
		*pfFunc = &subStrobeFunc;
	return 0;
}
//add by likangjun
struct class *  sub_flashlight_class;
struct device * sub_flashlight_dev;



static ssize_t sub_flashlight_enable_store(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    int enable = 0;
    if(buf != NULL && size != 0)
    {
        enable = (int)simple_strtoul(buf, NULL, 0);
    }
    if (enable)
    {
    	/*
       switch (enable) {
			case 1:
				flash_write(KTD2687_i2c_client, 0x06,get_led3_torch_value(200) | 2);
				break;
			case 2:
				flash_write(KTD2687_i2c_client, 0x06,get_led3_torch_value(300) | 2);
				break;
     		case 3:
				flash_write(KTD2687_i2c_client, 0x06,get_led3_torch_value(400) | 2);
				break;
     		case 4:
				flash_write(KTD2687_i2c_client, 0x06,get_led3_torch_value(500) | 2);
				break;
     		default :
				flash_write(KTD2687_i2c_client, 0x01,0);
				
				break;
       }
       */
       sub_FL_Init();
       mdelay(10);
       sub_FL_Enable();
    }
    else
    {
        sub_FL_Disable();
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
module_init(sub_flashlight_init);
module_exit(sub_flashlight_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("sub_flashlight");
MODULE_AUTHOR("likangjun <kangjun.li@ragentek.com>");
//add by likangjun for test end
