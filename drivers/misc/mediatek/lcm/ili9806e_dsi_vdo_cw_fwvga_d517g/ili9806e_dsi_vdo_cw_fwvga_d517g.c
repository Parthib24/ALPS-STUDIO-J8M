#ifndef BUILD_LK
#include <linux/string.h>
#endif
#include "lcm_drv.h"
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/mm_types.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/gpio.h>
#include <linux/device.h>

#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/regulator/consumer.h>
#include <linux/clk.h>
#include <linux/pinctrl/consumer.h>
#include <linux/of_gpio.h>
#endif
static unsigned int lcm_rst_pin;
static unsigned int lcm_gpio_enn;
static unsigned int lcm_gpio_enp;

#define AUXADC_LCM_VOLTAGE_CHANNEL     12
#define MIN_VOLTAGE (200)	//++++rgk bug-id:no modify by yangjuwei 20140401
#define MAX_VOLTAGE (400)	//++++rgk bug-id:no modify by yangjuwei 20140401

#define FRAME_WIDTH                                          (480)
#define FRAME_HEIGHT                                         (854)

#define REGFLAG_DELAY                                        0xFFE
#define REGFLAG_END_OF_TABLE                      			 0x1FF   // END OF REGISTERS MARKER

#define LCM_ID_ILI9806						0x9806
#define LCM_DSI_CMD_MODE                                    0

//static unsigned int lcm_esd_test = 0;      ///only for ESD test

static LCM_UTIL_FUNCS lcm_util = {0};

//#define SET_RESET_PIN(v)    (lcm_util.set_reset_pin((v)))
#define SET_RESET_PIN(v)    lcm_rst_pin_set(lcm_rst_pin,v)//(lcm_util.set_reset_pin((v)))

#define UDELAY(n) 				(lcm_util.udelay(n))
#define MDELAY(n) 				(lcm_util.mdelay(n))


// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------
#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)		lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)			lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)							lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)				lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg                                            lcm_util.dsi_read_reg()
#define read_reg_v2(cmd, buffer, buffer_size)				lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)
extern int IMM_GetOneChannelValue(int dwChannel, int data[4], int* rawdata);

static struct LCM_setting_table {
    unsigned cmd;
    unsigned char count;
    unsigned char para_list[64];
};

static void lcm_set_util_funcs(const LCM_UTIL_FUNCS *util)
{
	memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
}


static void lcm_get_params(LCM_PARAMS *params)
{

	memset(params, 0, sizeof(LCM_PARAMS));
	
	params->type   = LCM_TYPE_DSI;

	params->width  = FRAME_WIDTH;
	params->height = FRAME_HEIGHT;


#if (LCM_DSI_CMD_MODE)
	params->dsi.mode   = CMD_MODE;
#else
	params->dsi.mode   = SYNC_PULSE_VDO_MODE; //SYNC_PULSE_VDO_MODE;//BURST_VDO_MODE; 
#endif
	
	// DSI
	/* Command mode setting */
	//1 Three lane or Four lane
	params->dsi.LANE_NUM				= LCM_TWO_LANE;
	//The following defined the fomat for data coming from LCD engine.
	params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

	// Video mode setting		
	params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
		
	params->dsi.vertical_sync_active				= 6;// 3    2
	params->dsi.vertical_backporch					= 14;// 20   1
	params->dsi.vertical_frontporch					= 20; // 1  12
	params->dsi.vertical_active_line				= FRAME_HEIGHT; 

	params->dsi.horizontal_sync_active				= 10;// 50  2
	params->dsi.horizontal_backporch				= 90;//90
	params->dsi.horizontal_frontporch				= 90;//90
	params->dsi.horizontal_active_pixel				= FRAME_WIDTH;

	params->dsi.HS_TRAIL                            = 15;
		/*ui = 1000/(dis_params->PLL_CLOCK*2) + 0x01; 
		cycle_time = 8000/(dis_params->PLL_CLOCK*2) + 0x01;
		HS_TRAIL = (0x04*ui + 0x50)/cycle_time;*/
	//params->dsi.LPX=8; 

	params->dsi.PLL_CLOCK							=215;//208//270	
//	params->dsi.ssc_disable							= 1;
//	params->dsi.ssc_range							= 4;


//huihuang.shi
	params->dsi.esd_check_enable 					= 1;
/////////
	params->dsi.customization_esd_check_enable 		= 1;
	params->dsi.lcm_esd_check_table[0].cmd			= 0x0a;
	params->dsi.lcm_esd_check_table[0].count		= 1;
	params->dsi.lcm_esd_check_table[0].para_list[0] = 0x9c;

}

