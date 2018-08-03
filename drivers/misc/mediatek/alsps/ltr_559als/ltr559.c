/*
 *
 * Author: MingHsien Hsieh <minghsien.hsieh@mediatek.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#define POWER_NONE_MACRO MT65XX_POWER_NONE

#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/gpio.h>
#include "cust_alsps.h"
#include "ltr559.h"
#include "alsps.h"

//#include <linux/hwmsensor.h>
//#include <linux/hwmsen_dev.h>


#undef CUSTOM_KERNEL_SENSORHUB
#ifdef CUSTOM_KERNEL_SENSORHUB
#include <SCP_sensorHub.h>
#endif

#define LTR556_SW_CALI //lisong test

#define POWER_NONE_MACRO MT65XX_POWER_NONE
//#ifndef LTR556_SW_CALI//lisong test  //--modfied by hongguang@wecorp.com
#define GN_MTK_BSP_PS_DYNAMIC_CALI
//#endif
/******************************************************************************
 * configuration
*******************************************************************************/
/*----------------------------------------------------------------------------*/

#define LTR559_DEV_NAME   "LTR_559ALS"


/*----------------------------------------------------------------------------*/

#define APS_TAG                  "[ALS/PS] "
#ifdef CONFIG_MT_ENG_BUILD 

#define APS_FUN(f)               pr_err(APS_TAG"%s\n", __func__)
#define APS_ERR(fmt, args...)    pr_err(APS_TAG"%s %d : "fmt, __func__, __LINE__, ##args)
#define APS_LOG(fmt, args...)    pr_err(APS_TAG "%s %d : "fmt, __func__, __LINE__,##args)
#define APS_DBG(fmt, args...)    pr_err(APS_TAG "%s %d : "fmt, __func__, __LINE__, ##args)         
#else 

#define APS_FUN(f)        
#define APS_ERR(fmt, args...)   
#define APS_LOG(fmt, args...)  
#define APS_DBG(fmt, args...)       
#endif
/******************************************************************************
 * extern functions
*******************************************************************************/


/*----------------------------------------------------------------------------*/

static struct i2c_client *ltr559_i2c_client = NULL;

/*----------------------------------------------------------------------------*/
static const struct i2c_device_id ltr559_i2c_id[] = {{LTR559_DEV_NAME,0},{}};
struct alsps_hw alsps_cust;
static struct alsps_hw *hw = &alsps_cust;
struct platform_device *alspsPltFmDev;

/*----------------------------------------------------------------------------*/
static int ltr559_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id); 
static int ltr559_i2c_remove(struct i2c_client *client);
//static int ltr559_i2c_detect(struct i2c_client *client, int kind, struct i2c_board_info *info);
static int ltr559_i2c_detect(struct i2c_client *client, struct i2c_board_info *info);
/*----------------------------------------------------------------------------*/
static int ltr559_i2c_suspend(struct i2c_client *client, pm_message_t msg);
static int ltr559_i2c_resume(struct i2c_client *client);
static int ltr559_ps_enable(int gainrange);
static int dynamic_calibrate=2047;

//static int ps_trigger_high = 800;	
//static int ps_trigger_low = 760;

static int ps_gainrange;
static int als_gainrange;

static int final_prox_val , prox_val;
static int final_lux_val;

struct alsps_hw *get_cust_alsps(void)
{
	return &alsps_cust;
}

/*----------------------------------------------------------------------------*/
static DEFINE_MUTEX(read_lock);
static DEFINE_MUTEX(ltr559_mutex);


/*----------------------------------------------------------------------------*/
static int ltr559_als_read(int gainrange);
static int ltr559_ps_read(void);
static int ltr559_devinit(void);
static int ltr559_als_enable(int gainrange);


/*----------------------------------------------------------------------------*/


typedef enum {
    CMC_BIT_ALS    = 1,
    CMC_BIT_PS     = 2,
} CMC_BIT;

/*----------------------------------------------------------------------------*/
struct ltr559_i2c_addr {    /*define a series of i2c slave address*/
    u8  write_addr;  
    u8  ps_thd;     /*PS INT threshold*/
};

/*----------------------------------------------------------------------------*/

struct ltr559_priv {
    struct alsps_hw  *hw;
    struct i2c_client *client;
    struct work_struct  eint_work;
    struct mutex lock;
	/*i2c address group*/
    struct ltr559_i2c_addr  addr;

     /*misc*/
    u16		    als_modulus;
    atomic_t    i2c_retry;
    atomic_t    als_debounce;   /*debounce time after enabling als*/
    atomic_t    als_deb_on;     /*indicates if the debounce is on*/
    atomic_t    als_deb_end;    /*the jiffies representing the end of debounce*/
    atomic_t    ps_mask;        /*mask ps: always return far away*/
    atomic_t    ps_debounce;    /*debounce time after enabling ps*/
    atomic_t    ps_deb_on;      /*indicates if the debounce is on*/
    atomic_t    ps_deb_end;     /*the jiffies representing the end of debounce*/
    atomic_t    ps_suspend;
    atomic_t    als_suspend;
    atomic_t init_done;
	struct device_node *irq_node;
	int irq;

    /*data*/
    u16         als;
    u16          ps;
    u8          _align;
    u16         als_level_num;
    u16         als_value_num;
    u32         als_level[C_CUST_ALS_LEVEL-1];
    u32         als_value[C_CUST_ALS_LEVEL];
	int			 ps_cali;

	

    atomic_t    als_cmd_val;    /*the cmd value can't be read, stored in ram*/
    atomic_t    ps_cmd_val;     /*the cmd value can't be read, stored in ram*/
    atomic_t    ps_thd_val;     /*the cmd value can't be read, stored in ram*/
	atomic_t    ps_thd_val_high;     /*the cmd value can't be read, stored in ram*/
	atomic_t    ps_thd_val_low;     /*the cmd value can't be read, stored in ram*/
    ulong       enable;         /*enable mask*/
    ulong       pending_intr;   /*pending interrupt*/

    /*early suspend*/
#if defined(CONFIG_HAS_EARLYSUSPEND)
    struct early_suspend    early_drv;
#endif     
};

 struct PS_CALI_DATA_STRUCT
{
    int close;
    int far_away;
    int valid;
} ;

static struct PS_CALI_DATA_STRUCT ps_cali={0,0,0};
static int intr_flag_value = 0;


static struct ltr559_priv *ltr559_obj = NULL;

#ifdef CONFIG_OF
static const struct of_device_id alsps_of_match[] = {
	{.compatible = "mediatek,alsps"},
	{},
};
#endif

/*----------------------------------------------------------------------------*/
static struct i2c_driver ltr559_i2c_driver = {	
	.probe      = ltr559_i2c_probe,
	.remove     = ltr559_i2c_remove,
	.detect     = ltr559_i2c_detect,
	.suspend    = ltr559_i2c_suspend,
	.resume     = ltr559_i2c_resume,
	.id_table   = ltr559_i2c_id,
	
	.driver = {
		//.owner          = THIS_MODULE,
		.name           = LTR559_DEV_NAME,
		#ifdef CONFIG_OF
		.of_match_table = alsps_of_match,
		#endif
	},
};



static int ltr559_probe(void);
static int ltr559_remove(void);
//static int CM36558_init_flag = -1;
static struct  alsps_init_info ltr559_init_info = {
	.name = LTR559_DEV_NAME,
	.init = ltr559_probe,
	.uninit = ltr559_remove,

};

/* 
 * #########
 * ## I2C ##
 * #########
 */

// I2C Read
static int ltr559_i2c_read_reg(u8 regnum)
{
//APS_LOG("E!!\n");
    u8 buffer[1],reg_value[1];
	int res = 0;
	mutex_lock(&read_lock);
	
	buffer[0]= regnum;
	res = i2c_master_send(ltr559_obj->client, buffer, 0x1);
	if(res <= 0)	{
	   
	   APS_ERR("read reg send res = %d\n",res);
		return res;
	}
	res = i2c_master_recv(ltr559_obj->client, reg_value, 0x1);
	if(res <= 0)
	{
		APS_ERR("read reg recv res = %d\n",res);
		return res;
	}
	mutex_unlock(&read_lock);
	return reg_value[0];
}

// I2C Write
static int ltr559_i2c_write_reg(u8 regnum, u8 value)
{
//APS_LOG("E!!\n");
	u8 databuf[2];    
	int res = 0;
 
	//if(regnum == LTR559_ALS_CONTR){
	//	APS_DBG("Askia add in will write als reg(0x80) 20171115\n");
	//	if(0 == (value & 0x01)){
	//		APS_DBG("Askia add in will make als work in standby mode!!!!!!!!!!!!!!!!!!!!!!!!!!! 20171115\n");
	//		dump_stack();
	//	}
	//}
	databuf[0] = regnum;   
	databuf[1] = value;
	res = i2c_master_send(ltr559_obj->client, databuf, 0x2);

	if (res < 0)
	{
		APS_ERR("wirte reg send res = %d\n",res);
	   	return res;
	}		
	else
		return 0;
}

/*----------------------------------------------------------------------------*/
#ifdef GN_MTK_BSP_PS_DYNAMIC_CALI
static ssize_t ltr559_dynamic_calibrate(void)			
{	
	int i=0;
	int data;
	int data_total=0;
	ssize_t len = 0;
	int noise = 0;
	int count = 5;
	int max = 0;
	int ps_thd_val_low,	ps_thd_val_high ;
	struct ltr559_priv *obj = ltr559_obj;
	
	APS_LOG("E!!\n");
	
	if(!ltr559_obj)
	{	
		APS_ERR("ltr559_obj is null!!\n");
		//len = sprintf(buf, "ltr559_obj is null\n");
		return -1;
	}

	// wait for register to be stable
	msleep(15);


	for (i = 0; i < count; i++) {
		// wait for ps value be stable
		
		msleep(15);
		
		data=ltr559_ps_read();
		if (data < 0) {
			i--;
			continue;
		}
				
		if(data & 0x8000){
			noise = 0;
			break;
		}else{
			noise=data;
		}	
		
		data_total+=data;

		if (max++ > 100) {
			//len = sprintf(buf,"adjust fail\n");
			return len;
		}
	}

	
	noise=data_total/count;
	if(noise < (dynamic_calibrate + 500)){
	dynamic_calibrate = noise;
	if(noise < 100){

			atomic_set(&obj->ps_thd_val_high,  noise+100);//wangxiqiang
			atomic_set(&obj->ps_thd_val_low, noise+70);
	}else if(noise < 200){
			atomic_set(&obj->ps_thd_val_high,  noise+110);
			atomic_set(&obj->ps_thd_val_low, noise+80);
	}else if(noise < 300){
			atomic_set(&obj->ps_thd_val_high,  noise+150);
			atomic_set(&obj->ps_thd_val_low, noise+100);
	}else if(noise < 400){
			atomic_set(&obj->ps_thd_val_high,  noise+150);
			atomic_set(&obj->ps_thd_val_low, noise+100);
	}else if(noise < 600){
			atomic_set(&obj->ps_thd_val_high,  noise+200);
			atomic_set(&obj->ps_thd_val_low, noise+150);
	}else if(noise < 1000){
		atomic_set(&obj->ps_thd_val_high,  noise+250);
		atomic_set(&obj->ps_thd_val_low, noise+200);	
	}else if(noise < 1650){
			atomic_set(&obj->ps_thd_val_high,  noise+300);
			atomic_set(&obj->ps_thd_val_low, noise+250);
	}
	else{
			atomic_set(&obj->ps_thd_val_high,  1800);
			atomic_set(&obj->ps_thd_val_low, 1700);
			//isadjust = 0;
		printk(KERN_ERR "ltr558 the proximity sensor structure is error\n");
	}
}

	
	ps_thd_val_low = atomic_read(&obj->ps_thd_val_low);
	ps_thd_val_high = atomic_read(&obj->ps_thd_val_high);

	return 0;
}
#endif

