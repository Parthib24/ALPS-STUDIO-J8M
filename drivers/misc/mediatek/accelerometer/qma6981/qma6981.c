/* QMA6981 motion sensor driver
 *
 *
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
 
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/kobject.h>
#include <linux/platform_device.h>
#include "qma6981.h"
#include <linux/kernel.h>
#include <asm/atomic.h>
#include <cust_acc.h>
#include <accel.h>

#define QMA6981_RETRY_COUNT 3
/*range*/
#define QMA6981_CHIP_ID		    0x00
#define QMA6981_XOUTL			0x01	// 4-bit output value X
#define QMA6981_XOUTH			0x02	// 6-bit output value X
#define QMA6981_YOUTL			0x03	
#define QMA6981_YOUTH			0x04	
#define QMA6981_ZOUTL			0x05	
#define QMA6981_ZOUTH			0x06	
#define QMA6981_DIE_ID			0x07
#define QMA6981_DIE_ID_V2		0x47

#define QMA6981_DEV_NAME        "qma6981"
//#define QMA6981_DEV_NAME        "mediatek,gsensor"  //rl mod
#define QMA6981_BUFSIZE		256
static struct i2c_client *this_client = NULL;


static struct mutex read_i2c_xyz;
static struct acc_init_info qma6981_init_info;

struct acc_hw accel_cust;
static struct acc_hw *hw = &accel_cust;

static bool debug_flag = false; // rl add

#define QMA6981_AXIS_X          0
#define QMA6981_AXIS_Y          1
#define QMA6981_AXIS_Z          2


