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
#include <linux/mutex.h>
#include <linux/i2c.h>
#include <linux/leds.h>
#include <linux/proc_fs.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/gpio.h>

#ifdef CONFIG_COMPAT
#include <linux/compat.h>
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
#include <linux/mutex.h>
#else
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27)
#include <linux/semaphore.h>
#else
#include <asm/semaphore.h>
#endif
#endif
/*add by kangjun.li 2016/3/5 */

#include "mt_gpio.h"
#define GPIO_FLASHLIGHT_EN (8 | 0x80000000)

#define  FLASHLIGHT_DRVNAME "flashlight"

#if defined(CONFIG_MTK_LEGACY)
#define I2C_CONFIG_SETTING 1
#elif defined(CONFIG_OF)
#define I2C_CONFIG_SETTING 2 /* device tree */
#else

#define I2C_CONFIG_SETTING 1
#endif


#if I2C_CONFIG_SETTING == 1
#define FLASHLIGHT_I2C_BUSNUM 2
#define I2C_REGISTER_ID            0x29
#endif

#define PLATFORM_DRIVER_NAME "flashlight_platform"
#define FLASHLIGHT_DRIVER_CLASS_NAME "flashlight"


#if I2C_CONFIG_SETTING == 1
static struct i2c_board_info kd_flashlight_dev __initdata = {
	I2C_BOARD_INFO(FLASHLIGHT_DRVNAME, I2C_REGISTER_ID)
};
#endif
/*end of  flashlight add by kangjun.li*/

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
#define PK_DBG_FUNC(fmt, arg...)    pr_err(TAG_NAME "%s: " fmt, __FUNCTION__ ,##arg)
#define PK_WARN(fmt, arg...)        pr_err(TAG_NAME "%s: " fmt, __FUNCTION__ ,##arg)
#define PK_NOTICE(fmt, arg...)      pr_err(TAG_NAME "%s: " fmt, __FUNCTION__ ,##arg)
#define PK_INFO(fmt, arg...)        pr_err(TAG_NAME "%s: " fmt, __FUNCTION__ ,##arg)
#define PK_TRC_FUNC(f)              pr_err(TAG_NAME "<%s>\n", __FUNCTION__)
#define PK_TRC_VERBOSE(fmt, arg...) pr_err(TAG_NAME fmt, ##arg)
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
#define FLASHLIGHT_DEBUG
#ifdef FLASHLIGHT_DEBUG
#define LOG_INF(format, args...) pr_debug(FLASHLIGHT_DRVNAME " [%s] " format, __func__, ##args)
#else
#define LOG_INF(format, args...)
#endif

/******************************************************************************
 * local variables
******************************************************************************/

static DEFINE_SPINLOCK(g_strobeSMPLock); /* cotta-- SMP proection */

#pragma message("----------------------------------------------------------------------")

static u32 strobe_Res = 0;
static u32 strobe_Timeus = 0;
static BOOL g_strobe_On = 0;
static u32 flash_init_flag = 0;

static int g_duty=-1;
static int g_timeOutTimeMs=0;
static char get_led1_flash_value(int curr);
static char get_led2_flash_value(int curr);
static char get_led1_torch_value(int curr);
static char get_led2_torch_value(int curr);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
static DEFINE_MUTEX(g_strobeSem);
#else
static DECLARE_MUTEX(g_strobeSem);
#endif

static int flash_en(void);
static int torch_en(void);
/*add by kangjun.li*/
 //struct i2c_client *KTD2683_i2c_client = NULL;
  struct i2c_client *KTD2687_i2c_client = NULL;

#define STROBE_DEVICE_ID 0x60


static struct work_struct workTimeOut;

