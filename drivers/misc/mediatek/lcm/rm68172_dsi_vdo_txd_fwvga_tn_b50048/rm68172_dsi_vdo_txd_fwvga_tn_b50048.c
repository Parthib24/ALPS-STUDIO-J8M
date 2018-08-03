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

#define FRAME_WIDTH                                          (480)
#define FRAME_HEIGHT                                         (854)

#define REGFLAG_DELAY                                        0xFFE
#define REGFLAG_END_OF_TABLE                      			 0x1FF   // END OF REGISTERS MARKER

#define LCM_ID_ILI9806						0x9806
#define LCM_DSI_CMD_MODE                                    0
#define LCM_RM68172_ID                         (0x8172)

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
		
	params->dsi.vertical_sync_active				= 2;// 3    2
	params->dsi.vertical_backporch					= 14;// 20   1
	params->dsi.vertical_frontporch					= 16; // 1  12
	params->dsi.vertical_active_line				= FRAME_HEIGHT; 

	params->dsi.horizontal_sync_active              = 8;// 50  2
	params->dsi.horizontal_backporch				= 32;//90
	params->dsi.horizontal_frontporch				= 32;//90
	params->dsi.horizontal_active_pixel             = FRAME_WIDTH;

//	params->dsi.HS_TRAIL                            = 15;
		/*ui = 1000/(dis_params->PLL_CLOCK*2) + 0x01; 
		cycle_time = 8000/(dis_params->PLL_CLOCK*2) + 0x01;
		HS_TRAIL = (0x04*ui + 0x50)/cycle_time;*/
	//params->dsi.LPX=8; 

	params->dsi.PLL_CLOCK                           =172;//208//270	
	params->dsi.ssc_disable							= 1;
//	params->dsi.ssc_range							= 4;

	params->physical_width = 62;
	params->physical_height = 110;

//huihuang.shi
	params->dsi.esd_check_enable 					= 1;
	params->dsi.noncont_clock  											= 1;
  params->dsi.noncont_clock_period                = 3;
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
	// Display off sequence
	{0x28, 1, {0x00}},
	{REGFLAG_DELAY, 120, {}},

	// Sleep Mode On
	{0x10, 1, {0x00}},
	{REGFLAG_DELAY, 50, {}},
	{0x4F, 1, {0x01}},
	{REGFLAG_DELAY, 50, {}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};