/*----------------------------------------------------------------------------*/
static ssize_t ltr559_show_als(struct device_driver *ddri, char *buf)
{
	int res;
	//u8 dat = 0;
	
	APS_LOG("E!!\n");
	
	if(!ltr559_obj)
	{
		APS_ERR("ltr559_obj is null!!\n");
		return 0;
	}
	res = ltr559_als_read(als_gainrange);
    return snprintf(buf, PAGE_SIZE, "0x%04X\n", res);    
	
}
/*----------------------------------------------------------------------------*/
static ssize_t ltr559_show_ps(struct device_driver *ddri, char *buf)
{
	int  res;

	APS_LOG("E!!\n");
	
	if(!ltr559_obj)
	{
		APS_ERR("ltr559_obj is null!!\n");
		return 0;
	}
	res = ltr559_ps_read();
    return snprintf(buf, PAGE_SIZE, "0x%04X\n", res);     
}
/*----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------*/
static ssize_t ltr559_show_status(struct device_driver *ddri, char *buf)
{
	ssize_t len = 0;
	
	APS_LOG("E!!\n");
	
	if(!ltr559_obj)
	{
		APS_ERR("ltr559_obj is null!!\n");
		return 0;
	}
	
	if(ltr559_obj->hw)
	{
	
		len += snprintf(buf+len, PAGE_SIZE-len, "CUST: %d, (%d %d)\n", 
			ltr559_obj->hw->i2c_num, ltr559_obj->hw->power_id, ltr559_obj->hw->power_vol);
		
	}
	else
	{
		len += snprintf(buf+len, PAGE_SIZE-len, "CUST: NULL\n");
	}


	len += snprintf(buf+len, PAGE_SIZE-len, "MISC: %d %d\n", atomic_read(&ltr559_obj->als_suspend), atomic_read(&ltr559_obj->ps_suspend));

	return len;
}

/*----------------------------------------------------------------------------*/
static ssize_t ltr559_store_status(struct device_driver *ddri, char *buf, size_t count)
{
	int status1,ret;
	
	APS_LOG("E!!\n");
	
	if(!ltr559_obj)
	{
		APS_ERR("ltr559_obj is null!!\n");
		return 0;
	}
	
	if(1 == sscanf(buf, "%d ", &status1))
	{ 
	    ret=ltr559_ps_enable(ps_gainrange);
		APS_DBG("iret= %d, ps_gainrange = %d\n", ret, ps_gainrange);
	}
	else
	{
		APS_DBG("invalid content: '%s', length = %d\n", buf, count);
	}
	return count;    
}


/*----------------------------------------------------------------------------*/
static ssize_t ltr559_show_reg(struct device_driver *ddri, char *buf, size_t count)
{
	int i,len=0;
	int reg[]={0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,
		0x8d,0x8e,0x8f,0x90,0x91,0x92,0x93,0x94,0x95,0x97,0x98,0x99,0x9a,0x9e};
	
	APS_LOG("E!!\n");
	
	for(i=0;i<27;i++)
		{
		len += snprintf(buf+len, PAGE_SIZE-len, "reg:0x%04X value: 0x%04X\n", reg[i],ltr559_i2c_read_reg(reg[i]));	

	    }
	return len;
}
/*----------------------------------------------------------------------------*/
static ssize_t ltr559_store_reg(struct device_driver *ddri, char *buf, size_t count)
{
	int ret,value;
	u32 reg;
	
	APS_LOG("E!!\n");
	
	if(!ltr559_obj)
	{
		APS_ERR("ltr559_obj is null!!\n");
		return 0;
	}
	
	if(2 == sscanf(buf, "%x %x ", &reg,&value))
	{ 
		APS_DBG("before write reg: %x, reg_value = %x  write value=%x\n", reg,ltr559_i2c_read_reg(reg),value);
	    ret=ltr559_i2c_write_reg(reg,value);
		APS_DBG("after write reg: %x, reg_value = %x\n", reg,ltr559_i2c_read_reg(reg));
	}
	else
	{
		APS_DBG("invalid content: '%s', length = %d\n", buf, count);
	}
	return count;    
}


/*----------------------------------------------------------------------------*/
static ssize_t ltr559_show_als_en(struct device_driver *ddri, char *buf, size_t count)
{
	int i,len=0;
	int reg[]={0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,
		0x8d,0x8e,0x8f,0x90,0x91,0x92,0x93,0x94,0x95,0x97,0x98,0x99,0x9a,0x9e};
		
	APS_LOG("E!!\n");
	
	for(i=0;i<27;i++)
		{
		len += snprintf(buf+len, PAGE_SIZE-len, "reg:0x%04X value: 0x%04X\n", reg[i],ltr559_i2c_read_reg(reg[i]));	

	    }
	return len;
}
/*----------------------------------------------------------------------------*/
static ssize_t ltr559_store_als_en(struct device_driver *ddri, char *buf, size_t count)
{
	int ret;//,value;
	
	APS_LOG("E!!\n");
	
	if(!ltr559_obj)
	{
		APS_ERR("ltr559_obj is null!!\n");
		return 0;
	}
	ret = ltr559_als_enable(als_gainrange);
				if(ret < 0)
				{
					APS_ERR("enable als fail: %d\n", ret); 
					
				}
				set_bit(CMC_BIT_ALS, &ltr559_obj->enable);
	
	return count;    
}



/*----------------------------------------------------------------------------*/
static DRIVER_ATTR(als_en,     S_IWUSR | S_IRUGO, ltr559_show_als_en,   ltr559_store_als_en);

static DRIVER_ATTR(als,     S_IWUSR | S_IRUGO, ltr559_show_als,   NULL);
static DRIVER_ATTR(ps,      S_IWUSR | S_IRUGO, ltr559_show_ps,    NULL);
//static DRIVER_ATTR(config,  S_IWUSR | S_IRUGO, ltr559_show_config,ltr559_store_config);
//static DRIVER_ATTR(alslv,   S_IWUSR | S_IRUGO, ltr559_show_alslv, ltr559_store_alslv);
//static DRIVER_ATTR(alsval,  S_IWUSR | S_IRUGO, ltr559_show_alsval,ltr559_store_alsval);
//static DRIVER_ATTR(trace,   S_IWUSR | S_IRUGO,ltr559_show_trace, ltr559_store_trace);
static DRIVER_ATTR(status,  S_IWUSR | S_IRUGO, ltr559_show_status,  ltr559_store_status);
static DRIVER_ATTR(reg,     S_IWUSR | S_IRUGO, ltr559_show_reg,   ltr559_store_reg);
//static DRIVER_ATTR(i2c,     S_IWUSR | S_IRUGO, ltr559_show_i2c,   ltr559_store_i2c);
/*----------------------------------------------------------------------------*/
static struct driver_attribute *ltr559_attr_list[] = {
    &driver_attr_als,
    &driver_attr_ps,    
   // &driver_attr_trace,        /*trace log*/
   // &driver_attr_config,
   // &driver_attr_alslv,
   //&driver_attr_alsval,
    &driver_attr_status,
   //&driver_attr_i2c,
    &driver_attr_reg,
    &driver_attr_als_en,
};
/*----------------------------------------------------------------------------*/
static int ltr559_create_attr(struct driver_attribute *driver) 
{
	int idx, err = 0;
	int num = (int)(sizeof(ltr559_attr_list)/sizeof(ltr559_attr_list[0]));

	APS_LOG("E!!\n");
	
	if (driver == NULL)
	{
		return -EINVAL;
	}

	for(idx = 0; idx < num; idx++)
	{
		err = driver_create_file(driver, ltr559_attr_list[idx]);
		if(err)
		{            
			APS_ERR("driver_create_file (%s) = %d\n", ltr559_attr_list[idx]->attr.name, err);
			break;
		}
	}    
	return err;
}
/*----------------------------------------------------------------------------*/
	static int ltr559_delete_attr(struct device_driver *driver)
	{

	int idx ,err = 0;
	int num = (int)(sizeof(ltr559_attr_list)/sizeof(ltr559_attr_list[0]));

	APS_LOG("E!!\n");
	
	if (!driver)
	return -EINVAL;

	for (idx = 0; idx < num; idx++) 
	{
		driver_remove_file(driver, ltr559_attr_list[idx]);
	}
	
	return err;
}

/*----------------------------------------------------------------------------*/

/* 
 * ###############
 * ## PS CONFIG ##
 * ###############

 */

static int ltr559_ps_set_thres(void)
{
	int res;
	u8 databuf[2];
	
	struct i2c_client *client = ltr559_obj->client;
	struct ltr559_priv *obj = ltr559_obj;
	
	APS_LOG("E!!\n");
	//	APS_FUN();
	APS_DBG("ps_cali.valid: %d\n", ps_cali.valid);
	if(1 == ps_cali.valid)
	{
		databuf[0] = LTR559_PS_THRES_LOW_0; 
		databuf[1] = (u8)(ps_cali.far_away & 0x00FF);
		res = i2c_master_send(client, databuf, 0x2);
		if(res <= 0)
		{
			goto EXIT_ERR;
			return ltr559_ERR_I2C;
		}
		databuf[0] = LTR559_PS_THRES_LOW_1; 
		databuf[1] = (u8)((ps_cali.far_away & 0xFF00) >> 8);
		res = i2c_master_send(client, databuf, 0x2);
		if(res <= 0)
		{
			goto EXIT_ERR;
			return ltr559_ERR_I2C;
		}
		databuf[0] = LTR559_PS_THRES_UP_0;	
		databuf[1] = (u8)(ps_cali.close & 0x00FF);
		res = i2c_master_send(client, databuf, 0x2);
		if(res <= 0)
		{
			goto EXIT_ERR;
			return ltr559_ERR_I2C;
		}
		databuf[0] = LTR559_PS_THRES_UP_1;	
		databuf[1] = (u8)((ps_cali.close & 0xFF00) >> 8);;
		res = i2c_master_send(client, databuf, 0x2);
		if(res <= 0)
		{
			goto EXIT_ERR;
			return ltr559_ERR_I2C;
		}
	}
	else
	{
		databuf[0] = LTR559_PS_THRES_LOW_0; 
		databuf[1] = (u8)((atomic_read(&obj->ps_thd_val_low)) & 0x00FF);
		res = i2c_master_send(client, databuf, 0x2);
		if(res <= 0)
		{
			goto EXIT_ERR;
			return ltr559_ERR_I2C;
		}
		databuf[0] = LTR559_PS_THRES_LOW_1; 
		databuf[1] = (u8)((atomic_read(&obj->ps_thd_val_low )>> 8) & 0x00FF);
		
		res = i2c_master_send(client, databuf, 0x2);
		if(res <= 0)
		{
			goto EXIT_ERR;
			return ltr559_ERR_I2C;
		}
		databuf[0] = LTR559_PS_THRES_UP_0;	
		databuf[1] = (u8)((atomic_read(&obj->ps_thd_val_high)) & 0x00FF);
		res = i2c_master_send(client, databuf, 0x2);
		if(res <= 0)
		{
			goto EXIT_ERR;
			return ltr559_ERR_I2C;
		}
		databuf[0] = LTR559_PS_THRES_UP_1;	
		databuf[1] = (u8)((atomic_read(&obj->ps_thd_val_high) >> 8) & 0x00FF);
		res = i2c_master_send(client, databuf, 0x2);
		if(res <= 0)
		{
			goto EXIT_ERR;
			return ltr559_ERR_I2C;
		}
	
	}

	res = 0;
	return res;
	
	EXIT_ERR:
	APS_ERR("set thres: %d\n", res);
	return res;

}


