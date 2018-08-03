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
#include <mt-plat/mt_gpio.h>
static unsigned int lcm_rst_pin;
static unsigned int lcm_gpio_enn;
static unsigned int lcm_gpio_enp;

//#include <cust_adc.h>    	
#define AUXADC_LCM_VOLTAGE_CHANNEL     12
#define MIN_VOLTAGE (-100)	
#define MAX_VOLTAGE (100)	

#define FRAME_WIDTH                                          (720)
#define FRAME_HEIGHT                                         (1280)

#define REGFLAG_DELAY                                         0XFFE
#define REGFLAG_END_OF_TABLE                      			 0x1FF   // END OF REGISTERS MARKER

#define LCM_DSI_CMD_MODE                                    0

//static unsigned int lcm_esd_test = FALSE;      ///only for ESD test

static LCM_UTIL_FUNCS lcm_util = {0};
static void lcm_rst_pin_set(unsigned int GPIO, unsigned int output);
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

struct LCM_setting_table {
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
		params->dsi.LANE_NUM				= LCM_FOUR_LANE;
	//The following defined the fomat for data coming from LCD engine.
	params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

	// Video mode setting		
	params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
		
		params->dsi.vertical_sync_active				= 4;// 3    2
		params->dsi.vertical_backporch					= 20;// 20   1
		params->dsi.vertical_frontporch					= 20; // 1  12
		params->dsi.vertical_active_line				= FRAME_HEIGHT; 

		params->dsi.horizontal_sync_active				= 30;// 50  2
		params->dsi.horizontal_backporch				= 100;//90
		params->dsi.horizontal_frontporch				=  100;//90
		params->dsi.horizontal_active_pixel				= FRAME_WIDTH;

	//params->dsi.LPX=8; 

	// Bit rate calculation
		//1 Every lane speed
		//params->dsi.pll_select=1;
		//params->dsi.PLL_CLOCK  = LCM_DSI_6589_PLL_CLOCK_377;
		params->dsi.PLL_CLOCK=250;//208//270
		params->dsi.ssc_disable							= 1;
	    params->dsi.ssc_range							= 4;
	     params->dsi.HS_TRAIL							= 15;
		//params->dsi.pll_div1=0;		// div1=0,1,2,3;div1_real=1,2,4,4 ----0: 546Mbps  1:273Mbps
		//params->dsi.pll_div2=0;		// div2=0,1,2,3;div1_real=1,2,4,4	

	//	params->dsi.fbk_div =7;    // fref=26MHz, fvco=fref*(fbk_div+1)*2/(div1_real*div2_real)	
		//params->dsi.compatibility_for_nvk = 1;		// this parameter would be set to 1 if DriverIC is NTK's and when force match DSI clock for NTK's
	
