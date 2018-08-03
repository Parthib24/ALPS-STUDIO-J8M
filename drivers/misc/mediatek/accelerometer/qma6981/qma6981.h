
#ifndef __QMA6981_H__
#define __QMA6981_H__

#include <linux/ioctl.h>  /* For IOCTL macros */

#define QMA6981_TILT			0x03	// Tilt status
#define QMA6981_SRST			0x04	// Sampling Rate Status
#define QMA6981_SPCNT			0x05	// Sleep Count
#define QMA6981_INTSU			0x17	// Interrupt Setup
#define QMA6981_MODE			0x11	// Mode
#define QMA6981_SR			    0x10	// Auto-Wake/Sleep and Debounce Filter
#define QMA6981_PDET			0x09	// Tap Detection
#define QMA6981_PD			    0x0a	// Tap Debounce Count
#define QMA6981_INT_MAP0		0x19	// INT MAP
#define QMA6981_RANGE          0x0f    //range set register
#define QMA6981_INT_STAT		0x09    //interrupt statues

#define QMA6981_FIFO_WTMK		0x31	// FIFO water mark level
#define QMA6981_FIFO_CONFIG		0x3e	// fifo configure
#define QMA6981_FIFO_DATA		0x3f	//fifo data out


#define GRAVITY_EARTH_1000           9807	// about (9.80665f)*1000





#define QMA6981_I2C_SLAVE_ADDR		0x12
#define QMA6981_REG_POWER_CTL		0x11
#define QMA6981_REG_BW_RATE			0x10

#define QMA6981_BW_7HZ							0x21
#define QMA6981_BW_31HZ             0x23   //19.8
#define QMA6981_BW_62HZ             0x24   //35.7
#define QMA6981_BW_125HZ            0x25   //66.96


#define QMA6981_ERR_I2C                     -1
#define QMA6981_SUCCESS						0


#define QMA6981_OSX		-0.55
#define QMA6981_OSY		4
#define QMA6981_OXZ		2.35
#define QMA6981_SFZY		-0.0102
/************************************************/
/* 	Accelerometer section defines	 	*/
/************************************************/

/* Accelerometer Sensor Full Scale */
#define QMA6981_RANGE_2G   (1<<0)
#define QMA6981_RANGE_4G   (1<<1)
#define QMA6981_RANGE_8G   (1<<2)
#define QMA6981_RANGE_16G  (1<<3)

#define QMA6981_AXES_NUM        3


/* Magnetic Sensor Operating Mode */
#define QMA6981_NORMAL_MODE	0x00
#define QMA6981_POS_BIAS	0x01
#define QMA6981_NEG_BIAS	0x02
#define QMA6981_CC_MODE		0x00
#define QMA6981_SC_MODE		0x01
#define QMA6981_IDLE_MODE	0x02
#define QMA6981_SLEEP_MODE	0x03

/* Magnetometer output data rate  */
#define QMA6981_ODR_75		0x00	/* 0.75Hz output data rate */
#define QMA6981_ODR1_5		0x04	/* 1.5Hz output data rate */
#define QMA6981_ODR3_0		0x08	/* 3Hz output data rate */
#define QMA6981_ODR7_5		0x0C	/* 7.5Hz output data rate */
#define QMA6981_ODR15		0x10	/* 15Hz output data rate */
#define QMA6981_ODR30		0x14	/* 30Hz output data rate */
#define QMA6981_ODR75		0x18	/* 75Hz output data rate */
#define QMA6981_ODR220		0x1C	/* 220Hz output data rate */


#ifdef __KERNEL__

struct QMA6981_platform_data {

	int irq;	
	
	u8 h_range;

	u8 axis_map_x;
	u8 axis_map_y;
	u8 axis_map_z;

	u8 negate_x;
	u8 negate_y;
	u8 negate_z;

	int (*init)(void);
	void (*exit)(void);
	int (*power_on)(void);
	int (*power_off)(void);

};
#endif /* __KERNEL__ */
extern struct acc_hw* qma6981_get_cust_acc_hw(void); 







#if 0//rl add
static char QMA6981_i2c_write(unsigned char reg_addr,
				    unsigned char *data,
				    unsigned char len);

static char QMA6981_i2c_read(unsigned char reg_addr,
				   unsigned char *data,
				   unsigned char len);
#endif
static int QMA6981_read_acc_xyz(int *data);
static int QMA6981_create_attr(struct device_driver *driver);
static int QMA6981_SetPowerCTRL(struct i2c_client *client, bool enable);
static int QMA6981_SetBWRate(struct i2c_client *client, u8 bwrate);
static int qma6981_initialize(struct i2c_client *client);
#endif  /* __QMA6981_H__ */