static int ltr559_ps_enable(int gainrange)
{
	struct i2c_client *client = ltr559_obj->client;
	struct ltr559_priv *obj = ltr559_obj;
	u8 databuf[2];	
	int res;

	int data;
//	hwm_sensor_data sensor_data;	


	int error;
	int setgain;
	
	APS_LOG("E!!\n");
  APS_LOG("ltr559_ps_enable() ...start!\n");

	switch (gainrange) {
		case PS_RANGE16:
			setgain = MODE_PS_ON_Gain16;
			break;

		case PS_RANGE32:
			setgain = MODE_PS_ON_Gain32;
			break;

		case PS_RANGE64:
			setgain = MODE_PS_ON_Gain64;
			break;


		default:
			setgain = MODE_PS_ON_Gain16;
			break;
	}

	APS_LOG("LTR559_PS setgain = %d!\n",setgain);

	error = ltr559_i2c_write_reg(LTR559_PS_CONTR, setgain); 
	if(error<0)
	{
	    APS_LOG("ltr559_ps_enable() error1\n");
	    return error;
	}
	
	mdelay(WAKEUP_DELAY);
    
	/* =============== 
	 * ** IMPORTANT **
	 * ===============
	 * Other settings like timing and threshold to be set here, if required.
 	 * Not set and kept as device default for now.
 	 */
   error = ltr559_i2c_write_reg(LTR559_PS_N_PULSES, 0x0f); 
	if(error<0)
    {
        APS_LOG("ltr559_ps_enable() error2\n");
	    return error;
	} 
	/*error = ltr559_i2c_write_reg(LTR559_PS_LED, 0x63); 
	if(error<0)
    {
        APS_LOG("ltr559_ps_enable() error3...\n");
	    return error;
	}*/
//add by fangyufeng 150810 for d5192_xolo smart motion start
#ifdef LTR559_PS_MEAS_RATE_LOW
#pragma message("LTR559_PS_MEAS_RATE_LOW is used")

  error = ltr559_i2c_write_reg(LTR559_PS_MEAS_RATE, 0x1); 
#else
#pragma message("LTR559_PS_MEAS_RATE_LOW is not used")
error = ltr559_i2c_write_reg(LTR559_PS_MEAS_RATE, 0x8); 
#endif
	if(error<0)
  {
      APS_LOG("ltr559_ps_enable() LTR559_PS_MEAS_RATE\n");
	    return error;
	} 
//add by fangyufeng 150810 for d5192_xolo smart motion end
	data = ltr559_i2c_read_reg(LTR559_PS_CONTR);
	
	#ifdef GN_MTK_BSP_PS_DYNAMIC_CALI  //wangxiqiang
	if (data & 0x02) {

		if(0 == obj->hw->polling_mode_ps){
//			mt_eint_mask(CUST_EINT_ALS_NUM);
				enable_irq(ltr559_obj->irq);
		}
		
		if (ltr559_dynamic_calibrate() < 0)
			return -1;
	}
	#endif	

	/*for interrup work mode support -- by liaoxl.lenovo 12.08.2011*/
		if(0 == obj->hw->polling_mode_ps)
		{		

			ltr559_ps_set_thres();

			#if 1
			databuf[0] = LTR559_INTERRUPT;	
			databuf[1] = 0x01;
			res = i2c_master_send(client, databuf, 0x2);
			if(res <= 0)
			{
				goto EXIT_ERR;
				return ltr559_ERR_I2C;
			}
			
			databuf[0] = LTR559_INTERRUPT_PERSIST;	
			databuf[1] = 0x20;
			res = i2c_master_send(client, databuf, 0x2);
			if(res <= 0)
			{
				goto EXIT_ERR;
				return ltr559_ERR_I2C;
			}
			enable_irq(obj->irq);
	//		mt_eint_unmask(CUST_EINT_ALS_NUM);			
			#endif
	
		}

 	APS_LOG("ltr559_ps_enable ...OK!\n");


 	
	return error;

	EXIT_ERR:
	APS_ERR("set thres: %d\n", res);
	return res;
}

// Put PS into Standby mode
static int ltr559_ps_disable(void)
{
	int error;
	struct ltr559_priv *obj = ltr559_obj;
	
	APS_LOG("E!!\n");
		
	error = ltr559_i2c_write_reg(LTR559_PS_CONTR, MODE_PS_StdBy); 
	if(error<0)
 	    APS_LOG("ltr559_ps_disable ...ERROR\n");
 	else
        APS_LOG("ltr559_ps_disable ...OK\n");

	if(0 == obj->hw->polling_mode_ps)
	{
	//    cancel_work_sync(&obj->eint_work);
	    	disable_irq_nosync(obj->irq);
	//	mt_eint_mask(CUST_EINT_ALS_NUM);
	}
	
	return error;
}


static int ltr559_ps_read(void)
{
	int psval_lo, psval_hi, psdata;
#ifdef LTR556_SW_CALI//lisong test
    struct ltr559_priv *obj = ltr559_obj;
#endif
	
	APS_LOG("E!!\n");
	
	psval_lo = ltr559_i2c_read_reg(LTR559_PS_DATA_0);
	APS_DBG("ps_rawdata_psval_lo = %d\n", psval_lo);
	if (psval_lo < 0){
	    
	    APS_DBG("psval_lo error\n");
		psdata = psval_lo;
		goto out;
	}
	psval_hi = ltr559_i2c_read_reg(LTR559_PS_DATA_1);
    APS_DBG("ps_rawdata_psval_hi = %d\n", psval_hi);

	if (psval_hi < 0){
	    APS_DBG("psval_hi error\n");
		psdata = psval_hi;
		goto out;
	}
	
	psdata = ((psval_hi & 7)* 256) + psval_lo;
#if 0//def LTR556_SW_CALI//lisong test//--modfied by hongguang@wecorp.com
    //psdata = (psdata >> 3);

    if(psdata < obj->ps_cali)
		psdata = 0;
	else
		psdata = psdata - obj->ps_cali;
#endif
    APS_DBG("ps_rawdata = %d\n", psdata);

	prox_val = psdata;
    
	out:
	final_prox_val = psdata;
	
	
	return psdata;
}

/* 
 * ################
 * ## ALS CONFIG ##
 * ################
 */

static int ltr559_als_enable(int gainrange)
{
	int error;
	
	APS_LOG("E!!\n");
	APS_LOG("gainrange = %d\n",gainrange);
	
	switch (gainrange)
	{
		case ALS_RANGE_64K:
			error = ltr559_i2c_write_reg(LTR559_ALS_CONTR, MODE_ALS_ON_Range1);
			break;

		case ALS_RANGE_32K:
			error = ltr559_i2c_write_reg(LTR559_ALS_CONTR, MODE_ALS_ON_Range2);
			break;

		case ALS_RANGE_16K:
			error = ltr559_i2c_write_reg(LTR559_ALS_CONTR, MODE_ALS_ON_Range3);
			break;
			
		case ALS_RANGE_8K:
			error = ltr559_i2c_write_reg(LTR559_ALS_CONTR, MODE_ALS_ON_Range4);
			break;
			
		case ALS_RANGE_1300:
			error = ltr559_i2c_write_reg(LTR559_ALS_CONTR, MODE_ALS_ON_Range5);
			break;

		case ALS_RANGE_600:
			error = ltr559_i2c_write_reg(LTR559_ALS_CONTR, MODE_ALS_ON_Range6);
			break;
			
		default:
			error = ltr559_i2c_write_reg(LTR559_ALS_CONTR, MODE_ALS_ON_Range1);			
			APS_ERR("proxmy sensor gainrange %d!\n", gainrange);
			break;
	}

	msleep(WAKEUP_DELAY);
	//ltr559_i2c_read_reg(LTR559_ALS_CONTR);
	APS_DBG("added by fully reg[0x0080] value: 0x%04X\n", ltr559_i2c_read_reg(LTR559_ALS_CONTR));

	ltr559_i2c_write_reg(LTR559_ALS_MEAS_RATE, 0x08);
	/* =============== 
	 * ** IMPORTANT **
	 * ===============
	 * Other settings like timing and threshold to be set here, if required.
 	 * Not set and kept as device default for now.
 	 */
 	if(error<0)
 	    APS_LOG("ltr559_als_enable ...ERROR\n");
 	else
        APS_LOG("ltr559_als_enable ...OK\n");
        
	return error;
}


// Put ALS into Standby mode
static int ltr559_als_disable(void)
{
	int error;
	
	APS_LOG("E!!\n");
	
	error = ltr559_i2c_write_reg(LTR559_ALS_CONTR, MODE_ALS_StdBy); 
	if(error<0)
 	    APS_LOG("ltr559_als_disable ...ERROR\n");
 	else
        APS_LOG("ltr559_als_disable ...OK\n");
	return error;
}

static int ltr559_als_read(int gainrange)
{
	int alsval_ch0_lo, alsval_ch0_hi, alsval_ch0;
	int alsval_ch1_lo, alsval_ch1_hi, alsval_ch1;
	int  luxdata_int = -1;
	int ratio;
	int als_contr_reg = 0;
	
	als_contr_reg = ltr559_i2c_read_reg(LTR559_ALS_CONTR);
	
	if(	0 == (als_contr_reg & 0x01))
	{
		APS_DBG("added by fully als_value_lux = %d 20171114\n", luxdata_int);
		ltr559_als_enable(als_gainrange);
	}

	APS_LOG("E!!\n");
	
	alsval_ch1_lo = ltr559_i2c_read_reg(LTR559_ALS_DATA_CH1_0);
	alsval_ch1_hi = ltr559_i2c_read_reg(LTR559_ALS_DATA_CH1_1);
	alsval_ch1 = (alsval_ch1_hi * 256) + alsval_ch1_lo;
	APS_DBG("alsval_ch1_lo = %d,alsval_ch1_hi=%d,alsval_ch1=%d\n",alsval_ch1_lo,alsval_ch1_hi,alsval_ch1);

	alsval_ch0_lo = ltr559_i2c_read_reg(LTR559_ALS_DATA_CH0_0);
	alsval_ch0_hi = ltr559_i2c_read_reg(LTR559_ALS_DATA_CH0_1);
	alsval_ch0 = (alsval_ch0_hi * 256) + alsval_ch0_lo;
	APS_DBG("alsval_ch0_lo = %d,alsval_ch0_hi=%d,alsval_ch0=%d\n",alsval_ch0_lo,alsval_ch0_hi,alsval_ch0);
	
    if((alsval_ch1==0)||(alsval_ch0==0))
    {
		msleep(50);
		
		alsval_ch1_lo = ltr559_i2c_read_reg(LTR559_ALS_DATA_CH1_0);
		alsval_ch1_hi = ltr559_i2c_read_reg(LTR559_ALS_DATA_CH1_1);
		alsval_ch1 = (alsval_ch1_hi * 256) + alsval_ch1_lo;
		APS_DBG("alsval_ch1_lo = %d,alsval_ch1_hi=%d,alsval_ch1=%d\n",alsval_ch1_lo,alsval_ch1_hi,alsval_ch1);
		
		alsval_ch0_lo = ltr559_i2c_read_reg(LTR559_ALS_DATA_CH0_0);
		alsval_ch0_hi = ltr559_i2c_read_reg(LTR559_ALS_DATA_CH0_1);
		alsval_ch0 = (alsval_ch0_hi * 256) + alsval_ch0_lo;
		APS_DBG("alsval_ch0_lo = %d,alsval_ch0_hi=%d,alsval_ch0=%d\n",alsval_ch0_lo,alsval_ch0_hi,alsval_ch0);

		
		if((alsval_ch1==0)||(alsval_ch0==0)){

					msleep(50);
		
					alsval_ch1_lo = ltr559_i2c_read_reg(LTR559_ALS_DATA_CH1_0);
					alsval_ch1_hi = ltr559_i2c_read_reg(LTR559_ALS_DATA_CH1_1);
					alsval_ch1 = (alsval_ch1_hi * 256) + alsval_ch1_lo;
					APS_DBG("alsval_ch1_lo = %d,alsval_ch1_hi=%d,alsval_ch1=%d\n",alsval_ch1_lo,alsval_ch1_hi,alsval_ch1);
					
					alsval_ch0_lo = ltr559_i2c_read_reg(LTR559_ALS_DATA_CH0_0);
					alsval_ch0_hi = ltr559_i2c_read_reg(LTR559_ALS_DATA_CH0_1);
					alsval_ch0 = (alsval_ch0_hi * 256) + alsval_ch0_lo;
					APS_DBG("alsval_ch0_lo = %d,alsval_ch0_hi=%d,alsval_ch0=%d\n",alsval_ch0_lo,alsval_ch0_hi,alsval_ch0);

		
					if((alsval_ch1==0)||(alsval_ch0==0)){
							luxdata_int = 0;
					        goto err;
					}			
			}
    }
	ratio = (alsval_ch1*100) /(alsval_ch0+alsval_ch1);
	APS_DBG("ratio = %d  gainrange = %d\n",ratio,gainrange);
	if (ratio < 45){
		luxdata_int = (((17743 * alsval_ch0)+(11059 * alsval_ch1)))/10000;
	}
	else if ((ratio < 64) && (ratio >= 45)){
		luxdata_int = (((42785 * alsval_ch0)-(19548 * alsval_ch1)))/10000;
	}
	else if ((ratio <= 100) && (ratio >= 64)) {
		luxdata_int = (((5926 * alsval_ch0)+(1185 * alsval_ch1)))/10000;
	}
	else {
		luxdata_int = 0;
		}
	
	APS_DBG("als_value_lux = %d\n", luxdata_int);

	
	
	return luxdata_int;

	
err:
	final_lux_val = luxdata_int;
	APS_DBG("err als_value_lux = 0x%x\n", luxdata_int);
	return luxdata_int;
}