#define MSE_TAG					"[QMA-Gsensor] "
#define MSE_FUN(f)		pr_info(MSE_TAG"%s\n", __FUNCTION__)
#define MSE_ERR(fmt, args...)	pr_err(MSE_TAG"%s %d : "fmt, __FUNCTION__, __LINE__, ##args)
#define MSE_LOG(fmt, args...)	if(debug_flag)\
								pr_info(MSE_TAG fmt, ##args)

#define DEBUG 1

#ifndef CONFIG_HAS_EARLYSUSPEND
static int QMA6981_suspend(struct i2c_client *client, pm_message_t msg);
static int QMA6981_resume(struct i2c_client *client);
#endif

//static struct i2c_board_info __initdata i2c_qma6981={ I2C_BOARD_INFO(QMA6981_DEV_NAME, (QMA6981_I2C_SLAVE_ADDR))};
/*
 * QMA6981 acc data
 * brief Structure containing acc field values for x,y and z-axis in
 * signed short
*/

struct QMA6981_t {
	short	x, /**< x-axis acc field data. Range -512 to 512. */
		y, /**< y-axis acc field data. Range -512 to 512. */
		z; /**< z-axis acc filed data. Range -512 to 512. */
};


struct QMA6981_data{
	struct i2c_client *client;
	struct acc_hw *hw;
	struct QMA6981_platform_data *pdata;
	short xy_sensitivity;
	short z_sensitivity;

	struct mutex lock;
	struct mutex motionLock;
	struct delayed_work work;
	struct delayed_work motionWork;
	struct input_dev *input;
	//struct miscdevice qma_misc;
	struct hwmsen_convert   cvt;

	int delay_ms;

	int enabled;
	atomic_t                trace;
	atomic_t                suspend;
    atomic_t layout;
	s16                     cali_sw[QMA6981_AXES_NUM+1];

	/*data*/
	s8                      offset[QMA6981_AXES_NUM+1];  /*+1: for 4-byte alignment*/
	s16                     data[QMA6981_AXES_NUM+1];
	u8                      bandwidth;
	//struct completion data_updated;
	//wait_queue_head_t state_wq;
	    /*early suspend*/
#if defined(CONFIG_HAS_EARLYSUSPEND)
    	struct early_suspend    early_drv;
#endif 
};



struct QMA6981_data *qma6981;
static bool sensor_power = true;
static bool enable_status = false;
int QMA6981_Chip_id = 0;


#if 0 //rl add
struct i2c_client * QMA_GET_CLIENT()
{
	return  this_client;
}
#endif
static int I2C_RxData(char *rxData, int length)
{	
	uint8_t loop_i;    
	int res = 0;
	
	if((rxData == NULL) || (length < 1))	
	{	
		MSE_ERR("qma6981 I2C_RxData error");
		return -EINVAL;	
	}	

	for(loop_i = 0; loop_i < QMA6981_RETRY_COUNT; loop_i++)	
	{		
		//this_client->addr = this_client->addr & I2C_MASK_FLAG | I2C_WR_FLAG;
		this_client->addr = this_client->addr & I2C_MASK_FLAG ;//rl mod 
		this_client->addr = this_client->addr | I2C_WR_FLAG;//rl mod
		res = i2c_master_send(this_client, (const char*)rxData, ((length<<0X08) | 0X01));		
		if(res > 0)
			break;

		MSE_ERR("QMA6981 i2c_read retry %d times\n", loop_i);
		mdelay(10);	
	}    	
	this_client->addr = this_client->addr & I2C_MASK_FLAG;

	if(loop_i >= QMA6981_RETRY_COUNT)	
	{		
		MSE_ERR("qma6981 %s retry over %d\n", __func__, QMA6981_RETRY_COUNT);
		return -EIO;	
	}
	return 0;
}

static int I2C_TxData(char *txData, int length)
{
	uint8_t loop_i;


	/* Caller should check parameter validity.*/
	if ((txData == NULL) || (length < 2))
	{
		return -EINVAL;
	}

	this_client->addr = this_client->addr & I2C_MASK_FLAG;
	for(loop_i = 0; loop_i < QMA6981_RETRY_COUNT; loop_i++)
	{
		if(i2c_master_send(this_client, (const char*)txData, length) > 0)
			break;

		MSE_ERR("I2C_TxData delay!\n");
		mdelay(10);
	}

	if(loop_i >= QMA6981_RETRY_COUNT)
	{
		MSE_ERR( "%s retry over %d\n", __func__, QMA6981_RETRY_COUNT);
		return -EIO;
	}

	return 0;
}

/* X,Y and Z-axis accelerometer data readout
 * param *acc pointer to \ref QMA6981_t structure for x,y,z data readout
 * note data will be read by multi-byte protocol into a 6 byte structure
*/

typedef struct {
    int i:10;
    int rsv:22;
}testtypedef;
static int QMA6981_read_raw_xyz(int *data){
	int res=0;	
	unsigned char databuf[6];	
	//int output[3]={ 0 };	
	
	//unsigned char databuf2[1];	
	//int output2[1]={ 0 };	
	testtypedef out;



	databuf[0] = QMA6981_XOUTL;		
	//if(res = I2C_RxData(databuf, 6)){
	if(res != I2C_RxData(databuf, 6)){// rl mod
		MSE_ERR("read xyz error!!!");
		return -EFAULT;	
	}	

 	data[0] = out.i = (int)((databuf[1]<<2) |( databuf[0]>>6));
	data[1] = out.i = (int)((databuf[3]<<2) |( databuf[2]>>6));
	data[2] = out.i = (int)((databuf[5]<<2) |( databuf[4]>>6));

	MSE_LOG("ttt raw LBS data  %d,%d,%d\n",data[0],data[1],data[2]);


	return 0;
}

static int QMA6981_read_acc_xyz(int *data){
	
	int raw[3]={0};
	int acc[3]={0};
	//int temp,ret = 0;
	int ret = 0;
	struct i2c_client *client = this_client;	
	struct QMA6981_data *obj = i2c_get_clientdata(client);
	
	QMA6981_read_raw_xyz(raw);
	//printk("tttt raw[0]=%d,raw[1]=%d,raw[2]=%d \n",raw[0],raw[1],raw[2]);

//Out put the mg
	acc[0] = ((raw[0]*1000)>>8) * GRAVITY_EARTH_1000 / 1000 ;
	acc[1] = ((raw[1]*1000)>>8) * GRAVITY_EARTH_1000 / 1000 ;
	acc[2] = ((raw[2]*1000)>>8) * GRAVITY_EARTH_1000 / 1000 ;


	data[obj->cvt.map[QMA6981_AXIS_X]] = obj->cvt.sign[QMA6981_AXIS_X]*acc[QMA6981_AXIS_X];
	data[obj->cvt.map[QMA6981_AXIS_Y]] = obj->cvt.sign[QMA6981_AXIS_Y]*acc[QMA6981_AXIS_Y];
	data[obj->cvt.map[QMA6981_AXIS_Z]] = obj->cvt.sign[QMA6981_AXIS_Z]*acc[QMA6981_AXIS_Z];
	
	data[QMA6981_AXIS_X] = data[QMA6981_AXIS_X]+obj->cali_sw[QMA6981_AXIS_X];
	data[QMA6981_AXIS_Y] = data[QMA6981_AXIS_Y]+obj->cali_sw[QMA6981_AXIS_Y];
	data[QMA6981_AXIS_Z] = data[QMA6981_AXIS_Z]+obj->cali_sw[QMA6981_AXIS_Z];
	MSE_LOG("tttt qma6981 AFTER x:%d,y:%d,z:%d\n",data[QMA6981_AXIS_X],data[QMA6981_AXIS_Y],data[QMA6981_AXIS_Z]);

	return ret;
}


/*----------------------------------------------------------------------------*/
static int QMA6981_ReadOffset(struct i2c_client *client, s8 ofs[QMA6981_AXES_NUM])
{    
	int err = 0;

	ofs[1]=ofs[2]=ofs[0]=0x00;

	MSE_LOG("qma6981 offesx=%x, y=%x, z=%x\n",ofs[0],ofs[1],ofs[2]);
	
	return err;    
}

/*----------------------------------------------------------------------------*/
static int QMA6981_ResetCalibration(struct i2c_client *client)
{
	struct QMA6981_data *obj = i2c_get_clientdata(client);
	int err = 0;
	MSE_LOG("qma6981 QMA6981_ResetCalibration\n");
	memset(obj->cali_sw, 0x00, sizeof(obj->cali_sw));
	memset(obj->offset, 0x00, sizeof(obj->offset));
	return err;    
}

/*----------------------------------------------------------------------------*/
#if 0//rl add
static int QMA6981_ReadCalibrationEx(struct i2c_client *client, int act[QMA6981_AXES_NUM], int raw[QMA6981_AXES_NUM])
{  
	/*raw: the raw calibration data; act: the actual calibration data*/
	struct QMA6981_data *obj = i2c_get_clientdata(client);
	int mul;

	mul = 0;//only SW Calibration, disable HW Calibration
	MSE_LOG("qma6981 QMA6981_ReadCalibrationEx\n");
	raw[QMA6981_AXIS_X] = obj->offset[QMA6981_AXIS_X]*mul + obj->cali_sw[QMA6981_AXIS_X];
	raw[QMA6981_AXIS_Y] = obj->offset[QMA6981_AXIS_Y]*mul + obj->cali_sw[QMA6981_AXIS_Y];
	raw[QMA6981_AXIS_Z] = obj->offset[QMA6981_AXIS_Z]*mul + obj->cali_sw[QMA6981_AXIS_Z];

	act[obj->cvt.map[QMA6981_AXIS_X]] = obj->cvt.sign[QMA6981_AXIS_X]*raw[QMA6981_AXIS_X];
	act[obj->cvt.map[QMA6981_AXIS_Y]] = obj->cvt.sign[QMA6981_AXIS_Y]*raw[QMA6981_AXIS_Y];
	act[obj->cvt.map[QMA6981_AXIS_Z]] = obj->cvt.sign[QMA6981_AXIS_Z]*raw[QMA6981_AXIS_Z];                        
	                       
	return 0;
}
#endif

/*----------------------------------------------------------------------------*/
static int QMA6981_WriteCalibration(struct i2c_client *client, int dat[QMA6981_AXES_NUM])
{
	struct QMA6981_data *obj = i2c_get_clientdata(client);
	//int err;
	int err=0;//rl add
	//int cali[QMA6981_AXES_NUM], raw[QMA6981_AXES_NUM];


	MSE_LOG("qma6981PDATE: (%+3d %+3d %+3d)\n",dat[QMA6981_AXIS_X], dat[QMA6981_AXIS_Y], dat[QMA6981_AXIS_Z]);


	obj->cali_sw[QMA6981_AXIS_X] = dat[QMA6981_AXIS_X];
	obj->cali_sw[QMA6981_AXIS_Y] = dat[QMA6981_AXIS_Y];
	obj->cali_sw[QMA6981_AXIS_Z] = dat[QMA6981_AXIS_Z];	


	return err;
}
/*----------------------------------------------------------------------------*/
static int QMA6981_ReadCalibration(struct i2c_client *client, int dat[QMA6981_AXES_NUM])
{
    struct QMA6981_data *obj = i2c_get_clientdata(client);
    //int mul;
  
    dat[QMA6981_AXIS_X] = obj->cali_sw[QMA6981_AXIS_X];
    dat[QMA6981_AXIS_Y] = obj->cali_sw[QMA6981_AXIS_Y];
    dat[QMA6981_AXIS_Z] = obj->cali_sw[QMA6981_AXIS_Z];                      
    return 0;
}


static ssize_t show_dumpallreg_value(struct device_driver *ddri, char *buf)
{
	int res;
	int i =0;
	char strbuf[1024];
	char tempstrbuf[24];
	unsigned char databuf[2];
	int length=0;



	MSE_FUN();

	/* Check status register for data availability */
	for(i =0;i<64;i++)
	{
		databuf[0] = i;
		res = I2C_RxData(databuf, 1);
		if(res < 0)
			MSE_LOG("QMA6981 dump registers 0x%02x failed !\n", i);

		length = scnprintf(tempstrbuf, sizeof(tempstrbuf), "reg[0x%2x] =  0x%2x \n",i, databuf[0]);
		snprintf(strbuf+length*i, sizeof(strbuf)-length*i, "%s \n",tempstrbuf);
	}

	return scnprintf(buf, sizeof(strbuf), "%s\n", strbuf);
}


/*----------------------------------------------------------------------------*/
static ssize_t show_chipinfo_value(struct device_driver *ddri, char *buf)
{
	char strbuf[QMA6981_BUFSIZE];
	//int res,output;
	int res=0,output;
	unsigned char databuf;


	databuf = QMA6981_CHIP_ID;
	//if(res = I2C_RxData(databuf, 1))
	if(res == I2C_RxData(&databuf, 1))
	{
		MSE_LOG("read chip id error!!!");
		return -EFAULT;
	}
	output = (int)databuf;

	sprintf(strbuf, "chipid:%d \n", output);

	return sprintf(buf, "%s\n", strbuf);
}

/*----------------------------------------------------------------------------*/
static ssize_t show_trace_value(struct device_driver *ddri, char *buf)
{
	ssize_t res;
	struct QMA6981_data *obj = i2c_get_clientdata(this_client);
	if(NULL == obj)
	{
		MSE_LOG("QMA6981_data is null!!\n");
		return 0;
	}

	res = snprintf(buf, PAGE_SIZE, "0x%04X\n", atomic_read(&obj->trace));
	return res;
}
/*----------------------------------------------------------------------------*/
#if 1
static ssize_t store_trace_value(struct device_driver *ddri, const char *buf, size_t count)
{
	struct QMA6981_data *obj = i2c_get_clientdata(this_client);
	int trace;
	if(NULL == obj)
	{
		MSE_LOG("QMA6981_data is null!!\n");
		return 0;
	}

	if(1 == sscanf(buf, "0x%x", &trace))
	{
		atomic_set(&obj->trace, trace);
	}
	else
	{
		//MSE_LOG("invalid content: '%s', length = %d\n", buf, count);
		MSE_LOG("invalid content: '%s', length = %ld\n", buf, count);
	}

	return count;
}
#endif
#if 0
static ssize_t store_trace_value(struct device_driver *ddri, const char *buf, size_t count)
{
	struct bma250_i2c_data *obj = obj_i2c_data;
	int trace;

	if (obj == NULL) {
		GSE_ERR("i2c_data obj is null!!\n");
		return 0;
	}

	if (1 == sscanf(buf, "0x%x", &trace))
		atomic_set(&obj->trace, trace);
	else
		GSE_ERR("invalid content: '%s', length = %d\n", buf, (int)count);

	return count;
}
#endif
/*----------------------------------------------------------------------------*/
static ssize_t show_cali_value(struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = this_client;
	struct QMA6981_data *obj;
	int err, len = 0, mul;
	int tmp[QMA6981_AXES_NUM];

	if(NULL == client)
	{
		MSE_LOG("i2c client is null!!\n");
		return 0;
	}

	obj = i2c_get_clientdata(client);



	if(0 != (err = QMA6981_ReadOffset(client, obj->offset)))
	{
		return -EINVAL;
	}
	else if(0 != (err = QMA6981_ReadCalibration(client, tmp)))
	{
		return -EINVAL;
	}
	else
	{    
		mul = 1000/128*9.8;
		len += snprintf(buf+len, PAGE_SIZE-len, "[HW ][%d] (%+3d, %+3d, %+3d) : (0x%02X, 0x%02X, 0x%02X)\n", mul,                        
			obj->offset[QMA6981_AXIS_X], obj->offset[QMA6981_AXIS_Y], obj->offset[QMA6981_AXIS_Z],
			obj->offset[QMA6981_AXIS_X], obj->offset[QMA6981_AXIS_Y], obj->offset[QMA6981_AXIS_Z]);
		len += snprintf(buf+len, PAGE_SIZE-len, "[SW ][%d] (%+3d, %+3d, %+3d)\n", 1, 
			obj->cali_sw[QMA6981_AXIS_X], obj->cali_sw[QMA6981_AXIS_Y], obj->cali_sw[QMA6981_AXIS_Z]);

		len += snprintf(buf+len, PAGE_SIZE-len, "[ALL]    (%+3d, %+3d, %+3d) : (%+3d, %+3d, %+3d)\n", 
			obj->offset[QMA6981_AXIS_X]*mul + obj->cali_sw[QMA6981_AXIS_X],
			obj->offset[QMA6981_AXIS_Y]*mul + obj->cali_sw[QMA6981_AXIS_Y],
			obj->offset[QMA6981_AXIS_Z]*mul + obj->cali_sw[QMA6981_AXIS_Z],
			tmp[QMA6981_AXIS_X], tmp[QMA6981_AXIS_Y], tmp[QMA6981_AXIS_Z]);
		
		return len;
    }
}
/*----------------------------------------------------------------------------*/
static ssize_t store_cali_value(struct device_driver *ddri, const char *buf, size_t count)
{
	struct i2c_client *client = this_client;  
	int err, x, y, z;
	int dat[QMA6981_AXES_NUM];

	if(!strncmp(buf, "rst", 3))
	{
		if(0 != (err = QMA6981_ResetCalibration(client)))
		{
			MSE_LOG("reset offset err = %d\n", err);
		}	
	}
	else if(3 == sscanf(buf, "0x%02X 0x%02X 0x%02X", &x, &y, &z))
	{
		dat[QMA6981_AXIS_X] = x;
		dat[QMA6981_AXIS_Y] = y;
		dat[QMA6981_AXIS_Z] = z;
		if(0 != (err = QMA6981_WriteCalibration(client, dat)))
		{
			MSE_LOG("write calibration err = %d\n", err);
		}		
	}
	else
	{
		MSE_LOG("invalid format\n");
	}
	
	return count;
}

/*----------------------------------------------------------------------------*/
static ssize_t show_sensordata_value(struct device_driver *ddri, char *buf)
{

	int sensordata[3];
	char strbuf[QMA6981_BUFSIZE];

	//QMA6981_read_acc_xyz(sensordata);
	QMA6981_read_acc_xyz(&sensordata[0]);
	sprintf(strbuf, "%d %d %d\n", sensordata[0],sensordata[1],sensordata[2]);

	return sprintf(buf, "%s\n", strbuf);
}



/*----------------------------------------------------------------------------*/
static ssize_t show_layout_value(struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = this_client;
    struct QMA6981_data *data = i2c_get_clientdata(client);

	return scnprintf(buf, PAGE_SIZE, "(%d, %d)\n[%+2d %+2d %+2d]\n[%+2d %+2d %+2d]\n",
		data->hw->direction,atomic_read(&data->layout),	data->cvt.sign[0], data->cvt.sign[1],
		data->cvt.sign[2],data->cvt.map[0], data->cvt.map[1], data->cvt.map[2]);
}
/*----------------------------------------------------------------------------*/
static ssize_t store_layout_value(struct device_driver *ddri, const char *buf, size_t count)
{
	struct i2c_client *client = this_client;
    struct QMA6981_data *data = i2c_get_clientdata(client);

	int layout = 0;

	if(1 == sscanf(buf, "%d", &layout))
	{
		atomic_set(&data->layout, layout);
		if(!hwmsen_get_convert(layout, &data->cvt))
		{
			MSE_ERR("HWMSEN_GET_CONVERT function error!\r\n");
		}
		else if(!hwmsen_get_convert(data->hw->direction, &data->cvt))
		{
			MSE_ERR("invalid layout: %d, restore to %d\n", layout, data->hw->direction);
		}
		else
		{
			MSE_ERR("invalid layout: (%d, %d)\n", layout, data->hw->direction);
			hwmsen_get_convert(0, &data->cvt);
		}
	}
	else
	{
		MSE_ERR("invalid format = '%s'\n", buf);
	}

	return count;
}
static DRIVER_ATTR(chipinfo,    S_IRUGO, show_chipinfo_value, NULL);
static DRIVER_ATTR(sensordata, S_IWUSR | S_IRUGO, show_sensordata_value,    NULL);
static DRIVER_ATTR(dumpallreg,  S_IRUGO , show_dumpallreg_value, NULL);
static DRIVER_ATTR(cali,       S_IWUSR | S_IRUGO, show_cali_value,          store_cali_value);
static DRIVER_ATTR(trace,       S_IRUGO | S_IWUSR, show_trace_value, store_trace_value);//
static DRIVER_ATTR(layout,      S_IRUGO | S_IWUSR, show_layout_value, store_layout_value);

static struct driver_attribute *attributes[] = {
	&driver_attr_chipinfo,
	&driver_attr_sensordata,   /*dump sensor data*/
	&driver_attr_dumpallreg,
	&driver_attr_cali,         /*show calibration data*/
	&driver_attr_trace,
	&driver_attr_layout,

};

static int qma6981_initialize(struct i2c_client *client)
{
	
	int ret = 0;
	unsigned char data[2] = {0};
	
	MSE_FUN();
	//unsigned char databuf2[1];
	//int output2[1]={ 0 };
//	int res;

//higher output data rate ,Bandwidth = 62.5Hz ODR = 4*BW = 250Hz
	data[0] = 0x10;
	data[1] = 0x2c;
	ret = I2C_TxData(data,2);
   	if(ret < 0)
	  goto exit_i2c_err;

//range  2g , 3.9mg/LSB
	data[0] = 0x0F;
	data[1] = 0x01;
	ret = I2C_TxData(data,2);
   	if(ret < 0)
   		goto exit_i2c_err;


//active mode,disable deep sleep,
	data[0] = 0x11;
	data[1] = 0x80;
	ret = I2C_TxData(data,2);
   	if(ret < 0)
	  goto exit_i2c_err;


   	return 0;
exit_i2c_err:
      	MSE_LOG("qma6981_initialize fail: %d\n",ret);
	return ret;


}



/*  ioctl command for QMA6981 device file */
static long qma6981_unlocked_ioctl(struct file *file, unsigned int cmd,unsigned long arg)
{
	int err = 0;
//	unsigned char data[6];
	void __user *val;
	char strbuf[QMA6981_BUFSIZE];
	int vec[3] = {0};
	//SENSOR_DATA sensor_data;//rl remove
	struct SENSOR_DATA sensor_data;
	int cali[3];

	struct i2c_client *client = (struct i2c_client*)file->private_data;

	struct QMA6981_data *obj = (struct QMA6981_data*)i2c_get_clientdata(client);	

	MSE_LOG("qma6981_unlocked_ioctl - cmd=%u, arg = %lu\n" , cmd, arg);
	//MSE_LOG("SET CAL = %d , CLR CAL = %d",GSENSOR_IOCTL_SET_CALI , GSENSOR_IOCTL_CLR_CALI);
	MSE_LOG("SET CAL = %ld , CLR CAL = %d",GSENSOR_IOCTL_SET_CALI , GSENSOR_IOCTL_CLR_CALI);
	/* check QMA6981_client */
	if (&qma6981->client == NULL) {
		#if DEBUG
		MSE_LOG( "I2C driver not install\n");
		#endif
		return -EFAULT;
	}

	switch (cmd) {
	
	case GSENSOR_IOCTL_INIT:
		qma6981_initialize(client);
		break;
	case GSENSOR_IOCTL_READ_SENSORDATA:

		val = (void __user *) arg;
		if(val == NULL)
		{
			err = -EINVAL;
			break;	  
		}
		
		MSE_LOG("qma6981_unlocked_ioctl - cmd=GSENSOR_IOCTL_READ_SENSORDATA\n");
		QMA6981_read_acc_xyz(vec);
		sprintf(strbuf, "%04x %04x %04x", vec[0], vec[1], vec[2]);
		if(copy_to_user(val, strbuf, strlen(strbuf)+1))
		{
			err = -EFAULT;
			break;	  
		}	
		break;

	case GSENSOR_IOCTL_READ_RAW_DATA:
		val = (void __user *) arg;
		if(val == NULL)
		{
			err = -EINVAL;
			break;	  
		}
		QMA6981_read_raw_xyz(vec);
		
		sprintf(strbuf, "%04x %04x %04x", vec[0], vec[1], vec[2]);
		if(copy_to_user(val, strbuf, strlen(strbuf)+1))
		{
			err = -EFAULT;
			break;	  
		}
		break;	  

	case GSENSOR_IOCTL_SET_CALI:
		MSE_LOG("qma6981 ioctl GSENSOR_IOCTL_SET_CALI\n");
		val = (void __user*)arg;
		if(val == NULL)
		{
			err = -EINVAL;
			break;	  
		}
		if(copy_from_user(&sensor_data, val, sizeof(sensor_data)))
		{
			err = -EFAULT;
			break;	  
		}
		if(atomic_read(&obj->suspend))
		{
			MSE_LOG("Perform calibration in suspend state!!\n");
			err = -EINVAL;
		}
		else
		{
			cali[QMA6981_AXIS_X] = sensor_data.x;
			cali[QMA6981_AXIS_Y] = sensor_data.y;
			cali[QMA6981_AXIS_Z] = sensor_data.z;			  
			err = QMA6981_WriteCalibration(client, cali);			 
		}
		break;

	case GSENSOR_IOCTL_CLR_CALI:
		MSE_LOG("qma6981 ioctl GSENSOR_IOCTL_CLR_CALI\n");
		err = QMA6981_ResetCalibration(client);
		break;

	case GSENSOR_IOCTL_GET_CALI:
		MSE_LOG("qma6981 ioctl GSENSOR_IOCTL_GET_CALI\n");
		val = (void __user*)arg;
		if(val == NULL)
		{
			err = -EINVAL;
			break;	  
		}
		if(0 != (err = QMA6981_ReadCalibration(client, cali)))
		{
			break;
		}
		MSE_LOG("qma6981 get cali.x,y,z[%d,%d,%d]\n",cali[QMA6981_AXIS_X],cali[QMA6981_AXIS_Y],cali[QMA6981_AXIS_Z]);
		sensor_data.x = cali[QMA6981_AXIS_X];
		sensor_data.y = cali[QMA6981_AXIS_Y];
		sensor_data.z = cali[QMA6981_AXIS_Z];
		MSE_LOG("qma6981 get sensor_data.x,y,z[%d,%d,%d]\n",sensor_data.x,sensor_data.y,sensor_data.z);
		if(copy_to_user(val, &sensor_data, sizeof(sensor_data)))
		{
			err = -EFAULT;
			break;
		}		
		break;
	default:
		break;
	}
	return err;
}


/*----------------------------------------------------------------------------*/
static int qma6981_open(struct inode *inode, struct file *file)
{
	file->private_data = this_client;

	if(file->private_data == NULL)
	{
		MSE_LOG("null pointer!!\n");
		return -EINVAL;
	}
	return nonseekable_open(inode, file);
}
/*----------------------------------------------------------------------------*/
static int qma6981_release(struct inode *inode, struct file *file)
{
	file->private_data = NULL;
	return 0;
}

static const struct file_operations qma6981_fops = {
	.owner = THIS_MODULE,
	.open = qma6981_open,
	.release = qma6981_release,
	.unlocked_ioctl = qma6981_unlocked_ioctl,
	#ifdef CONFIG_COMPAT
	.compat_ioctl	= qma6981_unlocked_ioctl,// rl add for cali 
	#endif
};

//static const struct miscdevice qma6981_miscdevice = {
	static  struct miscdevice qma6981_miscdevice = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "gsensor",
	.fops = &qma6981_fops,
};