		params->dsi.esd_check_enable = 1;
		params->dsi.customization_esd_check_enable = 1;
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

{0xFF,03,{0x98,0x81,0x03}},

{0x01,01,{0x00}},

{0x02,01,{0x00}},

{0x03,01,{0x73}},

{0x04,01,{0x73}},

{0x05,01,{0x00}},

{0x06,01,{0x06}},

{0x07,01,{0x02}},

{0x08,01,{0x00}},

{0x09,01,{0x01}},

{0x0a,01,{0x01}},

{0x0b,01,{0x01}},

{0x0c,01,{0x01}},

{0x0d,01,{0x01}},

{0x0e,01,{0x01}},

{0x0f,01,{0x2F}},       

{0x10,01,{0x2F}},       

{0x11,01,{0x00}},

{0x12,01,{0x00}},

{0x13,01,{0x01}},

{0x14,01,{0x00}},

{0x15,01,{0x00}},

{0x16,01,{0x00}}, 

{0x17,01,{0x00}},

{0x18,01,{0x00}},

{0x19,01,{0x00}},

{0x1a,01,{0x00}},

{0x1b,01,{0x00}},

{0x1c,01,{0x00}},

{0x1d,01,{0x00}},

{0x1e,01,{0xC0}},

{0x1f,01,{0x80}},

{0x20,01,{0x03}},      

{0x21,01,{0x03}},

{0x22,01,{0x00}},

{0x23,01,{0x00}},

{0x24,01,{0x00}},

{0x25,01,{0x00}},

{0x26,01,{0x00}},

{0x27,01,{0x00}},

{0x28,01,{0x33}},

{0x29,01,{0x02}},       

{0x2a,01,{0x00}},

{0x2b,01,{0x00}},

{0x2c,01,{0x00}},

{0x2d,01,{0x00}},

{0x2e,01,{0x00}},

{0x2f,01,{0x00}},

{0x30,01,{0x00}},

{0x31,01,{0x00}},

{0x32,01,{0x00}},

{0x33,01,{0x00}},  

{0x34,01,{0x03}},

{0x35,01,{0x00}},

{0x36,01,{0x03}},

{0x37,01,{0x00}},

{0x38,01,{0x00}},

{0x39,01,{0x00}},

{0x3a,01,{0x00}}, 

{0x3b,01,{0x00}},

{0x3c,01,{0x00}},

{0x3d,01,{0x00}},

{0x3e,01,{0x00}},

{0x3f,01,{0x00}},

{0x40,01,{0x00}},

{0x41,01,{0x00}},

{0x42,01,{0x00}},

{0x43,01,{0x01}},

{0x44,01,{0x00}},

{0x50,01,{0x01}},

{0x51,01,{0x23}},

{0x52,01,{0x45}},

{0x53,01,{0x67}},

{0x54,01,{0x89}},

{0x55,01,{0xab}},

{0x56,01,{0x01}},

{0x57,01,{0x23}},

{0x58,01,{0x45}},

{0x59,01,{0x67}},

{0x5a,01,{0x89}},

{0x5b,01,{0xab}},

{0x5c,01,{0xcd}},

{0x5d,01,{0xef}},

{0x5e,01,{0x10}},

{0x5f,01,{0x09}},

{0x60,01,{0x08}},

{0x61,01,{0x0F}},

{0x62,01,{0x0E}},

{0x63,01,{0x0D}},

{0x64,01,{0x0C}},

{0x65,01,{0x02}},

{0x66,01,{0x02}},

{0x67,01,{0x02}},

{0x68,01,{0x02}},

{0x69,01,{0x02}},

{0x6a,01,{0x02}},

{0x6b,01,{0x02}},

{0x6c,01,{0x02}},

{0x6d,01,{0x02}},

{0x6e,01,{0x02}},

{0x6f,01,{0x02}},

{0x70,01,{0x02}},

{0x71,01,{0x06}},

{0x72,01,{0x07}},

{0x73,01,{0x02}},

{0x74,01,{0x02}},

{0x75,01,{0x06}},

{0x76,01,{0x07}},

{0x77,01,{0x0E}},

{0x78,01,{0x0F}},

{0x79,01,{0x0C}},

{0x7a,01,{0x0D}},

{0x7b,01,{0x02}},

{0x7c,01,{0x02}},

{0x7d,01,{0x02}},

{0x7e,01,{0x02}},

{0x7f,01,{0x02}},

{0x80,01,{0x02}},

{0x81,01,{0x02}},

{0x82,01,{0x02}},

{0x83,01,{0x02}},

{0x84,01,{0x02}},

{0x85,01,{0x02}},

{0x86,01,{0x02}},

{0x87,01,{0x09}},

{0x88,01,{0x08}},

{0x89,01,{0x02}},

{0x8A,01,{0x02}},

{0xFF,03,{0x98,0x81,0x04}},

{0x6C,01,{0x15}},               

{0x6E,01,{0x2A}},               

{0x6F,01,{0x57}},               

{0x3A,01,{0xA4}},               

{0x8D,01,{0x1A}},               

{0x87,01,{0xBA}},               

{0x26,01,{0x76}},            

{0xB2,01,{0xD1}},

{0x88,01,{0x0B}},

{0x3C,01,{0x81}},

{0xFF,03,{0x98,0x81,0x01}},

{0x22,01,{0x09}},              //BGR,SS  {0x0A}:front {0x09}:back	

{0x31,01,{0x00}},		

{0x53,01,{0x43}},		

{0x55,01,{0x50}},		

{0x50,01,{0xA8}},		

{0x51,01,{0xA8}},		

{0x60,01,{0x06}},   

{0x62,01,{0x20}}, 

{0xA0,01,{0x00}},		

{0xA1,01,{0x22}},   

{0xA2,01,{0x36}},   

{0xA3,01,{0x18}},   

{0xA4,01,{0x1E}},   

{0xA5,01,{0x32}},   

{0xA6,01,{0x27}},   

{0xA7,01,{0x25}},   

{0xA8,01,{0xA6}},   

{0xA9,01,{0x1D}},              

{0xAA,01,{0x28}},              

{0xAB,01,{0x7D}},              

{0xAC,01,{0x1A}},              

{0xAD,01,{0x19}},              

{0xAE,01,{0x4D}},              

{0xAF,01,{0x23}},              

{0xB0,01,{0x2B}},              

{0xB1,01,{0x49}},              

{0xB2,01,{0x55}},              

{0xB3,01,{0x23}},                                             

{0xC0,01,{0x00}},		

{0xC1,01,{0x22}},   

{0xC2,01,{0x36}},   

{0xC3,01,{0x18}},   

{0xC4,01,{0x1E}},   

{0xC5,01,{0x32}},   

{0xC6,01,{0x27}},   

{0xC7,01,{0x25}},   

{0xC8,01,{0xA6}},   

{0xC9,01,{0x1D}},   

{0xCA,01,{0x28}},   

{0xCB,01,{0x7D}},   

{0xCC,01,{0x1A}},   

{0xCD,01,{0x19}},   

{0xCE,01,{0x4D}},   

{0xCF,01,{0x23}},                   

{0xD0,01,{0x2B}},                   

{0xD1,01,{0x49}},                   

{0xD2,01,{0x55}},                   

{0xD3,01,{0x14}},             

{0xFF,03,{0x98,0x81,0x00}},

{0x11,01,{0x00}},

{REGFLAG_DELAY, 120, {}},

{0x29,01,{0x00}},

{0x35,01,{0x00}},

};
static void lcm_get_pin(void)
{
 static struct device_node *node;
 node = of_find_compatible_node(NULL, NULL, "mediatek,lcm_v52516_gpio_node");
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

#if 0
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
#endif
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
//  mt_set_gpio_out(78, 0); //add  lujian 20160416
//  mt_set_gpio_out(80, 0); // add  lujian 20160416

}