//static unsigned int vcom = 0x30;
static void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
	unsigned int i;
	
	for(i = 0; i < count; i++) 
	{   
		unsigned cmd;
		cmd = table[i].cmd;

		switch (cmd)
		{
			case REGFLAG_DELAY :
				MDELAY(table[i].count);
				break;
			case REGFLAG_END_OF_TABLE :
				break;
			/*case 0xd9 :
				table[i].para_list[0] = vcom;
				vcom +=2;
				dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
				break;*/
			default:
				dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
		}
	}
}

static struct LCM_setting_table lcm_deep_sleep_mode_in_setting[] = {
	{0x28, 0, {}},
	{REGFLAG_DELAY, 20, {}},
	{0x10, 0, {}},
	{REGFLAG_DELAY, 120, {}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};

static struct LCM_setting_table lcm_initialization_setting[] = {
//****************************************************************************//
//****************************** Page 1 Command ******************************//
//****************************************************************************//
{0xFF,5,{0xFF,0x98,0x06,0x04,0x01}},     // Change to Page 1

{0x08,1,{0x10}},                 // output SDA
        
{0x21,1,{0x01}},                 // DE = 1 Active  Display Function Control
        
{0x30,1,{0x01}},                 // 480 X 854
        
{0x31,1,{0x00}},                 // CLOUMN inversion
        
{0x50,1,{0x88}},                 // VGMP
        
{0x51,1,{0x88}},                 // VGMN
        
        
{0x60,1,{0x15}},                 // SDTI=6.5us
        
{0x61,1,{0x00}},                // CRTI
        
{0x62,1,{0x07}},                 // EQTI
        
{0x63,1,{0x00}},                // PCTI
        
        
{0x40,1,{0x15}},                 // DDVDH/DDVDL  18
        
{0x41,1,{0x44}},                 // DDVDH/DDVDL  Clamp  55
        
{0x42,1,{0x03}},                // VGH/VGL 
        
{0x43,1,{0x0A}},                 // VGH Clamp 16V       
        
{0x44,1,{0x06}},                 // VGL Clamp -10V            
        
{0x52,1,{0x00}},                   //Flicker
        
{0x53,1,{0x4A}},                   //Flicker  17
        
{0x57,1,{0x50}},                  
        
//+++++++++++++++++++ Gamma Setting ++++++++++++++++++//
{0xFF,5,{0xFF,0x98,0x06,0x04,0x01}},     // Change to Page 1
        
{0xA0,1,{0x00}},  // Gamma 255 
        
{0xA1,1,{0x09}}, // Gamma 251
        
{0xA2,1,{0x1C}},  // Gamma 247
        
{0xA3,1,{0x14}},  // Gamma 239
        
{0xA4,1,{0x0E}},  // Gamma 231
        
{0xA5,1,{0x17}}, // Gamma 203
        
{0xA6,1,{0x0B}},  // Gamma 175
        
{0xA7,1,{0x09}},  // Gamma 147
        
{0xA8,1,{0x02}},  // Gamma 108
        
{0xA9,1,{0x09}},  // Gamma 80
        
{0xAA,1,{0x0E}},  // Gamma 52
        
{0xAB,1,{0x07}},  // Gamma 24
        
{0xAC,1,{0x0D}},  // Gamma 16
        
{0xAD,1,{0x22}},  // Gamma 8
        
{0xAE,1,{0x1C}},  // Gamma 4
        
{0xAF,1,{0x00}},  // Gamma 0
        
///====================Nagitive
{0xC0,1,{0x00}},  // Gamma 0 
        
{0xC1,1,{0x0D}},  // Gamma 4
        
{0xC2,1,{0x1C}},  // Gamma 8
        
{0xC3,1,{0x12}},  // Gamma 16
        
{0xC4,1,{0x0B}},  // Gamma 24
        
{0xC5,1,{0x1C}},  // Gamma 52
        
{0xC6,1,{0x08}},  // Gamma 80
        
{0xC7,1,{0x08}},  // Gamma 108
        
{0xC8,1,{0x04}}, // Gamma 147
        
{0xC9,1,{0x07}},  // Gamma 175
        
{0xCA,1,{0x00}},  // Gamma 203
        
{0xCB,1,{0x05}},  // Gamma 231
        
{0xCC,1,{0x0A}},  // Gamma 239
        
{0xCD,1,{0x24}},  // Gamma 247
        
{0xCE,1,{0x20}},  // Gamma 251
        
{0xCF,1,{0x00}},  // Gamma 255
        
        
//**************************************************************************************//
//**************************************** Page 6 Command ******************************//
//**************************************************************************************//
{0xFF,5,{0xFF,0x98,0x06,0x04,0x06}},     // Change to Page 6
{0x00,1,{0x21}},
{0x01,1,{0x06}},
{0x02,1,{0xA0}},    
{0x03,1,{0x02}},
{0x04,1,{0x01}},
{0x05,1,{0x01}},
{0x06,1,{0x80}},    
{0x07,1,{0x03}},  
{0x08,1,{0x06}},  
{0x09,1,{0x80}},    
{0x0A,1,{0x00}},    
{0x0B,1,{0x00}},    
{0x0C,1,{0x20}},
{0x0D,1,{0x20}},
{0x0E,1,{0x09}},
{0x0F,1,{0x00}},
{0x10,1,{0xFF}},
{0x11,1,{0xE0}},
{0x12,1,{0x00}},
{0x13,1,{0x00}},
{0x14,1,{0x00}},
{0x15,1,{0xC0}},
{0x16,1,{0x08}},
{0x17,1,{0x00}},
{0x18,1,{0x00}},
{0x19,1,{0x00}},
{0x1A,1,{0x00}},
{0x1B,1,{0x00}},
        
{0x1C,1,{0x00}},
{0x1D,1,{0x00}},
{0x20,1,{0x01}},
{0x21,1,{0x23}},
{0x22,1,{0x45}},
{0x23,1,{0x67}},
{0x24,1,{0x01}},
{0x25,1,{0x23}},
{0x26,1,{0x45}},
{0x27,1,{0x67}},
        
{0x30,1,{0x12}},
{0x31,1,{0x22}},
{0x32,1,{0x22}},
{0x33,1,{0x22}},
{0x34,1,{0x87}},
{0x35,1,{0x96}},
{0x36,1,{0xAA}},
{0x37,1,{0xDB}},
{0x38,1,{0xCC}},
{0x39,1,{0xBD}},
{0x3A,1,{0x78}},
{0x3B,1,{0x69}},
{0x3C,1,{0x22}},
{0x3D,1,{0x22}},
{0x3E,1,{0x22}},
{0x3F,1,{0x22}},
{0x40,1,{0x22}},
{0x52,1,{0x10}},
{0x53,1,{0x10}},
{0x54,1,{0x13}},
        
        
//******************************************************************************//
{0xFF,5,{0xFF,0x98,0x06,0x04,0x07}},     // Change to Page 7
{0x02,1,{0x77}},                
{0x06,1,{0x13}}, 
{0xE1,1,{0x79}},                                
{0x17,1,{0x22}},                
{0xB3,1,{0x10}},
{0x26,1,{0xB2}},
//********************************************************************************//
{0xFF,5,{0xFF,0x98,0x06,0x04,0x00}},     // Change to Page 0
        
{0x11,1,{0x00}},                 // Sleep-Out
{REGFLAG_DELAY, 120, {}},	
{0x29,1,{0x00}},                 // Display On
{REGFLAG_DELAY, 10, {}},                                       
};
static void lcm_get_pin(void)
{
 static struct device_node *node;
 node = of_find_compatible_node(NULL, NULL, "mediatek,lcm_gpio_node");
 lcm_rst_pin = of_get_named_gpio(node, "lcm_rst_pin", 0);
 lcm_gpio_enn = of_get_named_gpio(node, "lcm_gpio_enn", 0);
  lcm_gpio_enp = of_get_named_gpio(node, "lcm_gpio_enp", 0);
  gpio_request(lcm_rst_pin, "lcm_rst_pin");
  gpio_request(lcm_gpio_enn, "lcm_gpio_enn");
 gpio_request(lcm_gpio_enp, "lcm_gpio_enp");

}

static void lcm_rst_pin_set(unsigned int GPIO, unsigned int output)
{
	
	lcm_get_pin();
	gpio_set_value(GPIO, output);

	
	}



static void lcm_update(unsigned int x, unsigned int y,
        unsigned int width, unsigned int height)
{
    unsigned int x0 = x;
    unsigned int y0 = y;
    unsigned int x1 = x0 + width - 1;
    unsigned int y1 = y0 + height - 1;

    unsigned char x0_MSB = ((x0>>8)&0xFF);
    unsigned char x0_LSB = (x0&0xFF);
    unsigned char x1_MSB = ((x1>>8)&0xFF);
    unsigned char x1_LSB = (x1&0xFF);
    unsigned char y0_MSB = ((y0>>8)&0xFF);
    unsigned char y0_LSB = (y0&0xFF);
    unsigned char y1_MSB = ((y1>>8)&0xFF);
    unsigned char y1_LSB = (y1&0xFF);

    unsigned int data_array[16];

    data_array[0]= 0x00053902;
    data_array[1]= (x1_MSB<<24)|(x0_LSB<<16)|(x0_MSB<<8)|0x2a;
    data_array[2]= (x1_LSB);
    dsi_set_cmdq(&data_array, 3, 1);
    
    data_array[0]= 0x00053902;
    data_array[1]= (y1_MSB<<24)|(y0_LSB<<16)|(y0_MSB<<8)|0x2b;
    data_array[2]= (y1_LSB);
    dsi_set_cmdq(&data_array, 3, 1);
    
    data_array[0]= 0x00290508;
    dsi_set_cmdq(&data_array, 1, 1);
    
    data_array[0]= 0x002c3909;
    dsi_set_cmdq(&data_array, 1, 0);


}

static void lcm_init(void)
{

	#ifdef BUILD_LK
		printf("sym  lcm_init start\n");
	#else
		printk("sym  lcm_init  start\n");
	#endif

 lcm_get_pin();

//gpio_direction_input(GPIO_READ_ID_PIN);//设置为输入
//gpio_get_value(GPIO_READ_ID_PIN);//获取输入电平，你自己弄个变量保存一下。
  gpio_direction_output(lcm_gpio_enn, 0);//设为输出 	
   	MDELAY(10);
  gpio_direction_output(lcm_gpio_enp, 0);//设为输出
 	MDELAY(10);
	gpio_set_value(lcm_gpio_enn, 1);//设为输出高电平
	MDELAY(10);
	gpio_set_value(lcm_gpio_enp, 1);//设为输出高电平
	MDELAY(10);

	SET_RESET_PIN(1);
	MDELAY(10);
	SET_RESET_PIN(0);
	MDELAY(20);
	SET_RESET_PIN(1);
	MDELAY(120);

	push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
	#ifdef BUILD_LK
		printf("sym  lcm_init end\n");
	#else
		printk("sym  lcm_init end\n");
	#endif
}

static void lcm_suspend(void)
{	
	push_table(lcm_deep_sleep_mode_in_setting, sizeof(lcm_deep_sleep_mode_in_setting) / sizeof(struct LCM_setting_table), 1);
	
	SET_RESET_PIN(1);
	MDELAY(10); 
	SET_RESET_PIN(0);
	MDELAY(10);
	SET_RESET_PIN(1);	
	MDELAY(120);
//gpio_direction_input(GPIO_READ_ID_PIN);//设置为输入
//gpio_get_value(GPIO_READ_ID_PIN);//获取输入电平，你自己弄个变量保存一下。
 gpio_direction_output(lcm_gpio_enn, 0);//设为输出
 	MDELAY(10);
 gpio_direction_output(lcm_gpio_enp, 0);//设为输出
	MDELAY(10);
	gpio_set_value(lcm_gpio_enn, 0);//设为输出low电平
	MDELAY(10);
	gpio_set_value(lcm_gpio_enp, 0);//设为输出low电平
MDELAY(10);

}


static void lcm_resume(void)
{
    lcm_init();
}


static unsigned int lcm_compare_id(void)
{
	int array[4];
	char buffer[5];
	char id_high=0;
	char id_low=0;
	int id=0;

	SET_RESET_PIN(1);
	MDELAY(10);
	SET_RESET_PIN(0);
	MDELAY(10);
	SET_RESET_PIN(1);
	MDELAY(200);

	array[0] = 0x00063902;
	array[1] = 0x0698FFFF;
	array[2] = 0x00000104;
	dsi_set_cmdq(array, 3, 1);
        MDELAY(10);

	array[0] = 0x00013700;
	dsi_set_cmdq(array, 1, 1);
	read_reg_v2(0x00, &buffer[0], 1);  //0x98

	array[0] = 0x00013700;		
	dsi_set_cmdq(array, 1, 1);
	read_reg_v2(0x01, &buffer[1], 1);  //0x06

	array[0] = 0x00013700;
	dsi_set_cmdq(array, 1, 1);
	read_reg_v2(0x02, &buffer[2], 1);  //0x04

	id_high = buffer[0];
	id_low = buffer[1];
	id = (id_high<<8) | id_low;

	
       #ifdef BUILD_LK
		printf("zbuffer %s \n", __func__);
		printf("%s id = 0x%08x \n", __func__, id);
	#else
		printk("zbuffer %s \n", __func__);
		printk("%s id = 0x%08x \n", __func__, id);
	
       #endif
	 
  	return (LCM_ID_ILI9806 == id)?1:0;

}


// zhoulidong  add for lcm detect (start)
static unsigned int rgk_lcm_compare_id(void)
{
    int data[4] = {0,0,0,0};
    int res = 0;
    int rawdata = 0;
    int lcm_vol = 0;

#ifdef AUXADC_LCM_VOLTAGE_CHANNEL
    res = IMM_GetOneChannelValue(AUXADC_LCM_VOLTAGE_CHANNEL,data,&rawdata);
    if(res < 0)
    { 
	#ifdef BUILD_LK
	printf("[adc_uboot]: jinmin get data error\n");
	#endif
	return 0;		   
    }
#endif

    lcm_vol = data[0]*1000+data[1]*10;

    #ifdef BUILD_LK
    printf("[adc_uboot]: jinmin lcm_vol= %d\n",lcm_vol);
    #endif
	
    if (lcm_vol>=MIN_VOLTAGE &&lcm_vol <= MAX_VOLTAGE)// && lcm_compare_id())
    {
	return 1;
    }

    return 0;

}

//add by yangjuwei 
static unsigned int lcm_ata_check(unsigned char *buffer)
{
		return 1;
#ifndef BUILD_LK
		int array[4];
		char buf[3];
		char id_high=0;
		char id_low=0;
		int id=0;
		
	    array[0] = 0x00043902;
		array[1] = 0x016098FF;
		dsi_set_cmdq(array, 2, 1);
	
		array[0] = 0x00013700;
		dsi_set_cmdq(array, 1, 1);
		read_reg_v2(0x01, &buf[0], 1);  //0x81
		id = buf[0];
		return (0x06 == id)?1:0;
#else
	return 0;
#endif
}

LCM_DRIVER ili9806e_dsi_vdo_cw_fwvga_d517g_lcm_drv = 
{
	.name			= "ili9806e_dsi_vdo_cw_fwvga_d517g",
	.set_util_funcs	= lcm_set_util_funcs,
	.get_params		= lcm_get_params,
	.init			= lcm_init,
	.suspend        = lcm_suspend,
	.resume         = lcm_resume,
	.compare_id     = rgk_lcm_compare_id,
	.ata_check		= lcm_ata_check,
#if (LCM_DSI_CMD_MODE)
	//.set_backlight	= lcm_setbacklight,
	//.update		= lcm_update,
#endif
};