/*----------------------------------------------------------------------------*/
int ltr559_get_addr(struct alsps_hw *hw, struct ltr559_i2c_addr *addr)
{
APS_LOG("E!!\n");
	if(!hw || !addr)
	{
		return -EFAULT;
	}
	addr->write_addr= hw->i2c_addr[0];

	return 0;
}


/*-----------------------------------------------------------------------------*/
void ltr559_eint_func(void)
{
	struct ltr559_priv *obj = ltr559_obj;
	
	APS_LOG("E!!\n");
	APS_FUN();
	
	if(!obj)
	{
		return;
	}
	
	schedule_work(&obj->eint_work);
	//schedule_delayed_work(&obj->eint_work,100);
}

#if defined(CONFIG_OF)
static irqreturn_t ltr559_eint_handler(int irq, void *desc)
{
APS_LOG("E!!\n");
	ltr559_eint_func();
	disable_irq_nosync(ltr559_obj->irq);

	return IRQ_HANDLED;
}
#endif

/*----------------------------------------------------------------------------*/
/*for interrup work mode support -- by liaoxl.lenovo 12.08.2011*/
int ltr559_setup_eint(struct i2c_client *client)
{
#if 0
	APS_FUN();
	struct ltr559_priv *obj = (struct ltr559_priv *)i2c_get_clientdata(client);        

	ltr559_obj = obj;
	mt_set_gpio_dir(GPIO_ALS_EINT_PIN, GPIO_DIR_IN);
	mt_set_gpio_mode(GPIO_ALS_EINT_PIN, GPIO_ALS_EINT_PIN_M_EINT);
	mt_set_gpio_pull_enable(GPIO_ALS_EINT_PIN, TRUE);
	mt_set_gpio_pull_select(GPIO_ALS_EINT_PIN, GPIO_PULL_UP);

	mt_eint_set_hw_debounce(CUST_EINT_ALS_NUM, CUST_EINT_ALS_DEBOUNCE_CN);
	mt_eint_registration(CUST_EINT_ALS_NUM, EINTF_TRIGGER_LOW, ltr559_eint_func, 0);
	mt_eint_unmask(CUST_EINT_ALS_NUM);  
    return 0;
#endif

	int ret = 0;
	struct pinctrl *pinctrl = NULL;
	struct pinctrl_state *pins_default = NULL;
	struct pinctrl_state *pins_cfg = NULL;
	u32 ints[2] = {0, 0};

	APS_LOG("E!!\n");
	alspsPltFmDev = get_alsps_platformdev();
/* gpio setting */
	pinctrl = devm_pinctrl_get(&alspsPltFmDev->dev);
	if (IS_ERR(pinctrl)) {
		ret = PTR_ERR(pinctrl);
		APS_ERR("Cannot find alsps pinctrl!\n");
		return ret;
	}
	pins_default = pinctrl_lookup_state(pinctrl, "pin_default");
	if (IS_ERR(pins_default)) {
		ret = PTR_ERR(pins_default);
		APS_ERR("Cannot find alsps pinctrl default!\n");
	}

	pins_cfg = pinctrl_lookup_state(pinctrl, "pin_cfg");
	if (IS_ERR(pins_cfg)) {
		ret = PTR_ERR(pins_cfg);
		APS_ERR("Cannot find alsps pinctrl pin_cfg!\n");
		return ret;
	}
	pinctrl_select_state(pinctrl, pins_cfg);
/* eint request */
	if (ltr559_obj->irq_node) {
		of_property_read_u32_array(ltr559_obj->irq_node, "debounce", ints, ARRAY_SIZE(ints));
		gpio_request(ints[0], "p-sensor");
		gpio_set_debounce(ints[0], ints[1]);
		APS_LOG("ints[0] = %d, ints[1] = %d!!\n", ints[0], ints[1]);

		ltr559_obj->irq = irq_of_parse_and_map(ltr559_obj->irq_node, 0);
		APS_LOG("ltr559_obj->irq = %d\n", ltr559_obj->irq);
		if (!ltr559_obj->irq) {
			APS_ERR("irq_of_parse_and_map fail!!\n");
			return -EINVAL;
		}

		if (request_irq(ltr559_obj->irq, ltr559_eint_handler, IRQF_TRIGGER_NONE, "ALS-eint", NULL)) {
			APS_ERR("IRQ LINE NOT AVAILABLE!!\n");
			return -EINVAL;
		}
		disable_irq_nosync(ltr559_obj->irq);
		enable_irq(ltr559_obj->irq);
	} else {
		APS_ERR("null irq node!!\n");
		return -EINVAL;
	}
	return 0;
}


/*----------------------------------------------------------------------------*/
static void ltr559_power(struct alsps_hw *hw, unsigned int on) 
{
APS_LOG("E!!\n");
#ifdef CONFIG_OF

#else

static unsigned int power_on = 0;

	//APS_LOG("power %s\n", on ? "on" : "off");

	if(hw->power_id != POWER_NONE_MACRO)
	{
		if(power_on == on)
		{
			APS_LOG("ignore power control: %d\n", on);
		}
		else if(on)
		{
			if(!hwPowerOn(hw->power_id, hw->power_vol, "LTR559")) 
			{
				APS_ERR("power on fails!!\n");
			}
		}
		else
		{
			if(!hwPowerDown(hw->power_id, "LTR559")) 
			{
				APS_ERR("power off fail!!\n");   
			}
		}
	}
	power_on = on;
#endif
}

/*----------------------------------------------------------------------------*/
/*for interrup work mode support -- by liaoxl.lenovo 12.08.2011*/
static int ltr559_check_and_clear_intr(struct i2c_client *client) 
{
	int res,intp,intl;
	u8 buffer[2];	
	u8 temp;
	
	APS_LOG("E!!\n");
	APS_FUN();

		//if (mt_get_gpio_in(GPIO_ALS_EINT_PIN) == 1) /*skip if no interrupt*/	
		//	  return 0;
	
		buffer[0] = LTR559_ALS_PS_STATUS;
		res = i2c_master_send(client, buffer, 0x1);
		if(res <= 0)
		{
			goto EXIT_ERR;
		}
		res = i2c_master_recv(client, buffer, 0x1);
		if(res <= 0)
		{
			goto EXIT_ERR;
		}
		temp = buffer[0];
		res = 1;
		intp = 0;
		intl = 0;
		if(0 != (buffer[0] & 0x02))
		{
			res = 0;
			intp = 1;
		}
		if(0 != (buffer[0] & 0x08))
		{
			res = 0;
			intl = 1;		
		}
	
		if(0 == res)
		{
			if((1 == intp) && (0 == intl))
			{
				buffer[1] = buffer[0] & 0xfD;
				
			}
			else if((0 == intp) && (1 == intl))
			{
				buffer[1] = buffer[0] & 0xf7;
			}
			else
			{
				buffer[1] = buffer[0] & 0xf5;
			}
			buffer[0] = LTR559_ALS_PS_STATUS	;
			res = i2c_master_send(client, buffer, 0x2);
			if(res <= 0)
			{
				goto EXIT_ERR;
			}
			else
			{
				res = 0;
			}
		}
	
		return res;
	
	EXIT_ERR:
		APS_ERR("ltr559_check_and_clear_intr fail\n");
		return 1;

}
/*----------------------------------------------------------------------------*/


static int ltr559_check_intr(struct i2c_client *client) 
{


	int res=0,intp=0,intl=0;
	u8 buffer[2]={0};

	APS_LOG("E!!\n");
	//	APS_FUN();
	//if (mt_get_gpio_in(GPIO_ALS_EINT_PIN) == 1) /*skip if no interrupt*/  
	//    return 0;

	buffer[0] = LTR559_ALS_PS_STATUS;
	res = i2c_master_send(client, buffer, 0x1);
	if(res <= 0)
	{
		goto EXIT_ERR;
	}
	res = i2c_master_recv(client, buffer, 0x1);
	if(res <= 0)
	{
		goto EXIT_ERR;
	}
	res = 1;
	intp = 0;
	intl = 0;
	if(0 != (buffer[0] & 0x02))
	{
		res = 0; //Ps int
		intp = 1;
	}
	if(0 != (buffer[0] & 0x08))
	{
		res = 2; //ALS int
		intl = 1;		
	}
	if(0x0A == (buffer[0] & 0x0A))
	{
		res = 4; //ALS & PS int
		intl = 1;		
	}

	/****************check hardware reset	 add start *********************/
	
			if(buffer[0] == 0)
		   {
		  
			  ltr559_devinit();
			  
			  ltr559_ps_enable(ps_gainrange);
			  
		   }
		   
		 
			
		/****************check hardware reset	 add end *********************/




	

	return res;

EXIT_ERR:
	APS_ERR("ltr559_check_intr fail\n");
	return 1;
}

static int ltr559_clear_intr(struct i2c_client *client) 
{
	int res  = 0;
	u8 buffer[2]={0};

	APS_LOG("E!!\n");
	//	APS_FUN();
	
	buffer[0] = LTR559_ALS_PS_STATUS;
	res = i2c_master_send(client, buffer, 0x1);
	if(res <= 0)
	{
		goto EXIT_ERR;
	}
	res = i2c_master_recv(client, buffer, 0x1);
	if(res <= 0)
	{
		goto EXIT_ERR;
	}
	APS_DBG("buffer[0] = %d \n",buffer[0]);
	buffer[1] = buffer[0] & 0x01;
	buffer[0] = LTR559_ALS_PS_STATUS	;

	res = i2c_master_send(client, buffer, 0x2);
	if(res <= 0)
	{
		goto EXIT_ERR;
	}
	else
	{
		res = 0;
	}

	return res;

EXIT_ERR:
	APS_ERR("ltr559_check_and_clear_intr fail\n");
	return 1;
}