static void lcm_resume(void)
{
    lcm_init();
}

#if 0
static unsigned int lcm_compare_id(void)
{
	
	int array[4];
	char buffer[3];
	//char id_high=0;
	//char id_low=0;
	int id=0;

	SET_RESET_PIN(1);
	MDELAY(20); 
	SET_RESET_PIN(0);
	MDELAY(20);
	SET_RESET_PIN(1);
	MDELAY(120);

	//{0x39, 0xFF, 5, { 0xFF,0x98,0x06,0x04,0x01}}, // Change to Page 1 CMD
	array[0] = 0x00043902;
	array[1] = 0x018198FF;
	dsi_set_cmdq(array, 2, 1);

	array[0] = 0x00013700;
	dsi_set_cmdq(array, 1, 1);
	read_reg_v2(0x00, &buffer[0], 1);  //0x98

	id = buffer[0];

	return (0x98 == id)?1:0;

}
#endif

#if 1
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
	
    if (lcm_vol>=MIN_VOLTAGE && lcm_vol <= MAX_VOLTAGE) //&& lcm_compare_id())
    {
	return 1;
    }

    return 0;

}
#endif 

#if 1//temp solution start
//add by yangjuwei 
static unsigned int lcm_ata_check(unsigned char *buffer)
{
	return 1;
/*#ifndef BUILD_LK
			int array[4];
			char buf[3];
			char id_high=0;
			char id_low=0;
			int id=0;
		
			//{0x39, 0xFF, 5, { 0xFF,0x98,0x06,0x04,0x01}}, // Change to Page 1 CMD
			array[0] = 0x00043902;
			array[1] = 0x018198FF;
			dsi_set_cmdq(array, 2, 1);
		
			array[0] = 0x00013700;
			dsi_set_cmdq(array, 1, 1);
			read_reg_v2(0x01, &buf[0], 1);  //0x81
			id = buf[0];
		
			return (0x81 == id)?1:0;

#else
	return 0;
#endif*/
}
#else
static unsigned int lcm_ata_check(unsigned char *buffer)
{
    int data[4] = {0,0,0,0};
    int res = 0;
    int rawdata = 0;
    int lcm_vol = 0;

#ifdef AUXADC_LCM_VOLTAGE_CHANNEL
    res = IMM_GetOneChannelValue(AUXADC_LCM_VOLTAGE_CHANNEL,data,&rawdata);
    if(res < 0)
    {
	    return 0;		   
    }
#endif

    lcm_vol = data[0]*1000+data[1]*10;
	
    if (lcm_vol>=MIN_VOLTAGE &&lcm_vol <= MAX_VOLTAGE)
    {
	    return 1;
    }
    return 0;
}
#endif//temp solution end

LCM_DRIVER ili9881c_dsi_vdo_hd720_txd_ivo_ips_v52516_lcm_drv = 
{
	.name			= "ili9881c_dsi_vdo_hd720_txd_ivo_ips_v52516",
	.set_util_funcs		= lcm_set_util_funcs,
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