static int QMA6981_SetPowerCTRL(struct i2c_client *client, bool enable)
{
    int res = 0;
//    struct QMA6981_data *obj = i2c_get_clientdata(client);
	
    u8 databuf[2];

    if (enable == sensor_power)
    {
//        MSE_LOG("Sensor power status is newest!\n");
        return QMA6981_SUCCESS;
    }



   // if (enable == FALSE)
   if (enable == 0)
        databuf[1]=0x00;
    else
        databuf[1]=0x80;

    databuf[0] = QMA6981_REG_POWER_CTL;
    res = I2C_TxData(databuf,2);


    if (res < 0)
        return QMA6981_ERR_I2C;

    sensor_power = enable;
    return QMA6981_SUCCESS;    
}

static int qma6981_open_report_data(int open)
{
	return 0;
}

static int qma6981_enable_nodata(int en)
{
    int err = 0;
	MSE_LOG("qma6981_enable_nodata!\n");
	if(((en == 0) && (sensor_power == false)) ||((en == 1) && (sensor_power == true)))
	{
		enable_status = sensor_power;
		MSE_LOG("Gsensor device have updated!\n");
	}
	else
	{
		enable_status = !sensor_power;
		if (atomic_read(&qma6981->suspend) == 0)
		{

			err = QMA6981_SetPowerCTRL(qma6981->client, enable_status);
			MSE_LOG("Gsensor not in suspend gsensor_SetPowerMode!, enable_status = %d\n",enable_status);
		}
		else
		{
			MSE_LOG("Gsensor in suspend and can not enable or disable!enable_status = %d\n",enable_status);
		}
	}

    if(err != 0)
	{
		MSE_LOG("gsensor_enable_nodata fail!\n");
		return -1;
	}

//    MSE_LOG("gsensor_enable_nodata OK!!!\n");
	return 0;
}