static struct LCM_setting_table lcm_compare_id_setting[] = {
	{0xf0, 5, {0x55, 0xaa, 0x52, 0x08, 0x01}},
	{REGFLAG_DELAY, 10, {}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};

static struct LCM_setting_table lcm_initialization_setting[] = {
{0xF0,05,{0x55,0xAA,0x52,0x08,0x02}},

{0xEA,06,{0x01,0x00,0x7D,0x00,0x00,0x00}},

{0xF6,02,{0x60,0x40}},

{0xFE,11,{0x01,0x80,0x09,0x09,0x00,0x09,0x00,0x40,0x05,0x14,0x5E}},

{0xF0,05,{0x55,0xAA,0x52,0x08,0x01}},

{0xB0,01,{0x0E}},

{0xB1,01,{0x0F}},

{0xB5,01,{0x08}},

{0xB6,01,{0x54}},

{0xB7,01,{0x44}},

{0xB8,01,{0x24}},

{0xB9,01,{0x34}},

{0xBA,01,{0x14}},

{0xBC,03,{0x00,0x78,0x15}},

{0xBD,03,{0x00,0x78,0x15}},

{0xD1, 52,{0x00, 0x00, 0x00, 0x0A, 0x00, 0x37, 0x00, 0x6F, 0x00, 0xA3, 0x00, 0xF0, 0x01, 0x26, 0x01, 0x6D, 0x01, 0x9D, 0x01, 0xDD, 0x02, 0x09, 0x02, 0x4A, 0x02, 0x7B, 0x02, 0x7C, 0x02, 0xA7, 0x02, 0xD3, 0x02, 0xED, 0x03, 0x0A, 0x03, 0x1D, 0x03, 0x34, 0x03, 0x44, 0x03, 0x59, 0x03, 0x67, 0x03, 0x7E, 0x03, 0xAD, 0x03, 0xFF}},

{0xD2, 52,{0x00, 0x00, 0x00, 0x0A, 0x00, 0x37, 0x00, 0x6F, 0x00, 0xA3, 0x00, 0xF0, 0x01, 0x26, 0x01, 0x6D, 0x01, 0x9D, 0x01, 0xDD, 0x02, 0x09, 0x02, 0x4A, 0x02, 0x7B, 0x02, 0x7C, 0x02, 0xA7, 0x02, 0xD3, 0x02, 0xED, 0x03, 0x0A, 0x03, 0x1D, 0x03, 0x34, 0x03, 0x44, 0x03, 0x59, 0x03, 0x67, 0x03, 0x7E, 0x03, 0xAD, 0x03, 0xFF}},

{0xD3, 52,{0x00, 0x00, 0x00, 0x0A, 0x00, 0x37, 0x00, 0x6F, 0x00, 0xA3, 0x00, 0xF0, 0x01, 0x26, 0x01, 0x6D, 0x01, 0x9D, 0x01, 0xDD, 0x02, 0x09, 0x02, 0x4A, 0x02, 0x7B, 0x02, 0x7C, 0x02, 0xA7, 0x02, 0xD3, 0x02, 0xED, 0x03, 0x0A, 0x03, 0x1D, 0x03, 0x34, 0x03, 0x44, 0x03, 0x59, 0x03, 0x67, 0x03, 0x7E, 0x03, 0xAD, 0x03, 0xFF}},

{0xD4, 52,{0x00, 0x00, 0x00, 0x0A, 0x00, 0x37, 0x00, 0x6F, 0x00, 0xA3, 0x00, 0xF0, 0x01, 0x26, 0x01, 0x6D, 0x01, 0x9D, 0x01, 0xDD, 0x02, 0x09, 0x02, 0x4A, 0x02, 0x7B, 0x02, 0x7C, 0x02, 0xA7, 0x02, 0xD3, 0x02, 0xED, 0x03, 0x0A, 0x03, 0x1D, 0x03, 0x34, 0x03, 0x44, 0x03, 0x59, 0x03, 0x67, 0x03, 0x7E, 0x03, 0xAD, 0x03, 0xFF}},

{0xD5, 52,{0x00, 0x00, 0x00, 0x0A, 0x00, 0x37, 0x00, 0x6F, 0x00, 0xA3, 0x00, 0xF0, 0x01, 0x26, 0x01, 0x6D, 0x01, 0x9D, 0x01, 0xDD, 0x02, 0x09, 0x02, 0x4A, 0x02, 0x7B, 0x02, 0x7C, 0x02, 0xA7, 0x02, 0xD3, 0x02, 0xED, 0x03, 0x0A, 0x03, 0x1D, 0x03, 0x34, 0x03, 0x44, 0x03, 0x59, 0x03, 0x67, 0x03, 0x7E, 0x03, 0xAD, 0x03, 0xFF}},

{0xD6, 52,{0x00, 0x00, 0x00, 0x0A, 0x00, 0x37, 0x00, 0x6F, 0x00, 0xA3, 0x00, 0xF0, 0x01, 0x26, 0x01, 0x6D, 0x01, 0x9D, 0x01, 0xDD, 0x02, 0x09, 0x02, 0x4A, 0x02, 0x7B, 0x02, 0x7C, 0x02, 0xA7, 0x02, 0xD3, 0x02, 0xED, 0x03, 0x0A, 0x03, 0x1D, 0x03, 0x34, 0x03, 0x44, 0x03, 0x59, 0x03, 0x67, 0x03, 0x7E, 0x03, 0xAD, 0x03, 0xFF}},

{0xF0,05,{0x55,0xAA,0x52,0x08,0x03}},

{0xB0,07,{0x05,0x17,0xF9,0x56,0x00,0x00,0x30}},

{0xB2, 9,{0xFE,0xFF,0x00,0x01,0x30,0x00,0x00,0xC4,0x08}},

{0xB3,06,{0x5B,0x00,0xFE,0x5C,0x58,0x00}},

{0xB4,11,{0x02,0x03,0x04,0x05,0x00,0x40,0x04,0x08,0x00,0x00,0x00}},

{0xB5,11,{0x00,0x44,0x02,0x80,0x60,0x5C,0x5C,0x5C,0x33,0x33,0x00}},

{0xB6,07,{0x02,0x00,0x00,0x00,0x0A,0x00,0x00}},

{0xB7, 8,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}},

{0xB8,03,{0x12,0x00,0x00}},

{0xB9,01,{0x90}},

{0xBA,16,{0xF5,0x41,0x3F,0xF9,0xDB,0xFF,0xFF,0xFF,0xFF,0xFF,0xFE,0xAC,0x8F,0xF2,0x04,0x5F}},

{0xBB,16,{0xF4,0x52,0x0F,0xFE,0xAC,0x8F,0xFF,0xFF,0xFF,0xFF,0xF9,0xDB,0xFF,0xF1,0x35,0x4F}},

{0xBC,04,{0xE6,0x1F,0xF8,0x67}},

{0xBD,04,{0xE6,0x1F,0xF8,0x67}},

{0xF0,05,{0x55,0xAA,0x52,0x08,0x00}},

{0xB0,02,{0x00,0x10}},

{0xB1,02,{0x78,0x00}},

{0xBA,01,{0x01}},

{0xB4,01,{0x10}},

{0xB5,01,{0x6B}},

{0xBC,01,{0x00}},

{0x35,01,{0x00}},

{0x11,01,{0x00}},

{REGFLAG_DELAY, 120, {0}},	

{0x29,01,{0x00}},

{REGFLAG_DELAY, 20, {0}},

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
  unsigned int id = 0;
  unsigned char buffer[5];
  unsigned int array[16];
 
// lcm_power_on();
 
  SET_RESET_PIN(1);
  MDELAY(10);
  SET_RESET_PIN(0);
  MDELAY(50);
  SET_RESET_PIN(1);
  MDELAY(120);
 
  push_table(lcm_compare_id_setting, sizeof(lcm_compare_id_setting) / sizeof(struct LCM_setting_table), 1);
 
  array[0] = 0x00033700;
  dsi_set_cmdq(array, 1, 1);
  MDELAY(5);
  read_reg_v2(0xC5, buffer, 3);
  id = ((buffer[1] << 8) | buffer[0]); //we only need ID
 
#if defined(BUILD_LK)
  printf("%s, [rm68172_ctc40_txd_fwvga]  buffer[0] = [0x%x] buffer[2] = [0x%x] ID = [0x%x]\n",__func__, buffer[0], buffer[1], id);
#else
  printk("%s, [rm68172_ctc40_txd_fwvga]  buffer[0] = [0x%x] buffer[2] = [0x%x] ID = [0x%x]\n",__func__, buffer[0], buffer[1], id);
#endif

  return ((LCM_RM68172_ID == id)? 1 : 0);
}
/*

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
*/
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
#endif*/
}

LCM_DRIVER rm68172_dsi_vdo_txd_fwvga_tn_b50048_lcm_drv = 
{
	.name			= "rm68172_dsi_vdo_txd_fwvga_tn_b50048",
	.set_util_funcs	= lcm_set_util_funcs,
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