static int ltr559_devinit(void)
{
	int res = 0;
	int init_ps_gain = 0;
	int init_als_gain = 0;
	u8 databuf[2] = {0}; 	

	struct i2c_client *client = ltr559_obj->client;

	struct ltr559_priv *obj = ltr559_obj;  
	
	APS_LOG("E!!\n"); 
	
	mdelay(PON_DELAY);

	//soft reset when device init add by steven
	databuf[0] = LTR559_ALS_CONTR;	
	databuf[1] = 0x02;
	res = i2c_master_send(client, databuf, 0x2);
	if(res <= 0)
	{
	APS_LOG("i2c_master_send fail!!\n");
		goto EXIT_ERR;
		return ltr559_ERR_I2C;
	}

	

#if 1
	// Enable PS to Gain4 at startup
	init_ps_gain = PS_RANGE16;
	ps_gainrange = init_ps_gain;

	//res = ltr559_ps_enable(init_ps_gain);//init do not need enable
	//if (res < 0)
	//	goto EXIT_ERR;


	// Enable ALS to Full Range at startup
	init_als_gain = ALS_RANGE_600;
	als_gainrange = init_als_gain;

	//res = ltr559_als_enable(init_als_gain);//init do not need enable
	
	//if (res < 0)
	//	goto EXIT_ERR;

	databuf[0] = LTR559_ALS_MEAS_RATE;	//wangxiqiang
	databuf[1] = 0x08;

#endif
	res = i2c_master_send(client, databuf, 0x2);
	if (res <= 0)
		goto EXIT_ERR;

	/*for interrup work mode support */
	if(0 == obj->hw->polling_mode_ps)
	{	
		APS_LOG("eint enable PS");
		
		if(0 == obj->hw->polling_mode_als)
		{	
			APS_LOG("eint enable ALS");
			databuf[0] = LTR559_ALS_THRES_UP_0;	
			databuf[1] = 0x00;
			res = i2c_master_send(client, databuf, 0x2);
			if(res <= 0)
			{
				goto EXIT_ERR;
				return ltr559_ERR_I2C;
			}
			databuf[0] = LTR559_ALS_THRES_UP_1;	
			databuf[1] = 0x00;
			res = i2c_master_send(client, databuf, 0x2);
			if(res <= 0)
			{
				goto EXIT_ERR;
				return ltr559_ERR_I2C;
			}
			databuf[0] = LTR559_ALS_THRES_LOW_0;	
			databuf[1] = 0x00;
			res = i2c_master_send(client, databuf, 0x2);
			if(res <= 0)
			{
				goto EXIT_ERR;
				return ltr559_ERR_I2C;
			}
			databuf[0] = LTR559_ALS_THRES_LOW_1;	
			databuf[1] = 0x00;
			res = i2c_master_send(client, databuf, 0x2);
			if(res <= 0)
			{
				goto EXIT_ERR;
				return ltr559_ERR_I2C;
			}
			
			databuf[0] = LTR559_INTERRUPT;	
			databuf[1] = 0x03;
			res = i2c_master_send(client, databuf, 0x2);
			if(res <= 0)
			{
				goto EXIT_ERR;
				return ltr559_ERR_I2C;
			}

			databuf[0] = LTR559_INTERRUPT_PERSIST;	
			databuf[1] = 0x20;
			res = i2c_master_send(client, databuf, 0x2);
			if(res <= 0)
			{
				goto EXIT_ERR;
				return ltr559_ERR_I2C;
			}

		}
		else
		{
		
		
			databuf[0] = LTR559_INTERRUPT;	
			databuf[1] = 0x01;
			res = i2c_master_send(client, databuf, 0x2);
			if(res <= 0)
			{
				goto EXIT_ERR;
				return ltr559_ERR_I2C;
			}

			databuf[0] = LTR559_INTERRUPT_PERSIST;	
			databuf[1] = 0x20;
			res = i2c_master_send(client, databuf, 0x2);
			if(res <= 0)
			{
				goto EXIT_ERR;
				return ltr559_ERR_I2C;
			}
		}
		ltr559_ps_set_thres();

	}

	if((res = ltr559_setup_eint(client))!=0)
	{
		APS_ERR("setup eint: %d\n", res);
		return res;
	}
	
	if((res = ltr559_check_and_clear_intr(client)))
	{
		APS_ERR("check/clear intr: %d\n", res);
		//    return res;
	}

	res = 0;

	EXIT_ERR:
	APS_ERR("init dev: %d\n", res);
	return res;

}
/*----------------------------------------------------------------------------*/


static int ltr559_get_als_value(struct ltr559_priv *obj, u16 als)
{

	int idx;
	int invalid = 0;
	
	APS_LOG("E!!\n");
	APS_DBG("als  = %d\n",als);
	 
	for(idx = 0; idx < obj->als_level_num; idx++)
	{
		if(als < obj->hw->als_level[idx])
		{
			break;
		}
	}
	
	if(idx >= obj->als_value_num)
	{
		APS_ERR("exceed range\n"); 
		idx = obj->als_value_num - 1;
	}
	
	if(1 == atomic_read(&obj->als_deb_on))
	{
		unsigned long endt = atomic_read(&obj->als_deb_end);
		if(time_after(jiffies, endt))
		{
			atomic_set(&obj->als_deb_on, 0);
		}
		
		if(1 == atomic_read(&obj->als_deb_on))
		{
			invalid = 1;
		}
	}

	if(!invalid)
	{
		APS_DBG("ALS: %05d => %05d\n", als, obj->hw->als_value[idx]);	
		return obj->hw->als_value[idx];
	}
	else
	{
		APS_ERR("ALS: %05d => %05d (-1)\n", als, obj->hw->als_value[idx]);    
		return -1;
	}
}
/*----------------------------------------------------------------------------*/
static int ltr559_get_ps_value(struct ltr559_priv *obj, u16 ps)
{
	int val,  mask = atomic_read(&obj->ps_mask);
	int invalid = 0;
	static int val_temp = 1;
	
	APS_LOG("E!!\n");
	APS_LOG("ps_thd_val_high = %d\t,ps_thd_val_low = %d\n",atomic_read(&obj->ps_thd_val_high),atomic_read(&obj->ps_thd_val_low));
	
	if((ps > atomic_read(&obj->ps_thd_val_high)))
	{
		val = 0;  /*close*/
		val_temp = 0;
		intr_flag_value = 1;
	}
			//else if((ps < atomic_read(&obj->ps_thd_val_low))&&(temp_ps[0]  < atomic_read(&obj->ps_thd_val_low)))
	else if((ps < atomic_read(&obj->ps_thd_val_low)))
	{
		val = 1;  /*far away*/
		val_temp = 1;
		intr_flag_value = 0;
	}
	else
		val = val_temp;	
			
	
	if(atomic_read(&obj->ps_suspend))
	{
		invalid = 1;
	}
	else if(1 == atomic_read(&obj->ps_deb_on))
	{
		unsigned long endt = atomic_read(&obj->ps_deb_end);
		if(time_after(jiffies, endt))
		{
			atomic_set(&obj->ps_deb_on, 0);
		}
		
		if (1 == atomic_read(&obj->ps_deb_on))
		{
			invalid = 1;
		}
	}
	else if (obj->als > 50000)
	{
		//invalid = 1;
		APS_DBG("ligh too high will result to failt proximiy\n");
		return 1;  /*far away*/
	}

	if(!invalid)
	{
		APS_DBG("PS:  %05d => %05d\n", ps, val);
		return val;
	}	
	else
	{
		APS_DBG("PS value is nor valid!\n");
		return -1;
	}	
}