/*----------------------------------------------------------------------------*/
static int QMA6981_SetBWRate(struct i2c_client *client, u8 bwrate)
{
    struct QMA6981_data *obj = i2c_get_clientdata(client);
    u8 databuf[10];    
    int res = 0;



    if( (obj->bandwidth != bwrate) || (atomic_read(&obj->suspend)) )
    {    
        memset(databuf, 0, sizeof(u8)*10);    
    

    
    
        /* write */
        databuf[1] = databuf[0] | bwrate;
        databuf[0] = QMA6981_REG_BW_RATE;    
    
        res = I2C_TxData(databuf,2);
        if (res < 0)
            return QMA6981_ERR_I2C;

        obj->bandwidth = bwrate;
    }

    return 0;    
}
static int qma6981_set_delay(u64 ns)
{
    int err = 0;
    int value;
	int sample_delay;

    value = (int)ns/1000/1000;
	if(value <= 10)
	{
		sample_delay = QMA6981_BW_125HZ;
	}
	else if(value <= 20)
	{
		sample_delay = QMA6981_BW_62HZ;
	}
	else if(value <= 66)
	{
		sample_delay = QMA6981_BW_31HZ;
	}else{
		sample_delay = QMA6981_BW_7HZ;
	}

	err = QMA6981_SetBWRate(qma6981->client, sample_delay);

	if(err != 0 ) //0x2C->BW=100Hz
	{
		MSE_LOG("Set delay parameter error!\n");
        return err;
	}


	return 0;
}