static char get_led1_flash_value(int curr)
{
	char ret = 0;
	ret = (char)(curr*128/1500-1);
	printk("the regs values = %d",ret);
	return ret;
	
}
static char get_led2_flash_value(int curr)
{
	char ret = 0;
	ret = (char)(curr*128/2000-1);
	printk("the regs values = %d",ret);
	return ret;
	
}
static char get_led1_torch_value(int curr)
{
	char ret = 0;
	ret = (char)(curr*128/375-1);
	printk("the regs values = %d",ret);
	return ret;
	
}
static char get_led2_torch_value(int curr)
{
	char ret = 0;
	ret = (char)(curr*64/375-1) << 1;
	printk("the regs values = %d",ret);
	return ret;
	
}

static int gduty1=0;
static int gduty2=0;
static int count = 0;
int led1_flag = 0;//default : OFF
int led2_flag = 0;
#define e_DutyNum 26
static int isTorch[e_DutyNum] = {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};


static int torchDuty[e_DutyNum] = {32,64,96,127,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
//48,94,142,187ma
static int flashDuty[e_DutyNum] = {3,8,12,14,16,20,25,29,33,37,42,46,50,55,59,63,67,72,76,80,84,93,101,110,118,127};
//200,250,300,350,400,450,500,550,600,650,700,750,800,850,900,950,1000,1100,1200,1300,1400,1500ma

/*Add three interface for flashlight by kangjun.li 2016/3/7*/

 int flash_read(struct i2c_client *client, u8 addr,u8* data)
{
    int  ret = 0;
    ret = i2c_master_send(client, &addr,1);
    if (ret < 0) 
    {
        printk("I2C send failed!! \n");
        return -1;
    }
    ret = i2c_master_recv(client, data,1);
    if (ret < 0) 
    {
        printk("I2C read failed!! \n");
        return -1;
    }
    return ret;
}
 char flash_read2(struct i2c_client *client, u8 addr)
{
    int  ret = 0;
	char data = 0;
    ret = i2c_master_send(client, &addr,1);
    if (ret < 0) 
    {
        printk("I2C send failed!! \n");
        return -1;
    }
    ret = i2c_master_recv(client, &data,1);
    if (ret < 0) 
    {
        printk("I2C read failed!! \n");
        return -1;
    }
    return data;
}
 int flash_write(struct i2c_client *client, u8 addr,u8 data)
{
    int ret = 0;
    char buf[2] = {0};
    buf[0] = addr;
    buf[1] = data;
    ret = i2c_master_send(client, buf, 2);
    if (ret < 0) 
    {
        printk("I2C send failed!!!\n");
        return -1;
    }
    return ret;
}
/*end of interface*/








    /*CAMERA-FLASH-EN */


extern int iWriteRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u16 i2cId);
extern int iReadRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u8 * a_pRecvData, u16 a_sizeRecvData, u16 i2cId);
static void work_timeOutFunc(struct work_struct *data);











int flash_enable(void)
{
			flash_write(KTD2687_i2c_client, 0x01,flash_read2(KTD2687_i2c_client, 0x01) | 0x01);
    		if (isTorch[gduty1] == 1 )//two leds all torch mode
    		{	
    			
    			flash_write(KTD2687_i2c_client, 0x01,flash_read2(KTD2687_i2c_client, 0x01) & (~(3 << 2)) );//clear [4:3] to 0
					printk("[kangjun.li]Enter into the torch mode\n");
					flash_write(KTD2687_i2c_client, 0x01,flash_read2(KTD2687_i2c_client, 0x01) | (2 << 2));
					printk("[kangjun.li]the torch duty1 = %d,the duty2 = %d\n",gduty1,gduty2);

    		}
    		else
    		{
    			
    			flash_write(KTD2687_i2c_client, 0x01,flash_read2(KTD2687_i2c_client, 0x01) & (~(3 << 2)) );//clear [4:3] to 0
					printk("[kangjun.li]Enter into the flash mode\n");
					flash_write(KTD2687_i2c_client, 0x01,flash_read2(KTD2687_i2c_client, 0x01) | (3 << 2));
					printk("[kangjun.li]the flash duty = %d,the duty2 = %d\n",gduty1,gduty2);
    			//flash_write(KTD2683_i2c_client, 0x03,torchDuty[gduty1]);//set two leds to torch mode 
//    			flash_write(KTD2683_i2c_client, 0x03,torchDuty[gduty2] << 1);//set two leds to torch mode 
    		}
	
    return 0;
}
#if 1
int flash_disable(void)
{
	flash_write(KTD2687_i2c_client, 0x01,0x00);//set to standby mode
	printk("[kangjun.li] close flash\n");
    return 0;
}
#endif

