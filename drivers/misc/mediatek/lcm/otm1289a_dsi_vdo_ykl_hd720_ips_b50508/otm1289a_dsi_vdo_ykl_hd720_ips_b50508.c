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
#define THREE_POWER_MIN_VOLTAGE (0)	
#define THREE_POWER_MAX_VOLTAGE (100)	

#define  TRUE   1 
#define  FALSE  0
#define FRAME_WIDTH                                          (720)
#define FRAME_HEIGHT                                         (1280)

#define REGFLAG_DELAY                                         0XFFE
#define REGFLAG_END_OF_TABLE                      			 0x1FF   // END OF REGISTERS MARKER

#define LCM_DSI_CMD_MODE                                    0

//static unsigned int lcm_esd_test = FALSE;      ///only for ESD test

static LCM_UTIL_FUNCS lcm_util = {0};

//#define SET_RESET_PIN(v)    (lcm_util.set_reset_pin((v)))
static void lcm_rst_pin_set(unsigned int GPIO, unsigned int output);
#define SET_RESET_PIN(v)    lcm_rst_pin_set(lcm_rst_pin,v)//(lcm_util.set_reset_pin((v)))

#define UDELAY(n) 				(lcm_util.udelay(n))
#define MDELAY(n) 				(lcm_util.mdelay(n))

//extern int ktd2150_write_bytes(unsigned char addr, unsigned char data);
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

#ifndef BUILD_LK
extern atomic_t ESDCheck_byCPU;
#endif

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
		
		params->dsi.vertical_sync_active				= 3;// 3    2
		params->dsi.vertical_backporch					= 20;// 20   1
		params->dsi.vertical_frontporch					= 12; // 1  12
		params->dsi.vertical_active_line				= FRAME_HEIGHT; 

		params->dsi.horizontal_sync_active				= 10;// 50  2
		params->dsi.horizontal_backporch				= 40;//90
		params->dsi.horizontal_frontporch				= 40;//90
		params->dsi.horizontal_active_pixel				= FRAME_WIDTH;
		params->dsi.ssc_disable                         = 1;
		params->dsi.HS_TRAIL                             = 15;
		/*ui = 1000/(dis_params->PLL_CLOCK*2) + 0x01; 
		cycle_time = 8000/(dis_params->PLL_CLOCK*2) + 0x01;
		HS_TRAIL = (0x04*ui + 0x50)/cycle_time;*/
	//params->dsi.LPX=8; 
	
		params->physical_width = 62;
		params->physical_height = 110;

	// Bit rate calculation
		//1 Every lane speed
		//params->dsi.pll_select=1;
		//params->dsi.PLL_CLOCK  = LCM_DSI_6589_PLL_CLOCK_377;
		params->dsi.PLL_CLOCK=200;//208//270
		//params->dsi.pll_div1=0;		// div1=0,1,2,3;div1_real=1,2,4,4 ----0: 546Mbps  1:273Mbps
		//params->dsi.pll_div2=0;		// div2=0,1,2,3;div1_real=1,2,4,4	

	//	params->dsi.fbk_div =7;    // fref=26MHz, fvco=fref*(fbk_div+1)*2/(div1_real*div2_real)	
		//params->dsi.compatibility_for_nvk = 1;		// this parameter would be set to 1 if DriverIC is NTK's and when force match DSI clock for NTK's
#if 0
        params->dsi.esd_check_enable = 0;
		params->dsi.customization_esd_check_enable = 0;
		params->dsi.lcm_esd_check_table[0].cmd			= 0x0a;
		params->dsi.lcm_esd_check_table[0].count		= 1;
		params->dsi.lcm_esd_check_table[0].para_list[0] = 0x9c;
#else
		params->dsi.esd_check_enable = 1;
		params->dsi.customization_esd_check_enable = 1;
		params->dsi.lcm_esd_check_table[0].cmd			= 0x0a;
		params->dsi.lcm_esd_check_table[0].count		= 1;
		params->dsi.lcm_esd_check_table[0].para_list[0] = 0x9c;
#endif
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

                      
{0x00,1,{0x00}},
{0xff,3,{0x12,0x89,0x01}},
{0x00,1,{0x80}},
{0xff,2,{0x12,0x89}},
                                         
{0x00,1,{0x90}},
{0xFF,1,{0xB0}},  //MIPI 4:0xb0, 3:0xa0, 2:0x90
                      
{0x00,1,{0x80}},
{0xc0,8,{0X4a,0x00,0x10,0x10,0x96,0x01,0x68,0x40}},//0xc0,0X4a,0x00,0x10,0x10,0x96,0x01,0x68,0x40}},  //TCON Setting
                      