static int qma6981_get_data(int* x ,int* y,int* z, int* status)
{
	int vec[3]={0};
	int ret=0;

	ret = QMA6981_read_acc_xyz(vec);

	*x = vec[0];
	*y = vec[1];
	*z = vec[2];
	//printk("tttttt x=%d,y=%d,z=%d\n",vec[0],vec[1],vec[2]);
	*status = SENSOR_STATUS_ACCURACY_HIGH;
	return ret;
}


static int qma6981_delete_attr(struct device_driver *driver)
{
	int idx ,err = 0;
	int num = (int)(sizeof(attributes)/sizeof(attributes[0]));

	if(driver == NULL)
	{
		return -EINVAL;
	}


	for(idx = 0; idx < num; idx++)
	{
		driver_remove_file(driver, attributes[idx]);
	}


	return err;
}

static int QMA6981_create_attr(struct device_driver *driver)
{
	int idx, err = 0;
	int num = (int)(sizeof(attributes)/sizeof(attributes[0]));
	if (driver == NULL)
	{
		return -EINVAL;
	}

	for(idx = 0; idx < num; idx++)
	{
		if(0 != (err = driver_create_file(driver, attributes[idx])))
		{
			MSE_LOG("attributes (%s) = %d\n", attributes[idx]->attr.name, err);
			break;
		}
	}
	return err;
}
static int QMA6981_probe(struct i2c_client *client, const struct i2c_device_id *devid)
{

	int err = 0;
//	unsigned char val;
	struct acc_control_path ctl={0};
	struct acc_data_path data={0};
	

//	struct i2c_client *new_client;
	
	MSE_LOG("QMA6981_probe start !\n");

	qma6981 = kzalloc(sizeof(struct QMA6981_data), GFP_KERNEL);
	if (qma6981 == NULL)
	{
		err = -ENOMEM;
		goto exit;
	}

	memset(qma6981, 0, sizeof(struct QMA6981_data));

	qma6981->hw = hw;

	if(0 != (err = hwmsen_get_convert(qma6981->hw->direction, &qma6981->cvt)))
	{
		MSE_ERR("invalid direction: %d\n", qma6981->hw->direction);
		goto exit;
	}
	client->addr = 0x12;//rl add
	qma6981->client = client;
	this_client = qma6981->client;	 
	i2c_set_clientdata(this_client, qma6981);

	qma6981->delay_ms = 100;
 	client->timing = 100;
	
	if(0 > qma6981_initialize(client))
	{
	printk(" QMA6981 initial failed\n");
		goto exit_kfree;
	}
	printk(" QMA6981 initial sucess\n");
	mutex_init(&qma6981->lock);
	mutex_init(&read_i2c_xyz);

	atomic_set(&qma6981->trace, 0);
	atomic_set(&qma6981->suspend, 0);
	err = misc_register(&qma6981_miscdevice);
	if(err){
		dev_err(&client->dev, "misc register failed\n");
		goto exit_kfree;
	}

	if(0 != (err = QMA6981_create_attr(&qma6981_init_info.platform_diver_addr->driver)))
	{
		dev_err(&client->dev,"create attribute err = %d\n", err);
		goto exit_create_attr_failed;
	}


	ctl.open_report_data= qma6981_open_report_data;
	ctl.enable_nodata =qma6981_enable_nodata;
	ctl.set_delay  = qma6981_set_delay;
	ctl.is_report_input_direct = false;
    	ctl.is_support_batch = false;


	err = acc_register_control_path(&ctl);
	if(err)
	{
	 	MSE_LOG("register acc control path err\n");
		goto exit_create_attr_failed;
	}


	data.get_data = qma6981_get_data;
	data.vender_div = 1000;
	err = acc_register_data_path(&data);
	if(err)
	{
	 	MSE_LOG("register acc data path err\n");
		goto exit_create_attr_failed;
	}
	#ifdef CONFIG_HAS_EARLYSUSPEND
	qma6981->early_drv.level    = EARLY_SUSPEND_LEVEL_DISABLE_FB - 1,	
	qma6981->early_drv.suspend  = QMA6981_early_suspend,	
	qma6981->early_drv.resume   = QMA6981_late_resume,    	
	register_early_suspend(&qma6981->early_drv);
	#endif



	//err = batch_register_support_info(ID_ACCELEROMETER,ctl.is_support_batch, 1000);
	err = batch_register_support_info(ID_ACCELEROMETER,ctl.is_support_batch, 1000,0);//rl add 

	MSE_LOG("QMA6981 device created successfully\n");
	
	return 0;


exit_create_attr_failed:
	misc_deregister(&qma6981_miscdevice);

//exit_kfree_input:
//	input_unregister_device(qma6981->input);//rl remove
exit_kfree:
	kfree(qma6981);
exit:
	return err;
}