int flash_init(void)
{       
	int ret = 0;
	printk("[kangjun.li] enable led1 \n");
	flash_write(KTD2687_i2c_client, 0x01,0x01);   //enable led1
	printk("[kangjun.li] end of enable \n");
	printk("[kangjun.li] the con reg data using my function (IN FL_Init)  = %d\n",flash_read2(KTD2687_i2c_client,0x01)); 
	

	printk("[kangjun.li] set 0x02(LVP Setting) to default value \n");
	flash_write(KTD2687_i2c_client, 0x02,0x01);
	printk("[kangjun.li] end of enable \n");
	
	printk("[kangjun.li] set 0x07(Boost Converter) to default value \n");
	flash_write(KTD2687_i2c_client, 0x07,0x09);
	printk("[kangjun.li] end of enable \n");

	printk("[kangjun.li] begin to set flash time-out register\n");
	flash_write(KTD2687_i2c_client,0x08, 0x1f);
	printk("[kangjun.li] the end of set time\n");
	//enable the STROBE pin and TORCH pin by logic input ,you can also control the [5:6] bits of 0x01 
   
    /* BEGIN: Added by Limin.Hu, 2015/12/10   PN:add_DualFlash */
    INIT_WORK(&workTimeOut, work_timeOutFunc);
    /* END:   Added by Limin.Hu, 2015/12/10   PN:add_DualFlash */
    return 0;
}
int flash_setduty(int duty)
{
	
		if(isTorch[duty] == 1)
		{
			flash_write(KTD2687_i2c_client, 0x05,get_led1_torch_value(150));//set led1 to torch mode
			//flash_write(KTD2687_i2c_client, 0x06,get_led2_torch_value(150));//set led2 to torch mode
		}
		else
		{
			flash_write(KTD2687_i2c_client, 0x03,get_led1_flash_value(1000));//set led1 to flash mode
			//flash_write(KTD2687_i2c_client, 0x04,get_led2_flash_value(1000));//set led2 to flash mode
		}
	return 0;
}
int FL_Enable(void)
{
	flash_enable();
	printk("[kangjun.li] Enter into led1 enable\n");
	 return 0;
}



int FL_Disable(void)
{	
	flash_disable();
	printk("[kangjun.li] Enter into the led1 diable\n");
	 return 0;
}

int FL_dim_duty(kal_uint32 duty)
{
	gduty1 = duty;
	flash_setduty(gduty1);
	printk("[kangjun.li]the duty1 = %d\n",duty);
    return 0;
}

int FL_Init(void)
{
	flash_init();
	printk("[kangjun.li] Enter into the led1 init\n");
	 return 0;
}


int FL_Uninit(void)
{
	flash_init_flag = 0;
	FL_Disable();
	printk("[kangjun.li] Enter into the led1 Uninit\n");
    return 0;
}


/*****************************************************************************
User interface
*****************************************************************************/