{0x00,1,{0x90}},
{0xc0,3,{0x3b,0x01,0x09}},          //Panel Timing Setting
                      
{0x00,1,{0x8c}},
{0xc0,1,{0x00}},                    //column inversion
                      
{0x00,1,{0x80}},
{0xc1,1,{0x33}},                  //frame rate:60Hz
                      
{0x00,1,{0x85}},
{0xc5,3,{0x09,0x0b,0x19}},       //VGH=3x, VGL=-3x, VGH=12V, VGL=-12V
                      
{0x00,1,{0x00}},
{0xd8,2,{0x27,0x27}},         //GVDD=4.8V, NGVDD=-4.8V
                      
{0x00,1,{0x00}},
{0xd9,2,{0x00,0x5E}},           //VCOM=-1.2875V
                      
{0x00,1,{0x84}},
{0xC4,1,{0x02}},               //chopper
                      
{0x00,1,{0x93}},
{0xC4,1,{0x04}},              //pump option
                      
{0x00,1,{0x96}}, 
{0xF5,1,{0xE7}},            //VCL regulator
                      
{0x00,1,{0xA0}}, 
{0xF5,1,{0x4A}},           //pump3 off
                      
{0x00,1,{0x8a}},
{0xc0,1,{0x11}},           //blank frame
                      
{0x00,1,{0x83}}, 
{0xF5,1,{0x81}},          //vcom active                      
             
{0x00,1,{0x90}},
{0xc4,2,{0x95,0x05}},       //  Power IC: 2xVPNL, x1.5=01, x2=05, x3=09
                      
//-------------------- panel enmode control --------------------//
                      
{0x00,1,{0x80}},
{0xcb,15,{0x14,0x14,0x14,0x14,0x14,0x14,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}},       //panel timing state control
                      
{0x00,1,{0x90}},
{0xcb,7,{0x00,0x00,0x00,0x00,0x00,0x14,0x14}},     //panel timing state control
                      
//-------------------- panel u2d/d2u mapping control --------------------//
                      
{0x00,1,{0x80}},
{0xcc,14,{0x0a,0x0c,0x0e,0x10,0x02,0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}},    //panel timing state control
                      
{0x00,1,{0x90}},
{0xcc,15,{0x00,0x00,0x00,0x00,0x00,0x1e,0x1d,0x09,0x0b,0x0d,0x0f,0x01,0x03,0x00,0x00}},    //panel timing state control
                      
{0x00,1,{0xa0}},
{0xcc,13,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x1e,0x1d}},      //panel timing state control
                      
{0x00,1,{0xb0}},
{0xcc,14,{0x10,0x0e,0x0c,0x0a,0x04,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}},    //panel timing state control
                      
{0x00,1,{0xc0}},
{0xcc,15,{0x00,0x00,0x00,0x00,0x00,0x1d,0x1e,0x0f,0x0d,0x0b,0x09,0x03,0x01,0x00,0x00}},   //panel timing state contro
                      
{0x00,1,{0xd0}},
{0xcc,13,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x1d,0x1e}},         //panel timing state control
                      
//-------------------- goa timing setting --------------------//
                     
{0x00,1,{0x80}},
{0xce,6,{0x8D,0x03,0x00,0x8C,0x8B,0x8A}},         //panel VST setting
                      
{0x00,1,{0x90}},
{0xce,9,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}},    //panel VEND setting
                      
{0x00,1,{0xa0}},
{0xce,15,{0x30,0x8B,0x84,0x8B,0x04,0x00,0x8A,0x83,0x8B,0x89,0x82,0x8B,0x88,0x81,0x8B}},     //panel CLKA setting
                      
{0x00,1,{0xb0}},
{0xce,15,{0x30,0x87,0x80,0x8B,0x04,0x00,0x86,0x00,0x8B,0x85,0x01,0x8B,0x84,0x02,0x8B}},     //panel CLKB setting
                      
{0x00,1,{0xc0}},
{0xce,15,{0x30,0x00,0x82,0x00,0x00,0x00,0x81,0x00,0x00,0x80,0x00,0x00,0x00,0x00,0x00}},     //panel CLKC setting
                      
{0x00,1,{0xd0}},
{0xce,15,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}},     //panel CLKD setting
                      
{0x00,1,{0xf0}},
{0xce,6,{0x01,0x00,0xf1,0x01,0x00,0x00}},                                                  //panel ECLK setting
                      
//-------------------- gamma 2.2 -----------------------------------------//
           
{0x00,1,{0x00}},
{0xE1,16,{0x0c,0x18,0x22,0x30,0x3c,0x59,0x57,0x6d,0x89,0x77,0x81,0x67,0x4d,0x37,0x24,0x06}},
                      