static int QMA6981_i2c_remove(struct i2c_client *client)
{
	int err = 0;

	struct QMA6981_data *dev = i2c_get_clientdata(client);

	if(0 != (err = qma6981_delete_attr(&(qma6981_init_info.platform_diver_addr->driver))))
	{
		MSE_LOG("qma6981_delete_attr fail: %d\n", err);
	}
	
	misc_deregister(&qma6981_miscdevice);

	kfree(dev);
	i2c_unregister_device(client);
	this_client = NULL;
	return 0;
}
/*----------------------------------------------------------------------------*/
static void qma6981_power(struct acc_hw *hw, unsigned int on)
{
	static unsigned int power_on = 0;
#if 0
	if(hw->power_id != MT65XX_POWER_NONE)
	{
		MSE_LOG("power %s\n", on ? "on" : "off");
		if(power_on == on)
		{
			MSE_LOG("ignore power control: %d\n", on);
		}
		else if(on)
		{
			if(!hwPowerOn(hw->power_id, hw->power_vol, "qma6981"))
			{
				MSE_LOG( "power on fails!!\n");
			}
		}
		else
		{
			if(!hwPowerDown(hw->power_id, "qma6981"))
			{
				MSE_LOG( "power off fail!!\n");
			}
		}
	}
#endif
	power_on = on;
}