/*----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------*/
/*for interrup work mode support */
static void ltr559_eint_work(struct work_struct *work)
{
	struct ltr559_priv *obj = (struct ltr559_priv *)container_of(work, struct ltr559_priv, eint_work);
	int res = 0;
	int ps_value = -1;
	int temp_noise = 0;
	u8 databuf[2] = {0};
	
	APS_LOG("E!!\n");
	
	res = ltr559_check_intr(obj->client);
	if (res <  0) {
		goto EXIT_INTR_ERR;
	}// else {
	//	APS_LOG("ltr559 interrupt value = %d\n", intr_flag);
	//	res = ps_report_interrupt_data(ltr559_get_ps_value(obj,ltr559_ps_read()));
	//}
	else if(res==2)
	{
		APS_ERR("get sensor als data !\n");
		//sensor_data = (hwm_sensor_data *)buff_out;
	#if 0	
		obj->als = ltr559_als_read(als_gainrange);
        #if defined(MTK_AAL_SUPPORT)
		sensor_data.values[0] = obj->als;
		#else
		sensor_data.values[0] = ltr559_get_als_value(obj, obj->als);
		#endif
		sensor_data.value_divide = 1;
		sensor_data.status = SENSOR_STATUS_ACCURACY_MEDIUM;
	#endif
	}
	else if(res==0||res==4)
	{
		//get raw data
		obj->ps = ltr559_ps_read();
    		if(obj->ps < 0)
    		{
    			res = -1;
    			return;
    		}
				
		APS_DBG("ltr559_eint_work rawdata ps=%d als_ch0=%d!\n",obj->ps,obj->als);
		ps_value = ltr559_get_ps_value(obj, obj->ps);
		//sensor_data.values[1] = obj->ps;
	//	sensor_data.value_divide = 1;
	//	sensor_data.status = SENSOR_STATUS_ACCURACY_MEDIUM;			
/*singal interrupt function add*/
		APS_DBG("intr_flag_value=%d\n",intr_flag_value);
	#if 1
		if(intr_flag_value){
			APS_DBG(" interrupt value ps will < ps_threshold_low");

			databuf[0] = LTR559_PS_THRES_LOW_0;	
			databuf[1] = (u8)((atomic_read(&obj->ps_thd_val_low)) & 0x00FF);
			res = i2c_master_send(obj->client, databuf, 0x2);
			if(res <= 0)
			{
				return;
			}
			databuf[0] = LTR559_PS_THRES_LOW_1;	
			databuf[1] = (u8)(((atomic_read(&obj->ps_thd_val_low)) & 0xFF00) >> 8);
			res = i2c_master_send(obj->client, databuf, 0x2);
			if(res <= 0)
			{
				return;
			}
			databuf[0] = LTR559_PS_THRES_UP_0;	
			databuf[1] = (u8)(0x00FF);
			res = i2c_master_send(obj->client, databuf, 0x2);
			if(res <= 0)
			{
				return;
			}
			databuf[0] = LTR559_PS_THRES_UP_1; 
			databuf[1] = (u8)((0xFF00) >> 8);;
			res = i2c_master_send(obj->client, databuf, 0x2);
			APS_DBG("obj->ps_thd_val_low=%ld !\n",obj->ps_thd_val_low);
//			if(res <= 0)
	//		{
	//			return;
	//		}
		}
		else{	
//#ifndef LTR556_SW_CALI//lisong test		//--modfied by hongguang@wecorp.com		
	//		if(obj->ps > 20 && obj->ps < (dynamic_calibrate - 50)){ 
			if(obj->ps < 100){			
				atomic_set(&obj->ps_thd_val_high,  obj->ps+100);
				atomic_set(&obj->ps_thd_val_low, obj->ps+70);
			}else if(obj->ps < 200){
				atomic_set(&obj->ps_thd_val_high,  obj->ps+110);
				atomic_set(&obj->ps_thd_val_low, obj->ps+80);
			}else if(obj->ps < 300){
				atomic_set(&obj->ps_thd_val_high,  obj->ps+150);
				atomic_set(&obj->ps_thd_val_low, obj->ps+100);
			}else if(obj->ps < 400){
				atomic_set(&obj->ps_thd_val_high,  obj->ps+150);
				atomic_set(&obj->ps_thd_val_low, obj->ps+100);
			}else if(obj->ps < 600){
				atomic_set(&obj->ps_thd_val_high,  obj->ps+200);
				atomic_set(&obj->ps_thd_val_low, obj->ps+150);
			}else if(obj->ps < 1000){
				atomic_set(&obj->ps_thd_val_high,  obj->ps+250);
				atomic_set(&obj->ps_thd_val_low, obj->ps+200);	
			}else if(obj->ps < 1650){
				atomic_set(&obj->ps_thd_val_high,  obj->ps+300);
				atomic_set(&obj->ps_thd_val_low, obj->ps+250);
			}
			else{
				atomic_set(&obj->ps_thd_val_high,  1800);
				atomic_set(&obj->ps_thd_val_low, 1700);
				printk(KERN_ERR "ltr559 the proximity sensor structure is error\n");
			}
		
			dynamic_calibrate = obj->ps;

		//	}	

			if(obj->ps	> 50){
				temp_noise = obj->ps - 50;
			}else{
				temp_noise = 0;
			}
//#endif
			//wake_lock_timeout(&ps_wake_lock,ps_wakeup_timeout*HZ);
			databuf[0] = LTR559_PS_THRES_LOW_0; 
			databuf[1] = (u8)(0 & 0x00FF);//get the noise one time 
			res = i2c_master_send(obj->client, databuf, 0x2);
			if(res <= 0)
			{
				return;
			}
			databuf[0] = LTR559_PS_THRES_LOW_1; 
			databuf[1] = (u8)((0 & 0xFF00) >> 8);
			res = i2c_master_send(obj->client, databuf, 0x2);
			if(res <= 0)
			{
				return;
			}
			databuf[0] = LTR559_PS_THRES_UP_0;	
			databuf[1] = (u8)((atomic_read(&obj->ps_thd_val_high)) & 0x00FF);
			res = i2c_master_send(obj->client, databuf, 0x2);
			if(res <= 0)
			{
				return;
			}
			databuf[0] = LTR559_PS_THRES_UP_1; 
			databuf[1] = (u8)(((atomic_read(&obj->ps_thd_val_high)) & 0xFF00) >> 8);;
			res = i2c_master_send(obj->client, databuf, 0x2);
//			APS_DBG("obj->ps_thd_val_high=%ld !\n",obj->ps_thd_val_high);
//			if(res <= 0)
//			{			
//				return;
//			}
		}
	#endif
		res = ps_report_interrupt_data(ps_value);
	//	sensor_data.value_divide = 1;
	//	sensor_data.status = SENSOR_STATUS_ACCURACY_MEDIUM;
		//let up layer to know
	//	if((err = hwmsen_get_interrupt_data(ID_PROXIMITY, &sensor_data)))
	//	{
	//	  APS_ERR("call hwmsen_get_interrupt_data fail = %d\n", err);
	//	}
	}
	ltr559_clear_intr(obj->client);
#if defined(CONFIG_OF)
	enable_irq(obj->irq);
#elif defined(CUST_EINT_ALS_TYPE)
	mt_eint_unmask(CUST_EINT_ALS_NUM);
#else
	mt65xx_eint_unmask(CUST_EINT_ALS_NUM);
#endif
	return;
 EXIT_INTR_ERR:
#if defined(CONFIG_OF)
	enable_irq(obj->irq);
#elif defined(CUST_EINT_ALS_TYPE)
	mt_eint_unmask(CUST_EINT_ALS_NUM);
#else
	mt65xx_eint_unmask(CUST_EINT_ALS_NUM);
#endif
	APS_ERR("ltr559_eint_work err: %d\n", res);
#if 0
//	hwm_sensor_data sensor_data;
	int temp_noise;	 
//	u8 buffer[1];
//	u8 reg_value[1];
	u8 databuf[2];
	int res = 0;
	APS_FUN();
	err = ltr559_check_intr(obj->client);
	if(err < 0)
	{
		APS_ERR("ltr559_eint_work check intrs: %d\n", err);
	}
	else if(err==2)
	{
		APS_ERR("get sensor als data !\n");
		//sensor_data = (hwm_sensor_data *)buff_out;
		obj->als = ltr559_als_read(als_gainrange);
        #if defined(MTK_AAL_SUPPORT)
		sensor_data.values[0] = obj->als;
		#else
		sensor_data.values[0] = ltr559_get_als_value(obj, obj->als);
		#endif
		sensor_data.value_divide = 1;
		sensor_data.status = SENSOR_STATUS_ACCURACY_MEDIUM;
	}
	else if(err==0||err==4)
	{
		//get raw data
		obj->ps = ltr559_ps_read();
    		if(obj->ps < 0)
    		{
    			err = -1;
    			return;
    		}
				
		APS_DBG("ltr559_eint_work rawdata ps=%d als_ch0=%d!\n",obj->ps,obj->als);
		sensor_data.values[0] = ltr559_get_ps_value(obj, obj->ps);
		//sensor_data.values[1] = obj->ps;
		sensor_data.value_divide = 1;
		sensor_data.status = SENSOR_STATUS_ACCURACY_MEDIUM;			
/*singal interrupt function add*/
		APS_DBG("intr_flag_value=%d\n",intr_flag_value);
		if(intr_flag_value){
			APS_DBG(" interrupt value ps will < ps_threshold_low");

			databuf[0] = LTR559_PS_THRES_LOW_0;	
			databuf[1] = (u8)((atomic_read(&obj->ps_thd_val_low)) & 0x00FF);
			res = i2c_master_send(obj->client, databuf, 0x2);
			if(res <= 0)
			{
				return;
			}
			databuf[0] = LTR559_PS_THRES_LOW_1;	
			databuf[1] = (u8)(((atomic_read(&obj->ps_thd_val_low)) & 0xFF00) >> 8);
			res = i2c_master_send(obj->client, databuf, 0x2);
			if(res <= 0)
			{
				return;
			}
			databuf[0] = LTR559_PS_THRES_UP_0;	
			databuf[1] = (u8)(0x00FF);
			res = i2c_master_send(obj->client, databuf, 0x2);
			if(res <= 0)
			{
				return;
			}
			databuf[0] = LTR559_PS_THRES_UP_1; 
			databuf[1] = (u8)((0xFF00) >> 8);;
			res = i2c_master_send(obj->client, databuf, 0x2);
			//APS_DBG("obj->ps_thd_val_low=%ld !\n",obj->ps_thd_val_low);
			if(res <= 0)
			{
				return;
			}
		}
		else{	
//#ifndef LTR556_SW_CALI//lisong test		//--modfied by hongguang@wecorp.com		
			if(obj->ps > 20 && obj->ps < (dynamic_calibrate - 50)){ 
			if(obj->ps < 100){			
				atomic_set(&obj->ps_thd_val_high,  obj->ps+100);
				atomic_set(&obj->ps_thd_val_low, obj->ps+70);
			}else if(obj->ps < 200){
				atomic_set(&obj->ps_thd_val_high,  obj->ps+110);
				atomic_set(&obj->ps_thd_val_low, obj->ps+80);
			}else if(obj->ps < 300){
				atomic_set(&obj->ps_thd_val_high,  obj->ps+150);
				atomic_set(&obj->ps_thd_val_low, obj->ps+100);
			}else if(obj->ps < 400){
				atomic_set(&obj->ps_thd_val_high,  obj->ps+150);
				atomic_set(&obj->ps_thd_val_low, obj->ps+100);
			}else if(obj->ps < 600){
				atomic_set(&obj->ps_thd_val_high,  obj->ps+200);
				atomic_set(&obj->ps_thd_val_low, obj->ps+150);
			}else if(obj->ps < 1000){
				atomic_set(&obj->ps_thd_val_high,  obj->ps+250);
				atomic_set(&obj->ps_thd_val_low, obj->ps+200);	
			}else if(obj->ps < 1650){
				atomic_set(&obj->ps_thd_val_high,  obj->ps+300);
				atomic_set(&obj->ps_thd_val_low, obj->ps+250);
			}
			else{
				atomic_set(&obj->ps_thd_val_high,  1800);
				atomic_set(&obj->ps_thd_val_low, 1700);
				printk(KERN_ERR "ltr559 the proximity sensor structure is error\n");
			}
		
			dynamic_calibrate = obj->ps;

			}	

			if(obj->ps	> 50){
				temp_noise = obj->ps - 50;
			}else{
				temp_noise = 0;
			}
//#endif
			//wake_lock_timeout(&ps_wake_lock,ps_wakeup_timeout*HZ);
			databuf[0] = LTR559_PS_THRES_LOW_0; 
			databuf[1] = (u8)(0 & 0x00FF);//get the noise one time 
			res = i2c_master_send(obj->client, databuf, 0x2);
			if(res <= 0)
			{
				return;
			}
			databuf[0] = LTR559_PS_THRES_LOW_1; 
			databuf[1] = (u8)((0 & 0xFF00) >> 8);
			res = i2c_master_send(obj->client, databuf, 0x2);
			if(res <= 0)
			{
				return;
			}
			databuf[0] = LTR559_PS_THRES_UP_0;	
			databuf[1] = (u8)((atomic_read(&obj->ps_thd_val_high)) & 0x00FF);
			res = i2c_master_send(obj->client, databuf, 0x2);
			if(res <= 0)
			{
				return;
			}
			databuf[0] = LTR559_PS_THRES_UP_1; 
			databuf[1] = (u8)(((atomic_read(&obj->ps_thd_val_high)) & 0xFF00) >> 8);;
			res = i2c_master_send(obj->client, databuf, 0x2);
//			APS_DBG("obj->ps_thd_val_high=%ld !\n",obj->ps_thd_val_high);
			if(res <= 0)
			{
				return;
			}
		}
		
	//	sensor_data.value_divide = 1;
	//	sensor_data.status = SENSOR_STATUS_ACCURACY_MEDIUM;
		//let up layer to know
	//	if((err = hwmsen_get_interrupt_data(ID_PROXIMITY, &sensor_data)))
	//	{
	//	  APS_ERR("call hwmsen_get_interrupt_data fail = %d\n", err);
	//	}
	}
	ltr559_clear_intr(obj->client);
    mt_eint_unmask(CUST_EINT_ALS_NUM);       
#endif

}



/****************************************************************************** 
 * Function Configuration
******************************************************************************/
static int ltr559_open(struct inode *inode, struct file *file)
{
APS_LOG("E!!\n");
	file->private_data = ltr559_i2c_client;

	if (!file->private_data)
	{
		APS_ERR("null pointer!!\n");
		return -EINVAL;
	}
	
	return nonseekable_open(inode, file);
}
/*----------------------------------------------------------------------------*/
static int ltr559_release(struct inode *inode, struct file *file)
{
APS_LOG("E!!\n");
	file->private_data = NULL;
	return 0;
}
/*----------------------------------------------------------------------------*/