static void work_timeOutFunc(struct work_struct *data)
{
    FL_Disable();
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



static int constant_flashlight_ioctl(MUINT32 cmd, MUINT32 arg)
{
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
    		FL_dim_duty(arg);
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
	            led1_flag = 1;
    			FL_Enable();
    		}
    		else
    		{
    			
    			FL_Disable();
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




static int constant_flashlight_open(void *pArg)
{
	int i4RetValue = 0;
	PK_DBG("constant_flashlight_open line=%d\n", __LINE__);

	mt_set_gpio_mode(GPIO_FLASHLIGHT_EN,GPIO_MODE_00);  // gpio mode
  mt_set_gpio_pull_enable(GPIO_FLASHLIGHT_EN,GPIO_PULL_ENABLE);
  mt_set_gpio_dir(GPIO_FLASHLIGHT_EN,GPIO_DIR_OUT); // output
  mt_set_gpio_out(GPIO_FLASHLIGHT_EN,GPIO_OUT_ONE); // low
	if (0 == strobe_Res)
	{
		FL_Init();
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

}


static int constant_flashlight_release(void *pArg)
{
  PK_DBG(" constant_flashlight_release\n");

  if (strobe_Res)
	{
    spin_lock_irq(&g_strobeSMPLock);

    strobe_Res = 0;
    strobe_Timeus = 0;

        /* LED On Status */
    g_strobe_On = FALSE;

    spin_unlock_irq(&g_strobeSMPLock);

  	FL_Uninit();
  }

  PK_DBG(" Done\n");

	mt_set_gpio_mode(GPIO_FLASHLIGHT_EN,GPIO_MODE_00);  // gpio mode
  mt_set_gpio_pull_enable(GPIO_FLASHLIGHT_EN,GPIO_PULL_DISABLE);
  mt_set_gpio_dir(GPIO_FLASHLIGHT_EN,GPIO_DIR_OUT); // output
  mt_set_gpio_out(GPIO_FLASHLIGHT_EN,GPIO_OUT_ZERO); // low
  return 0;

}


FLASHLIGHT_FUNCTION_STRUCT	constantFlashlightFunc=
{
	constant_flashlight_open,
	constant_flashlight_release,
	constant_flashlight_ioctl
};




MUINT32 constantFlashlightInit(PFLASHLIGHT_FUNCTION_STRUCT *pfFunc)
{
    if (pfFunc != NULL)
    {
        *pfFunc = &constantFlashlightFunc;
    }
    return 0;
}



/* LED flash control for high current capture mode*/
ssize_t strobe_VDIrq(void)
{

    return 0;
}

EXPORT_SYMBOL(strobe_VDIrq);



#if 1

static int flash_enable_flag = 0;
static ssize_t flashlight_torch_enable_show(struct device_driver *ddp, char *buf)
{
	return snprintf(buf, 4, "%d\n", flash_enable_flag);
}

static ssize_t flashlight_torch_enable_store(struct device_driver *ddp,
				      const char *buf, size_t count)
{
	PK_DBG("the buf is %s\n,count is %d\n",buf,count);
	if(*buf<'0'|| *buf > '9'){
		return count;
	}
	int value = (*buf) - '0';
	if (value != 0)
		flash_enable_flag = 1;
	else
		flash_enable_flag = 0;

	if(flash_enable_flag){
		if(!flash_init_flag){
			FL_Init();
			if(value == 1)
			{
				flash_write(KTD2687_i2c_client, 0x05,get_led1_torch_value(150));//set led1 to torch mode
				//flash_write(KTD2687_i2c_client, 0x06,get_led2_torch_value(150));//set led2 to torch mode
				torch_en();
			}	
			else if(value == 2)
			{
				flash_write(KTD2687_i2c_client, 0x03,get_led1_flash_value(1000));//set led1 to flash mode
				//flash_write(KTD2687_i2c_client, 0x04,get_led2_flash_value(1000));//set led2 to flash mode
				flash_en();
			}
		}
		
		
	}
	else{
		FL_Disable();
		 flash_disable();
	}	
	
	return count;
}

static DRIVER_ATTR(flash_torch_en, 0644, flashlight_torch_enable_show, flashlight_torch_enable_store);
/*----------------------------------------------------------------------------*/
static struct driver_attribute *flash_attr_list[] = {
	&driver_attr_flash_torch_en,
};
/*----------------------------------------------------------------------------*/
 int flash_create_attr(struct device_driver *driver)
{
	int idx, err = 0;
	int num = (int)(sizeof(flash_attr_list)/sizeof(flash_attr_list[0]));

	if (driver == NULL)
		return -EINVAL;

	for (idx = 0; idx < num; idx++) {
		err = driver_create_file(driver, flash_attr_list[idx]);
		if (err) {
			PK_DBG("driver_create_file (%s) = %d\n", flash_attr_list[idx]->attr.name, err);
			break;
		}
	}
	return err;
}
/*----------------------------------------------------------------------------*/
 int flash_delete_attr(struct device_driver *driver)
{
	int idx , err = 0;
	int num = (int)(sizeof(flash_attr_list)/sizeof(flash_attr_list[0]));

	if (driver == NULL)
		return -EINVAL;

	for (idx = 0; idx < num; idx++)
		driver_remove_file(driver, flash_attr_list[idx]);

	return err;
}


#endif




/*add by kangjun.li 2016/3/5*/

//static struct class *flashlight_class = NULL;
#if 1
static int  FLASHLIGHT_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int FLASHLIGHT_i2c_remove(struct i2c_client *client);
static const struct i2c_device_id FLASHLIGHT_i2c_id[] = { {FLASHLIGHT_DRVNAME, 0}, {} };



/* Compatible name must be the same with that defined in codegen.dws and cust_i2c.dtsi */
/* TOOL : kernel-3.10\tools\dct */
/* PATH : vendor\mediatek\proprietary\custom\#project#\kernel\dct\dct */
#if I2C_CONFIG_SETTING == 2
static const struct of_device_id FLASHLIGHT_of_match[] = {
	{.compatible = "mediatek,STROBE_MAIN"},
	{},
};
#endif

static struct i2c_driver FLASHLIGHT_i2c_driver = {
	.probe = FLASHLIGHT_i2c_probe,
	.remove = FLASHLIGHT_i2c_remove,
	.driver.name = FLASHLIGHT_DRVNAME,
#if I2C_CONFIG_SETTING == 2
	.driver.of_match_table = FLASHLIGHT_of_match,
#endif
	.id_table = FLASHLIGHT_i2c_id,
};

static int FLASHLIGHT_i2c_remove(struct i2c_client *client)
{
	return 0;
}

static int flash_en(void)
{
	printk("[kangjun.li]Enter into the flash mode\n");
	flash_write(KTD2687_i2c_client, 0x01,flash_read2(KTD2687_i2c_client, 0x01) & (~(3 << 2)) );//clear [4:3] to 0
	flash_write(KTD2687_i2c_client, 0x01,flash_read2(KTD2687_i2c_client, 0x01) | (3 << 2));
}
static int torch_en(void)
{
	printk("[kangjun.li]Enter into the flash mode\n");
	flash_write(KTD2687_i2c_client, 0x01,flash_read2(KTD2687_i2c_client, 0x01) & (~(3 << 2)) );//clear [4:3] to 0
	flash_write(KTD2687_i2c_client, 0x01,flash_read2(KTD2687_i2c_client, 0x01) | (2 << 2));
}

struct class *flashlight_torch_class;
struct device *flashlight_torch_dev;
static ssize_t torch_level_store(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    int enable = 0;
    if(buf != NULL && size != 0)
    {
        enable = (int)simple_strtoul(buf, NULL, 0);
    }
    if (enable != 0)
    {
        flash_init();
        switch (enable) {
        case 1:
        	flash_write(KTD2687_i2c_client, 0x05,get_led1_torch_value(100));//set led1 to flash mode
			//flash_write(KTD2687_i2c_client, 0x06,get_led2_torch_value(100));//set led2 to flash mode
			torch_en();
			break;
		case 2:
			flash_write(KTD2687_i2c_client, 0x05,get_led1_torch_value(200));//set led1 to flash mode
			//flash_write(KTD2687_i2c_client, 0x06,get_led2_torch_value(200));//set led2 to flash mode
			torch_en();
			break;
		case 3:
			flash_write(KTD2687_i2c_client, 0x05,get_led1_torch_value(350));//set led1 to flash mode
			//flash_write(KTD2687_i2c_client, 0x06,get_led2_torch_value(350));//set led2 to flash mode
			torch_en();
			break;
		
		case 9:
			flash_write(KTD2687_i2c_client, 0x03,get_led1_flash_value(500));//set led1 to flash mode
			//flash_write(KTD2687_i2c_client, 0x04,get_led2_flash_value(500));//set led2 to flash mode
			flash_en();
			break;
		default:
			printk("The duty value is invalid! It must between 1~9 .\n");
			flash_disable();
			break;
		}
        
			
			
    }
    else
    {
        flash_disable();
       // flash_write(KTD2683_i2c_client, 0x0e,0x00);
    }
    
    return size;
}
static DEVICE_ATTR(torch_level, 0644, NULL, torch_level_store);

/* Kirby: add new-style driver {*/
static int FLASHLIGHT_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int i4RetValue = 0;
	flashlight_torch_class = class_create(THIS_MODULE, "torch");
	flashlight_torch_dev = device_create(flashlight_torch_class,NULL, 0, NULL,  "torch");
    device_create_file(flashlight_torch_dev, &dev_attr_torch_level);
	LOG_INF("Start\n");

	/* Kirby: add new-style driver { */
	KTD2687_i2c_client = client;

	LOG_INF("Attached!!\n");

	return 0;
}

static int FLASHLIGHT_probe(struct platform_device *pdev)
{
	return i2c_add_driver(&FLASHLIGHT_i2c_driver);
}

static int FLASHLIGHT_remove(struct platform_device *pdev)
{
	i2c_del_driver(&FLASHLIGHT_i2c_driver);
	return 0;
}

static int FLASHLIGHT_suspend(struct platform_device *pdev, pm_message_t mesg)
{
	return 0;
}

static int FLASHLIGHT_resume(struct platform_device *pdev)
{
	return 0;
}
#endif
/* platform structure */
static struct platform_driver g_stFLASHLIGHT_Driver = {
	.probe = FLASHLIGHT_probe,
	.remove = FLASHLIGHT_remove,
	.suspend = FLASHLIGHT_suspend,
	.resume = FLASHLIGHT_resume,
	.driver = {
		   .name = PLATFORM_DRIVER_NAME,
		   .owner = THIS_MODULE,
		   }
};

static struct platform_device g_stFLASHLIGHT_device = {
	.name = PLATFORM_DRIVER_NAME,
	.id = 0,
	.dev = {}
};

static int __init FLASHLIGHT_i2C_init(void)
{
	#if I2C_CONFIG_SETTING == 1
	i2c_register_board_info(FLASHLIGHT_I2C_BUSNUM, &kd_flashlight_dev, 1);
	#endif

	if (platform_device_register(&g_stFLASHLIGHT_device)) {
		LOG_INF("failed to register AF driver\n");
		return -ENODEV;
	}

	if (platform_driver_register(&g_stFLASHLIGHT_Driver)) {
		LOG_INF("Failed to register AF driver\n");
		return -ENODEV;
	}

	return 0;
}

static void __exit FLASHLIGHT_i2C_exit(void)
{
	platform_driver_unregister(&g_stFLASHLIGHT_Driver);
}
module_init(FLASHLIGHT_i2C_init);
module_exit(FLASHLIGHT_i2C_exit);

MODULE_DESCRIPTION("FLASHLIGHT module driver");
MODULE_AUTHOR("kangjun.li <kangjun.li@Ragentek.com>");
MODULE_LICENSE("GPL");