#ifndef CONFIG_HAS_EARLYSUSPEND
/*----------------------------------------------------------------------------*/
static int QMA6981_suspend(struct i2c_client *client, pm_message_t msg)
{
	struct QMA6981_data *obj = i2c_get_clientdata(client);
	int err = 0;
	MSE_FUN();
	if(msg.event == PM_EVENT_SUSPEND)
	{
		if(NULL == obj){
		   MSE_ERR("null pointer!\n");
		   return -EINVAL;	
		}
		atomic_set(&obj->suspend, 1);
		
		err = QMA6981_SetPowerCTRL(obj->client, false);
		if(err)
		{
			MSE_ERR("write power ctl fail!\n");
			return err;
		}
		sensor_power = false;
		qma6981_power(obj->hw, 0);
	}
	return 0;//rl add
}

static int QMA6981_resume(struct i2c_client *client)
{
	struct QMA6981_data *obj = i2c_get_clientdata(client);
	int err;
	MSE_FUN();
	if(NULL == obj){
	   MSE_ERR("null pointer!\n");
	   return -EINVAL;	
	}
	qma6981_power(obj->hw, 1);

	err = qma6981_initialize(obj->client);
	if(err)
	{
		MSE_ERR("initialize client fail!\n");
		return err;
	}
	atomic_set(&obj->suspend, 0);

	return 0;
}
/*----------------------------------------------------------------------------*/
#else /*CONFIG_HAS_EARLY_SUSPEND is defined*/
/*----------------------------------------------------------------------------*/
static void QMA6981_early_suspend(struct early_suspend *h)
{
	struct QMA6981_data *obj = container_of(h, struct QMA6981_data, early_drv);
	MSE_FUN();
	if(NULL == obj)
	{
		MSE_LOG("null pointer!!\n");
		return;
	}
	atomic_set(&obj->suspend, 1);
	err = QMA6981_SetPowerCTRL(obj->client, false);
	if(err)
	{
		MSE_ERR("write power ctl fail!\n");
		return err;
	}

	qma6981_power(obj->hw, 0);
}
/*----------------------------------------------------------------------------*/
static void QMA6981_late_resume(struct early_suspend *h)
{
	struct QMA6981_data *obj = container_of(h, struct QMA6981_data, early_drv);
	MSE_FUN();

	if(NULL == obj)
	{
		MSE_LOG("null pointer!!\n");
		return;
	}
	qma6981_power(obj->hw, 1);

	err = qma6981_initialize(obj->client);
	if(err)
	{
		MSE_ERR("initialize client fail!\n");
		return err;
	}
	atomic_set(&obj->suspend, 0);	
}
/*----------------------------------------------------------------------------*/
#endif /*CONFIG_HAS_EARLYSUSPEND*/