static int ltr559_unlocked_ioctl(struct file *file, unsigned int cmd,
       unsigned long arg)       
{
	struct i2c_client *client = (struct i2c_client*)file->private_data;
	struct ltr559_priv *obj = i2c_get_clientdata(client);  
	int err = 0;
	void __user *ptr = (void __user*) arg;
	int dat;
	uint32_t enable;
	int threshold[2];
	int ps_cali;
	int ps_result;
	
	APS_LOG("E!!\n");
	APS_DBG("cmd= %d\n", cmd);
	
	switch (cmd)
	{
		case ALSPS_SET_PS_MODE:
			APS_LOG("ALSPS_SET_PS_MODE!!\n");
			if(copy_from_user(&enable, ptr, sizeof(enable)))
			{
				err = -EFAULT;
				goto err_out;
			}
			if(enable)
			{
			    err = ltr559_ps_enable(ps_gainrange);
				if(err < 0)
				{
					APS_ERR("enable ps fail: %d\n", err); 
					goto err_out;
				}
				set_bit(CMC_BIT_PS, &obj->enable);
			}
			else
			{
			    err = ltr559_ps_disable();
				if(err < 0)
				{
					APS_ERR("disable ps fail: %d\n", err); 
					goto err_out;
				}
				
				clear_bit(CMC_BIT_PS, &obj->enable);
			}
			break;

		case ALSPS_GET_PS_MODE:
				APS_LOG("ALSPS_GET_PS_MODE!!\n");
			enable = test_bit(CMC_BIT_PS, &obj->enable) ? (1) : (0);
			if(copy_to_user(ptr, &enable, sizeof(enable)))
			{
				err = -EFAULT;
				goto err_out;
			}
			break;

		case ALSPS_GET_PS_DATA:
			APS_DBG("ALSPS_GET_PS_DATA\n"); 
		    obj->ps = ltr559_ps_read();
			if(obj->ps < 0)
			{
				goto err_out;
			}
			
			dat = ltr559_get_ps_value(obj, obj->ps);
			if(copy_to_user(ptr, &dat, sizeof(dat)))
			{
				err = -EFAULT;
				goto err_out;
			}  
			break;

		case ALSPS_GET_PS_RAW_DATA:
				APS_LOG("ALSPS_GET_PS_RAW_DATA!!\n");


			
			/****************check hardware reset	 add start *********************/
				/*
					if(0 == ltr559_i2c_read_reg(0x8c))
					   {
					  
						  ltr559_devinit();
						  
						  ltr559_ps_enable(ps_gainrange);
						  
					   }
					*/   
					 
						
			/****************check hardware reset	 add end *********************/
			
			obj->ps = ltr559_ps_read();
			if(obj->ps < 0)
			{
				goto err_out;
			}
			dat = obj->ps;
			
			if(copy_to_user(ptr, &dat, sizeof(dat)))
			{
				err = -EFAULT;
				goto err_out;
			}  
			break;

		case ALSPS_SET_ALS_MODE:
				APS_LOG("ALSPS_SET_ALS_MODE!!\n");
			if(copy_from_user(&enable, ptr, sizeof(enable)))
			{
				err = -EFAULT;
				goto err_out;
			}
			if(enable)
			{
			    err = ltr559_als_enable(als_gainrange);
				if(err < 0)
				{
					APS_ERR("enable als fail: %d\n", err); 
					goto err_out;
				}
				set_bit(CMC_BIT_ALS, &obj->enable);
			}
			else
			{
			    err = ltr559_als_disable();
				if(err < 0)
				{
					APS_ERR("disable als fail: %d\n", err); 
					goto err_out;
				}
				clear_bit(CMC_BIT_ALS, &obj->enable);
			}
			break;

		case ALSPS_GET_ALS_MODE:
				APS_LOG("ALSPS_GET_ALS_MODE!!\n");
			enable = test_bit(CMC_BIT_ALS, &obj->enable) ? (1) : (0);
			if(copy_to_user(ptr, &enable, sizeof(enable)))
			{
				err = -EFAULT;
				goto err_out;
			}
			break;

		case ALSPS_GET_ALS_DATA: 
				APS_LOG("ALSPS_GET_ALS_DATA!!\n");
		    obj->als = ltr559_als_read(als_gainrange);
			if(obj->als < 0)
			{
				goto err_out;
			}

			dat = ltr559_get_als_value(obj, obj->als);
			if(copy_to_user(ptr, &dat, sizeof(dat)))
			{
				err = -EFAULT;
				goto err_out;
			}              
			break;

		case ALSPS_GET_ALS_RAW_DATA: 
	APS_LOG("ALSPS_GET_ALS_RAW_DATA!!\n");
			err = ltr559_als_enable(als_gainrange);
			if(err < 0)
			{
				APS_ERR("enable als fail: %d\n", err); 
				goto err_out;
			}
			set_bit(CMC_BIT_ALS, &obj->enable);			
			
			obj->als = ltr559_als_read(als_gainrange);
			if(obj->als < 0)
			{
				goto err_out;
			}

			dat = obj->als;
			if(copy_to_user(ptr, &dat, sizeof(dat)))
			{
				err = -EFAULT;
				goto err_out;
			}              
			break;
#ifdef LTR556_SW_CALI//lisong test
		case ALSPS_GET_PS_TEST_RESULT:
				APS_LOG("ALSPS_GET_PS_TEST_RESULT!!\n");
            obj->ps = ltr559_ps_read();
            if(obj->ps < 0)
			{
				goto err_out;
			}
            
			if(obj->ps > (atomic_read(&obj->ps_thd_val_high) - obj->ps_cali))
			{
				ps_result = 0;
			}
			else	
                ps_result = 1;
			
			if(copy_to_user(ptr, &ps_result, sizeof(ps_result)))
			{
				err = -EFAULT;
				goto err_out;
			}			 

				break;


		case ALSPS_IOCTL_CLR_CALI:
			APS_LOG("ALSPS_IOCTL_CLR_CALI!!\n");
			if(copy_from_user(&dat, ptr, sizeof(dat)))
			{
				err = -EFAULT;
				goto err_out;
			}
			if(dat == 0)
				obj->ps_cali = 0;

            atomic_set(&obj->ps_thd_val_high, (obj->hw->ps_threshold_high));
            atomic_set(&obj->ps_thd_val_low,  (obj->hw->ps_threshold_low));//need to confirm

            ltr559_ps_set_thres();

			break;			

		case ALSPS_IOCTL_GET_CALI:
				APS_LOG("ALSPS_IOCTL_GET_CALI!!\n");
			ps_cali = obj->ps_cali ;
			if(copy_to_user(ptr, &ps_cali, sizeof(ps_cali)))
			{
				err = -EFAULT;
				goto err_out;
			}
			break;
			
		case ALSPS_IOCTL_SET_CALI:
				APS_LOG("ALSPS_IOCTL_SET_CALI!!\n");
            if(copy_from_user(&ps_cali, ptr, sizeof(ps_cali)))
            {
            	err = -EFAULT;
            	goto err_out;
            }

            obj->ps_cali = ps_cali;

			if(obj->ps_cali < (int)(obj->hw->ps_threshold_low / 2 - 20))
			{				
				atomic_set(&obj->ps_thd_val_high,  (obj->hw->ps_threshold_high-obj->ps_cali));
				atomic_set(&obj->ps_thd_val_low,  (obj->hw->ps_threshold_low-obj->ps_cali));
			}
			else
			{
				atomic_set(&obj->ps_thd_val_high, (obj->ps_cali+120));
				atomic_set(&obj->ps_thd_val_low,  (obj->ps_cali+48));
			}    

            ltr559_ps_set_thres();
        
            break;	

        case ALSPS_SET_PS_THRESHOLD:
				APS_LOG("ALSPS_SET_PS_THRESHOLD!!\n");
            break;
                
		case ALSPS_GET_PS_THRESHOLD_HIGH:
				APS_LOG("ALSPS_GET_PS_THRESHOLD_HIGH!!\n");
			//threshold[0] = atomic_read(&obj->ps_thd_val_high) - obj->ps_cali;
			threshold[0] = atomic_read(&obj->ps_thd_val_high);
			APS_ERR("%s get threshold high: 0x%x\n", __func__, threshold[0]); 
			if(copy_to_user(ptr, &threshold[0], sizeof(threshold[0])))
			{
				err = -EFAULT;
				goto err_out;
			}
			break;	
            
		case ALSPS_GET_PS_THRESHOLD_LOW:
				APS_LOG("ALSPS_GET_PS_THRESHOLD_LOW!!\n");
            //threshold[0] = atomic_read(&obj->ps_thd_val_low) - obj->ps_cali;
            threshold[0] = atomic_read(&obj->ps_thd_val_low);
            APS_ERR("%s get threshold low: 0x%x\n", __func__, threshold[0]); 
            if(copy_to_user(ptr, &threshold[0], sizeof(threshold[0])))
            {
            	err = -EFAULT;
            	goto err_out;
            }
            break;			
#endif
		default:
			APS_LOG("default!!\n");
			APS_ERR("%s not supported = 0x%04x", __FUNCTION__, cmd);
			err = -ENOIOCTLCMD;
			break;
	}

	err_out:
	return err;    
}

/*----------------------------------------------------------------------------*/
static struct file_operations ltr559_fops = {
	//.owner = THIS_MODULE,
	.open = ltr559_open,
	.release = ltr559_release,
	.unlocked_ioctl = ltr559_unlocked_ioctl,
};
/*----------------------------------------------------------------------------*/
static struct miscdevice ltr559_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "als_ps",
	.fops = &ltr559_fops,
};

static int ltr559_i2c_suspend(struct i2c_client *client, pm_message_t msg) 
{
	/*early_suspend is only applied for ALS*/
	struct ltr559_priv *obj = ltr559_obj;   
	int err;
	
	APS_LOG("E!!\n");
	APS_FUN();    

	if(!obj)
	{
		APS_ERR("null pointer!!\n");
		return -1;
	}
	
	atomic_set(&obj->als_suspend, 1); 
	err = ltr559_als_disable();
	if(err < 0)
	{
		APS_ERR("disable als fail: %d\n", err); 
	}
	APS_FUN();    	
	return 0;
}
/*----------------------------------------------------------------------------*/
static int ltr559_i2c_resume(struct i2c_client *client)
{
	struct ltr559_priv *obj = ltr559_obj;         
	int err;
	
	APS_LOG("E!!\n");
	APS_FUN();

	if(!obj)
	{
		APS_ERR("null pointer!!\n");
		return -1;
	}

	atomic_set(&obj->als_suspend, 0);
	if(test_bit(CMC_BIT_ALS, &obj->enable))
	{
	    err = ltr559_als_enable(als_gainrange);
		if(err < 0)
		{
			APS_ERR("enable als fail: %d\n", err);        

		}
	}

	APS_FUN();
	return 0;
}
#if 0
static void ltr559_early_suspend(struct early_suspend *h) 
{   /*early_suspend is only applied for ALS*/
	struct ltr559_priv *obj = container_of(h, struct ltr559_priv, early_drv);   
	int err;
	APS_FUN();    

	if(!obj)
	{
		APS_ERR("null pointer!!\n");
		return;
	}
	
	atomic_set(&obj->als_suspend, 1); 
	err = ltr559_als_disable();
	if(err < 0)
	{
		APS_ERR("disable als fail: %d\n", err); 
	}
}

static void ltr559_late_resume(struct early_suspend *h)
{   /*early_suspend is only applied for ALS*/
	struct ltr559_priv *obj = container_of(h, struct ltr559_priv, early_drv);         
	int err;
	APS_FUN();

	if(!obj)
	{
		APS_ERR("null pointer!!\n");
		return;
	}

	atomic_set(&obj->als_suspend, 0);
	if(test_bit(CMC_BIT_ALS, &obj->enable))
	{
	    err = ltr559_als_enable(als_gainrange);
		if(err < 0)
		{
			APS_ERR("enable als fail: %d\n", err);        

		}
	}
}

int ltr559_ps_operate(void* self, uint32_t command, void* buff_in, int size_in,
		void* buff_out, int size_out, int* actualout)
{
	int err = 0;
	int value;
	hwm_sensor_data* sensor_data;
	struct ltr559_priv *obj = (struct ltr559_priv *)self;
	
	switch (command)
	{
		case SENSOR_DELAY:
			if((buff_in == NULL) || (size_in < sizeof(int)))
			{
				APS_ERR("Set delay parameter error!\n");
				err = -EINVAL;
			}
			// Do nothing
			break;

		case SENSOR_ENABLE:
			if((buff_in == NULL) || (size_in < sizeof(int)))
			{
				APS_ERR("Enable sensor parameter error!\n");
				err = -EINVAL;
			}
			else
			{				
				value = *(int *)buff_in;
				if(value)
				{
				    err = ltr559_ps_enable(ps_gainrange);
					if(err < 0)
					{
						APS_ERR("enable ps fail: %d\n", err); 
						return -1;
					}
					set_bit(CMC_BIT_PS, &obj->enable);
				}
				else
				{
				    err = ltr559_ps_disable();
					if(err < 0)
					{
						APS_ERR("disable ps fail: %d\n", err); 
						return -1;
					}
					clear_bit(CMC_BIT_PS, &obj->enable);
				}
			}
			break;

		case SENSOR_GET_DATA:
			if((buff_out == NULL) || (size_out< sizeof(hwm_sensor_data)))
			{
				APS_ERR("get sensor data parameter error!\n");
				err = -EINVAL;
			}
			else
			{
				APS_ERR("get sensor ps data !\n");
				sensor_data = (hwm_sensor_data *)buff_out;
				obj->ps = ltr559_ps_read();
    			if(obj->ps < 0)
    			{
    				err = -1;
    				break;
    			}
				sensor_data->values[0] = ltr559_get_ps_value(obj, obj->ps);
				//sensor_data->values[1] = obj->ps;		//steven polling mode *#*#3646633#*#*
				sensor_data->value_divide = 1;
				sensor_data->status = SENSOR_STATUS_ACCURACY_MEDIUM;			
			}
			break;
		default:
			APS_ERR("proxmy sensor operate function no this parameter %d!\n", command);
			err = -1;
			break;
	}
	
	return err;
}

int ltr559_als_operate(void* self, uint32_t command, void* buff_in, int size_in,
		void* buff_out, int size_out, int* actualout)
{
	int err = 0;
	int value;
	hwm_sensor_data* sensor_data;
	struct ltr559_priv *obj = (struct ltr559_priv *)self;

	switch (command)
	{
		case SENSOR_DELAY:
			if((buff_in == NULL) || (size_in < sizeof(int)))
			{
				APS_ERR("Set delay parameter error!\n");
				err = -EINVAL;
			}
			// Do nothing
			break;

		case SENSOR_ENABLE:
			if((buff_in == NULL) || (size_in < sizeof(int)))
			{
				APS_ERR("Enable sensor parameter error!\n");
				err = -EINVAL;
			}
			else
			{
				value = *(int *)buff_in;				
				if(value)
				{
				    err = ltr559_als_enable(als_gainrange);
					if(err < 0)
					{
						APS_ERR("enable als fail: %d\n", err); 
						return -1;
					}
					set_bit(CMC_BIT_ALS, &obj->enable);
				}
				else
				{
				    err = ltr559_als_disable();
					if(err < 0)
					{
						APS_ERR("disable als fail: %d\n", err); 
						return -1;
					}
					clear_bit(CMC_BIT_ALS, &obj->enable);
				}
				
			}
			break;

		case SENSOR_GET_DATA:
			if((buff_out == NULL) || (size_out< sizeof(hwm_sensor_data)))
			{
				APS_ERR("get sensor data parameter error!\n");
				err = -EINVAL;
			}
			else
			{
				APS_ERR("get sensor als data !\n");
				sensor_data = (hwm_sensor_data *)buff_out;
				obj->als = ltr559_als_read(als_gainrange);
                #if defined(MTK_AAL_SUPPORT)
				sensor_data->values[0] = obj->als;
				#else
				sensor_data->values[0] = ltr559_get_als_value(obj, obj->als);
				#endif
				sensor_data->value_divide = 1;
				sensor_data->status = SENSOR_STATUS_ACCURACY_MEDIUM;
			}
			break;
		default:
			APS_ERR("light sensor operate function no this parameter %d!\n", command);
			err = -1;
			break;
	}
	
	return err;
}
#endif