{0x00,1,{0x00}},
{0xE2,16,{0x0c,0x18,0x22,0x30,0x3c,0x59,0x57,0x6d,0x89,0x77,0x81,0x67,0x4d,0x37,0x24,0x06}},
                      
{0x00,1,{0x00}},
{0xff,3,{0xff,0xff,0xff}},        //CMD2 Disable
                      
{0x11,1,{0x00}},//SLEEP OUT      
{REGFLAG_DELAY,120,{}},        
                                 
{0x29,1,{0x00}},//Display ON   
{REGFLAG_DELAY,20,{}},  
{REGFLAG_END_OF_TABLE, 0x00, {}}
	
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
    dsi_set_cmdq(data_array, 3, 1);
    
    data_array[0]= 0x00053902;
    data_array[1]= (y1_MSB<<24)|(y0_LSB<<16)|(y0_MSB<<8)|0x2b;
    data_array[2]= (y1_LSB);
    dsi_set_cmdq(data_array, 3, 1);
    
    data_array[0]= 0x00290508;
    dsi_set_cmdq(data_array, 1, 1);
    
    data_array[0]= 0x002c3909;
    dsi_set_cmdq(data_array, 1, 0);
}
#endif

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
	MDELAY(30);
//	ktd2150_write_bytes(addr,data);
	MDELAY(50);
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
	
	
//	ktd2150_write_bytes(addr,data);
	MDELAY(50);

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
#if 1
static unsigned int lcm_compare_id(void)
	{
		
		int array[4];
		char buffer[4];
		//char id_high=0;
		//char id_low=0;
		int id=0;
	
		SET_RESET_PIN(1);
		MDELAY(20); 
		SET_RESET_PIN(0);
		MDELAY(20);
		SET_RESET_PIN(1);
		MDELAY(120);
/*	
		//{0x39, 0xFF, 5, { 0xFF,0x98,0x06,0x04,0x01}}, // Change to Page 1 CMD
		array[0] = 0x00043902;
		array[1] = 0x018198FF;
		dsi_set_cmdq(array, 2, 1);
*/	
		array[0] = 0x00043902;
		dsi_set_cmdq(array, 1, 1);
		read_reg_v2(0xa1, buffer, 1);  //0x12
	
		id = buffer[2];
	
		
   #ifdef BUILD_LK
		  
			printf("jinmin [lk]=%d %d %d \n", buffer[0],buffer[1], buffer[2]);
			printf("id =0x%x\n", id);
#else
			printk("jinmin [kernel]=%d %d %d \n", buffer[0],buffer[1], buffer[2]);
			printk("id =0x%x\n", id);
   #endif
	
		return (0x12 == id)?1:0;
	
	}
#endif


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
	
    if (lcm_vol>=THREE_POWER_MIN_VOLTAGE &&lcm_vol <= THREE_POWER_MAX_VOLTAGE)// && lcm_compare_id())
    { 	
    	 #ifdef BUILD_LK
    	 printf("lxf is at four power\n");
    	 #else 
    	 printk("lxf is at four power\n");
    	 #endif 
   		 return 1;
    }
    else 
    	return 0; 
}
//add by yangjuwei 



static unsigned int lcm_ata_check(unsigned char *buffer)
{
	return 1;
/*#ifndef BUILD_LK
			int array[4];
			char buf[3];
			//char id_high=0;
			//char id_low=0;
			int id=0;
		
	    array[0] = 0x00043902;
		array[1] = 0x018198FF;
		dsi_set_cmdq(array, 2, 1);
	
		array[0] = 0x00013700;
		dsi_set_cmdq(array, 1, 1);
		atomic_set(&ESDCheck_byCPU,1);
		read_reg_v2(0x00, &buf[0], 1);  //0x98
		atomic_set(&ESDCheck_byCPU,0);
		id = buf[0];	
			return (0x98 == id)?1:0;
#else
	return 1;
#endif*/
}

LCM_DRIVER otm1289a_dsi_vdo_ykl_hd720_ips_b50508_lcm_drv = 
{
	.name			= "otm1289a_dsi_vdo_ykl_hd720_ips_b50508",
	.set_util_funcs		= lcm_set_util_funcs,
	.get_params		= lcm_get_params,
	.init			= lcm_init,
    .suspend        = lcm_suspend,
    .resume         = lcm_resume,
    .compare_id     = lcm_compare_id,
	.ata_check		= lcm_ata_check,
#if (LCM_DSI_CMD_MODE)
	//.set_backlight	= lcm_setbacklight,
	//.update		= lcm_update,
#endif
};