static const struct i2c_device_id QMA6981_id[] = {{QMA6981_DEV_NAME,0},{}};

#ifdef CONFIG_OF
static const struct of_device_id accel_of_match[] = {
	{.compatible = "mediatek,gsensor"},
	{},
};
#endif
static struct i2c_driver QMA6981_driver = {
	.probe = QMA6981_probe,
	.remove = QMA6981_i2c_remove,
	.id_table = QMA6981_id,
#if !defined(CONFIG_HAS_EARLYSUSPEND)
	.suspend = QMA6981_suspend,
	.resume = QMA6981_resume,
#endif
	.driver = {
		.owner = THIS_MODULE,
		.name = "qma6981",
		#ifdef CONFIG_OF
		.of_match_table = accel_of_match,
		#endif
	},
	
};

static int  qma6981_local_init(void)
{
	const char *name = "mediatek,qma6981";	
	hw = get_accel_dts_func(name, hw);

	qma6981_power(hw, 1);

	if(i2c_add_driver(&QMA6981_driver))
	{
		MSE_LOG("add i2c driver error\n");
		return -1;
	}
	printk(" qma6981_local_init end");
	return 0;
}

static int qma6981_local_remove(void)
{
	const char *name = "mediatek,qma6981";	
	hw = get_accel_dts_func(name, hw);
	qma6981_power(hw, 0);    

	i2c_del_driver(&QMA6981_driver);
	return 0;
}




static struct acc_init_info qma6981_init_info = {
		.name = "qma6981",
		.init = qma6981_local_init,
		.uninit = qma6981_local_remove,
	
};

static int __init QMA6981_init(void)
{
	int ret = 0;
	const char *name = "mediatek,qma6981";	
	hw = get_accel_dts_func(name, hw);
	if (!hw) {
		printk("get cust_accel dts info fail\n");
	}
	MSE_LOG("%s: i2c_number=%d\n", __func__,hw->i2c_num);
//	i2c_register_board_info(hw->i2c_num, &i2c_qma6981, 1);

	ret = acc_driver_add(&qma6981_init_info);

	return ret;
}

static void __exit QMA6981_exit(void)
{
	i2c_del_driver(&QMA6981_driver);
	return;
}

module_init(QMA6981_init);
module_exit(QMA6981_exit);

MODULE_DESCRIPTION("QST QMA6981 Acc driver");
MODULE_AUTHOR("QST");
MODULE_LICENSE("GPL");