static int als_open_report_data(int open)
{
		
	APS_LOG("E!!\n");
	return 0;
}


static int als_enable_nodata(int en)
{
	int res = 0;

	APS_LOG("E!!\n");
	APS_LOG("ltr559_obj als enable value = %d\n", en);

	mutex_lock(&ltr559_mutex);
	if (en)
		set_bit(CMC_BIT_ALS, &ltr559_obj->enable);
	else
		clear_bit(CMC_BIT_ALS, &ltr559_obj->enable);
	mutex_unlock(&ltr559_mutex);
	if (!ltr559_obj) {
		APS_ERR("ltr559_obj is null!!\n");
		return -1;
	}
	if (en)
		res = ltr559_als_enable(als_gainrange);
	else
		res = ltr559_als_disable();
	if (res) {
		APS_ERR("als_enable_nodata is failed!!\n");
		return -1;
	}
	return 0;
}

static int als_set_delay(u64 ns)
{
		
	APS_LOG("E!!\n");
	return 0;
}

static int als_get_data(int *value, int *status)
{
	int err = 0;
	struct ltr559_priv *obj = NULL;

	APS_LOG("E!!\n");
	if (!ltr559_obj) {
		APS_ERR("ltr559_obj is null!!\n");
		return -1;
	}
	obj = ltr559_obj;
	obj->als =  ltr559_als_read(als_gainrange);
//	if (err) {
	//	err = -1;
	//} else {
		*value = ltr559_get_als_value(obj, obj->als);
		*status = SENSOR_STATUS_ACCURACY_MEDIUM;
	//}


	return err;
}

static int ps_open_report_data(int open)
{
	
	APS_LOG("E!!\n");
	return 0;
}

static int ps_enable_nodata(int en)
{
	int res = 0;

	APS_LOG("E!!\n");
	APS_LOG("ltr559_obj als enable value = %d\n", en);
	mutex_lock(&ltr559_mutex);
	if (en)
		set_bit(CMC_BIT_PS, &ltr559_obj->enable);
	else
		clear_bit(CMC_BIT_PS, &ltr559_obj->enable);
	mutex_unlock(&ltr559_mutex);
	if (!ltr559_obj) {
		APS_ERR("ltr559_obj is null!!\n");
		return -1;
	}
	if(en)
		res = ltr559_ps_enable(ps_gainrange);
	else
		res = ltr559_ps_disable();
	if (res) {
		APS_ERR("als_enable_nodata is failed!!\n");
		return -1;
	}
	return 0;

}

static int ps_set_delay(u64 ns)
{
APS_LOG("E!!\n");
	return 0;
}

static int ps_get_data(int *value, int *status)
{
	APS_LOG("E!!\n");
	//int err = 0;

	if (!ltr559_obj) {
		APS_ERR("ltr559_obj is null!!\n");
		return -1;
	}
	ltr559_obj->ps  = ltr559_ps_read();
	//if (err) {
	//	err = -1;
	//} else {
	*value = ltr559_get_ps_value(ltr559_obj, ltr559_obj->ps);
	*status = SENSOR_STATUS_ACCURACY_MEDIUM;
	//}

	return 0;
}


/*----------------------------------------------------------------------------*/
//static int ltr559_i2c_detect(struct i2c_client *client, int kind, struct i2c_board_info *info) 
static int ltr559_i2c_detect(struct i2c_client *client, struct i2c_board_info *info) 
{    
APS_LOG("E!!\n");
	strcpy(info->type, LTR559_DEV_NAME);
	return 0;
}

/*----------------------------------------------------------------------------*/
static int ltr559_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct ltr559_priv *obj;
//	struct hwmsen_object obj_ps, obj_als;
	int err = 0;
	int retry = 3;
	struct als_control_path als_ctl = { 0 };
	struct als_data_path als_data = { 0 };
	struct ps_control_path ps_ctl = { 0 };
	struct ps_data_path ps_data = { 0 };


	APS_LOG("E!!\n");
	if(!(obj = kzalloc(sizeof(*obj), GFP_KERNEL)))
	{
		err = -ENOMEM;
		goto exit;
	}
	memset(obj, 0, sizeof(*obj));
	ltr559_obj = obj;

	obj->hw =hw;
	ltr559_get_addr(obj->hw, &obj->addr);

	INIT_WORK(&obj->eint_work, ltr559_eint_work);
	obj->client = client;
	i2c_set_clientdata(client, obj);	
	atomic_set(&obj->als_debounce, 300);
	atomic_set(&obj->als_deb_on, 0);
	atomic_set(&obj->als_deb_end, 0);
	atomic_set(&obj->ps_debounce, 300);
	atomic_set(&obj->ps_deb_on, 0);
	atomic_set(&obj->ps_deb_end, 0);
	atomic_set(&obj->ps_mask, 0);
	atomic_set(&obj->als_suspend, 0);
	atomic_set(&obj->ps_thd_val_high,  obj->hw->ps_threshold_high);
	atomic_set(&obj->ps_thd_val_low,  obj->hw->ps_threshold_low);
	//atomic_set(&obj->als_cmd_val, 0xDF);
	//atomic_set(&obj->ps_cmd_val,  0xC1);
	atomic_set(&obj->ps_thd_val,  obj->hw->ps_threshold);
	obj->enable = 0;
	obj->pending_intr = 0;
	obj->als_level_num = sizeof(obj->hw->als_level)/sizeof(obj->hw->als_level[0]);
	obj->als_value_num = sizeof(obj->hw->als_value)/sizeof(obj->hw->als_value[0]);   
	obj->als_modulus = (400*100)/(16*150);//(1/Gain)*(400/Tine), this value is fix after init ATIME and CONTROL register value
										//(400)/16*2.72 here is amplify *100
	BUG_ON(sizeof(obj->als_level) != sizeof(obj->hw->als_level));
	memcpy(obj->als_level, obj->hw->als_level, sizeof(obj->als_level));
	BUG_ON(sizeof(obj->als_value) != sizeof(obj->hw->als_value));
	memcpy(obj->als_value, obj->hw->als_value, sizeof(obj->als_value));
	atomic_set(&obj->i2c_retry, 3);
	set_bit(CMC_BIT_ALS, &obj->enable);
	set_bit(CMC_BIT_PS, &obj->enable);
	obj->irq_node = of_find_compatible_node(NULL, NULL, "mediatek, als-eint");

	als_ctl.is_use_common_factory = false;
	ps_ctl.is_use_common_factory = false;

	APS_LOG("ltr559_devinit() start...!\n");
	ltr559_i2c_client = client;

	do{
		err = ltr559_devinit();
		if(!err){
			break;
		}
	}while(retry--);
	
	if(err)
	{
		goto exit_init_failed;
	}
	APS_LOG("ltr559_devinit() ...OK!\n");

	//printk("@@@@@@ manufacturer value:%x\n",ltr559_i2c_read_reg(0x87));

	err = misc_register(&ltr559_device);
	if(err)
	{
		APS_ERR("ltr559_device register failed\n");
		goto exit_misc_device_register_failed;
	}

	
	/* Register sysfs attribute */
	err = ltr559_create_attr((struct driver_attribute *)&(ltr559_init_info.platform_diver_addr->driver));
	if(err)
	{
		APS_ERR(KERN_ERR "create attribute err = %d\n", err);
		goto exit_create_attr_failed;
	}

	als_ctl.open_report_data = als_open_report_data;
	als_ctl.enable_nodata = als_enable_nodata;
	als_ctl.set_delay = als_set_delay;
	als_ctl.is_report_input_direct = false;

	als_ctl.is_support_batch = false;

	err = als_register_control_path(&als_ctl);
	if (err) {
		APS_ERR("register fail = %d\n", err);
		goto exit_sensor_obj_attach_fail;
	}

	als_data.get_data = als_get_data;
	als_data.vender_div = 100;
	err = als_register_data_path(&als_data);
	if (err) {
		APS_ERR("tregister fail = %d\n", err);
		goto exit_sensor_obj_attach_fail;
	}

	ps_ctl.open_report_data = ps_open_report_data;
	ps_ctl.enable_nodata = ps_enable_nodata;
	ps_ctl.set_delay = ps_set_delay;
	ps_ctl.is_report_input_direct = false;
	ps_ctl.is_support_batch = false;

	err = ps_register_control_path(&ps_ctl);
	if (err) {
		APS_ERR("register fail = %d\n", err);
		goto exit_sensor_obj_attach_fail;
	}

	ps_data.get_data = ps_get_data;
	ps_data.vender_div = 100;
	err = ps_register_data_path(&ps_data);
	if (err) {
		APS_ERR("tregister fail = %d\n", err);
		goto exit_sensor_obj_attach_fail;
	}

#if 0
	err = batch_register_support_info(ID_LIGHT, als_ctl.is_support_batch, 100, 0);
	if (err) {
		APS_ERR("register light batch support err = %d\n", err);
		goto exit_sensor_obj_attach_fail;
	}

	err = batch_register_support_info(ID_PROXIMITY, ps_ctl.is_support_batch, 100, 0);
	if (err) {
		APS_ERR("register proximity batch support err = %d\n", err);
		goto exit_sensor_obj_attach_fail;
	}
#endif


	APS_LOG("%s: OK\n", __func__);
	return 0;

	exit_sensor_obj_attach_fail:
	exit_create_attr_failed:
	misc_deregister(&ltr559_device);
	exit_misc_device_register_failed:
	exit_init_failed:
	//i2c_detach_client(client);
	//exit_kfree:
	kfree(obj);
	exit:
	ltr559_i2c_client = NULL;           
//	MT6516_EINTIRQMask(CUST_EINT_ALS_NUM);  /*mask interrupt if fail*/
	APS_ERR("%s: err = %d\n", __func__, err);
	return err;
}

/*----------------------------------------------------------------------------*/

static int ltr559_i2c_remove(struct i2c_client *client)
{
	int err;	
	
	APS_LOG("E!!\n");
	
	err = ltr559_delete_attr(&ltr559_i2c_driver.driver);
	if(err)
	{
		APS_ERR("ltr559_delete_attr fail: %d\n", err);
	} 

	err = misc_deregister(&ltr559_device);
	if(err)
	{
		APS_ERR("misc_deregister fail: %d\n", err);    
	}
	
	ltr559_i2c_client = NULL;
	i2c_unregister_device(client);
	kfree(i2c_get_clientdata(client));

	return 0;
}
/*----------------------------------------------------------------------------*/
static int ltr559_probe() 
{
APS_LOG("E!!\n");

	ltr559_power(hw, 1);
	if(i2c_add_driver(&ltr559_i2c_driver))
	{
		APS_ERR("add driver error\n");
		return -1;
	} 
	return 0;
}
/*----------------------------------------------------------------------------*/
static int ltr559_remove()
{
APS_LOG("E!!\n");
	ltr559_power(hw, 0);    
	i2c_del_driver(&ltr559_i2c_driver);
	return 0;
}
/*----------------------------------------------------------------------------*/



/*----------------------------------------------------------------------------*/
static int __init ltr559_init(void)
{
	const char *name = "mediatek,ltr559";

	hw =   get_alsps_dts_func(name, hw);
	if (!hw)
		APS_ERR("get dts info fail\n");
	alsps_driver_add(&ltr559_init_info);
	return 0;

}
/*----------------------------------------------------------------------------*/
static void __exit ltr559_exit(void)
{
//	APS_FUN();
//	platform_driver_unregister(&ltr559_alsps_driver);
}
/*----------------------------------------------------------------------------*/
module_init(ltr559_init);
module_exit(ltr559_exit);
/*----------------------------------------------------------------------------*/
MODULE_AUTHOR("XX Xx");
MODULE_DESCRIPTION("LTR-559ALS Driver");
MODULE_LICENSE("GPL");

