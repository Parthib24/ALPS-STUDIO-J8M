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

#define MIN_VOLTAGE (700)
#define MAX_VOLTAGE (1100)

// ---------------------------------------------------------------------------
//  Local Constantsq
// ---------------------------------------------------------------------------

#define FRAME_WIDTH  (480)
#define FRAME_HEIGHT (854)

#define REGFLAG_DELAY             							0XFFE
#define REGFLAG_END_OF_TABLE      							0xFFF   // END OF REGISTERS MARKER
#define LCM_ID_ILI9806E 										(0x980604)
// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static LCM_UTIL_FUNCS lcm_util;

#define SET_RESET_PIN(v)    (lcm_util.set_reset_pin((v)))

#define UDELAY(n) (lcm_util.udelay(n))
#define MDELAY(n) (lcm_util.mdelay(n))

// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------

#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)	        lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)		lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)										lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)					lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd)											lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size)   				lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)    

#define   LCM_DSI_CMD_MODE							(0)



struct LCM_setting_table {
    unsigned cmd;
    unsigned char count;
    unsigned char para_list[128];
};							
static struct LCM_setting_table lcm_initialization_setting[] = {
{0XFF,5,{0xFF,0x98,0x06,0x04,0x01}},

{0X08,1,{0x10}},  
{0X21,1,{0x01}},  
{0X30,1,{0x01}},  
{0X31,1,{0x02}}, //00 COLUNM  02 2DOT     
{0X40,1,{0x18}},  
{0X41,1,{0x33}},      
{0X42,1,{0x03}},  
{0X43,1,{0x09}},  
{0X44,1,{0x07}},
{0X50,1,{0x78}},  
{0X51,1,{0x78}},  
{0X52,1,{0x00}},  
{0X53,1,{0x40}},  
{0X60,1,{0x0a}},  
{0x61,1,{0x00}},  
{0X62,1,{0x08}},  
{0X63,1,{0x00}},
//++++++++++++++++++ Gamma
{0XA0,1,{0x00}},  
{0XA1,1,{0x07}},  
{0XA2,1,{0x14}},  
{0XA3,1,{0x12}},  
{0XA4,1,{0x0B}},  
{0XA5,1,{0x1C}},  
{0XA6,1,{0x0C}},  
{0XA7,1,{0x08}},  
{0XA8,1,{0x04}},  
{0XA9,1,{0x0B}},  
{0XAA,1,{0x03}},  
{0XAB,1,{0x06}},  
{0XAC,1,{0x17}},  
{0XAD,1,{0x33}},  
{0XAE,1,{0x30}},  
{0XAF,1,{0x00}},

///==============Nagitive
{0XC0,1,{0x00}},  
{0XC1,1,{0x09}},  
{0XC2,1,{0x15}},  
{0XC3,1,{0x11}},  
{0XC4,1,{0x0B}},  
{0XC5,1,{0x19}},  
{0XC6,1,{0x08}},  
{0XC7,1,{0x09}},  
{0XC8,1,{0x04}},  
{0XC9,1,{0x08}},  
{0XCA,1,{0x07}},
{0XCB,1,{0x05}},  
{0XCC,1,{0x07}},  
{0XCD,1,{0x24}},  
{0XCE,1,{0x20}},  
{0XCF,1,{0x00}},
//**********************************//
//**********************************//
//**********************************//  
{0XFF,5,{0xFF,0x98,0x06,0x04,0x06}},  
{0X00,1,{0x21}},  
{0X01,1,{0x09}},  
{0X02,1,{0x00}},  
{0X03,1,{0x00}},  
{0X04,1,{0x01}},
{0X05,1,{0x01}},  
{0X06,1,{0x80}},  
{0X07,1,{0x05}},  
{0X08,1,{0x02}},
{0X09,1,{0x80}},  
{0X0A,1,{0x00}},  
{0X0B,1,{0x00}},
{0X0C,1,{0x0a}},
{0X0D,1,{0x0a}},
{0X0E,1,{0x00}},
{0X0F,1,{0x00}},
{0X10,1,{0xe0}},
{0X11,1,{0xe4}},
{0X12,1,{0x04}},
{0X13,1,{0x00}},
{0X14,1,{0x00}},
{0X15,1,{0xC0}},
{0X16,1,{0x08}},
{0X17,1,{0x00}},
{0X18,1,{0x00}},
{0X19,1,{0x00}},
{0X1A,1,{0x00}},
{0X1B,1,{0x00}},
{0X1C,1,{0x00}},
{0X1D,1,{0x00}},
{0X20,1,{0x01}},
{0X21,1,{0x23}},
{0X22,1,{0x45}},
{0X23,1,{0x67}},
{0X24,1,{0x01}},
{0X25,1,{0x23}},
{0X26,1,{0x45}},
{0X27,1,{0x67}},
{0X30,1,{0x01}},
{0X31,1,{0x11}},
{0X32,1,{0x00}},
{0X33,1,{0xEE}},
{0X34,1,{0xFF}},
{0X35,1,{0xBB}},
{0X36,1,{0xcA}},
{0X37,1,{0xDD}},
{0X38,1,{0xaC}},
{0X39,1,{0x76}},
{0X3A,1,{0x67}},
{0X3B,1,{0x22}},
{0X3C,1,{0x22}},
{0X3D,1,{0x22}},
{0X3E,1,{0x22}},
{0X3F,1,{0x22}},
{0X40,1,{0x22}},
{0X52,1,{0x10}},
{0X53,1,{0x10}},
  
//**********************************//
//**********************************//  
{0XFF,5,{0xFF,0x98,0x06,0x04,0x07}},  

{0X17,1,{0x22}},                          
   
{0X02,1,{0x77}},
{0XE1,1,{0x79}},
{0X06,1,{0x13}},
{0X26,1,{0xB2}},
//**********************************//
//**********************************//
//**********************************//  
{0XFF,5,{0xFF,0x98,0x06,0x04,0x00}},  
//  {0X36,1,{0xC8}},
//{0X55,1,{0x0B}},

{0x11,1,{0x00}},
 {REGFLAG_DELAY,120,{}},
{0x29,1,{0x00}},//Display ON
{REGFLAG_DELAY,20,{}},
};

#if 0 //Singh
static struct LCM_setting_table lcm_sleep_out_setting[] = {
    // Sleep Out
	{0x11, 0, {0x00}},
    {REGFLAG_DELAY, 120, {}},

    // Display ON
	{0x29, 0, {0x00}},
	{REGFLAG_DELAY, 10, {}},
	
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};
#endif

static struct LCM_setting_table lcm_sleep_in_setting[] = {
	// Display off sequence
	{0x28, 1, {0x00}},
	{REGFLAG_DELAY, 10, {}},
    // Sleep Mode On
	{0x10, 1, {0x00}},
    {REGFLAG_DELAY, 120, {}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};


//static struct LCM_setting_table lcm_backlight_level_setting[] = {
	//{0x51, 1, {0xFF}},
	//{REGFLAG_END_OF_TABLE, 0x00, {}}
//};


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
				
            default:
				dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
       	}
    }
	
}
static void init_lcm_registers(void)
{
    push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);

}

// ---------------------------------------------------------------------------
//  LCM Driver Implementations
// ---------------------------------------------------------------------------

static void lcm_set_util_funcs(const LCM_UTIL_FUNCS * util)
{
    memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
}


static void lcm_get_params(LCM_PARAMS * params)
{
		memset(params, 0, sizeof(LCM_PARAMS));
	
		params->type   = LCM_TYPE_DSI;

	params->width = FRAME_WIDTH;
	params->height = FRAME_HEIGHT;

		// enable tearing-free
		params->dbi.te_mode 				= LCM_DBI_TE_MODE_VSYNC_ONLY;
		params->dbi.te_edge_polarity		= LCM_POLARITY_RISING;
    params->dsi.mode = SYNC_PULSE_VDO_MODE;//BURST_VDO_MODE; SYNC_PULSE_VDO_MODE SYNC_EVENT_VDO_MODE
    params->dsi.esd_check_enable = 1;
    params->dsi.customization_esd_check_enable = 0;
    params->dsi.lcm_esd_check_table[0].cmd = 0x0a;
    params->dsi.lcm_esd_check_table[0].count = 1;
    params->dsi.lcm_esd_check_table[0].para_list[0] = 0x9c;
    // DSI
    /* Command mode setting */
    params->dsi.LANE_NUM = LCM_TWO_LANE;
    //The following defined the fomat for data coming from LCD engine.
		params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
		params->dsi.data_format.trans_seq   = LCM_DSI_TRANS_SEQ_MSB_FIRST;
		params->dsi.data_format.padding     = LCM_DSI_PADDING_ON_LSB;
    params->dsi.data_format.format = LCM_DSI_FORMAT_RGB888;
 
		params->dsi.packet_size=256;

		// Video mode setting		
    params->dsi.intermediat_buffer_num = 0;//because DSI/DPI HW design change, this parameters should be 0 when video mode in MT658X; or memory leakage

	params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
    
    params->dsi.word_count=FRAME_WIDTH*3;	//DSI CMD mode need set these two bellow params, different to 6577
    params->dsi.vertical_active_line=FRAME_HEIGHT;
    params->dsi.vertical_sync_active = 6;
    params->dsi.vertical_backporch = 14;//8;
    params->dsi.vertical_frontporch = 20;//8;
    params->dsi.vertical_active_line = FRAME_HEIGHT;
    params->dsi.horizontal_sync_active = 10;//8;
    params->dsi.horizontal_backporch = 60;//60;
    params->dsi.horizontal_frontporch = 60;//140;	
    params->dsi.horizontal_active_pixel = FRAME_WIDTH;
/*
		params->dsi.vertical_sync_active				= 1;// 3    2
		params->dsi.vertical_backporch					= 1;// 20   1
		params->dsi.vertical_frontporch					= 2; // 1  12
		params->dsi.vertical_active_line				= FRAME_HEIGHT; 

		params->dsi.horizontal_sync_active				= 10;// 50  2
		params->dsi.horizontal_backporch				= 42;
		params->dsi.horizontal_frontporch				= 52;
		params->dsi.horizontal_bllp				= 85;

		params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
		params->dsi.compatibility_for_nvk = 0;*/

    	params->dsi.PLL_CLOCK = 214;//dsi clock customization: should config clock value directly
		params->dsi.ssc_disable = 1;
	}



static void lcm_suspend(void)
{

	push_table(lcm_sleep_in_setting, sizeof(lcm_sleep_in_setting) / sizeof(struct LCM_setting_table), 1);
	SET_RESET_PIN(1);	
	MDELAY(10);	
	SET_RESET_PIN(0);
}



#if (LCM_DSI_CMD_MODE)
static void lcm_update(unsigned int x, unsigned int y,
		       unsigned int width, unsigned int height)
{
	unsigned int x0 = x;
	unsigned int y0 = y;
	unsigned int x1 = x0 + width - 1;
	unsigned int y1 = y0 + height - 1;

	unsigned char x0_MSB = ((x0 >> 8) & 0xFF);
	unsigned char x0_LSB = (x0 & 0xFF);
	unsigned char x1_MSB = ((x1 >> 8) & 0xFF);
	unsigned char x1_LSB = (x1 & 0xFF);
	unsigned char y0_MSB = ((y0 >> 8) & 0xFF);
	unsigned char y0_LSB = (y0 & 0xFF);
	unsigned char y1_MSB = ((y1 >> 8) & 0xFF);
	unsigned char y1_LSB = (y1 & 0xFF);

	unsigned int data_array[16];

	data_array[0] = 0x00053902;
	data_array[1] =
	    (x1_MSB << 24) | (x0_LSB << 16) | (x0_MSB << 8) | 0x2a;
	data_array[2] = (x1_LSB);
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0] = 0x00053902;
	data_array[1] =
	    (y1_MSB << 24) | (y0_LSB << 16) | (y0_MSB << 8) | 0x2b;
	data_array[2] = (y1_LSB);
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0] = 0x002c3909;
	dsi_set_cmdq(data_array, 1, 0);

}
#endif
//extern void DSI_clk_HS_mode(char enter);
extern int IMM_GetOneChannelValue(int dwChannel, int data[4], int* rawdata);

static unsigned int lcm_compare_id(void)
{
	int array[4];
    char buffer[5];
    char id_high=0;
    char id_midd=0;
    char id_low=0;
    int id=0;
    //int lcm_adc = 0, data[4] = {0,0,0,0}, lcm_vol = 0;
    //Do reset here
    //return 1;
    SET_RESET_PIN(1);
    MDELAY(10);       
    SET_RESET_PIN(0);
    MDELAY(25);       
    SET_RESET_PIN(1);
    MDELAY(120);      
   
    array[0]=0x00063902;
    array[1]=0x0698ffff;
    array[2]=0x00000104;
    dsi_set_cmdq(array, 3, 1);
    MDELAY(10);

    array[0]=0x00033700;
    dsi_set_cmdq(array, 1, 1);
    //read_reg_v2(0x04, buffer, 3);//if read 0x04,should get 0x008000,that is both OK.

    read_reg_v2(0x00, buffer,1);
    id_high = buffer[0]; ///////////////////////0x98

    read_reg_v2(0x01, buffer,1);
    id_midd = buffer[0]; ///////////////////////0x06

    read_reg_v2(0x02, buffer,1);
    id_low = buffer[0]; ////////////////////////0x04

    id =(id_high << 16) | (id_midd << 8) | id_low;
#if 0 //def AUXADC_LCM_VOLTAGE_CHANNEL
    IMM_GetOneChannelValue(AUXADC_LCM_VOLTAGE_CHANNEL,data,&lcm_adc); //read lcd _id
    lcm_vol = data[0]*1000+data[1]*10;
#if defined(BUILD_LK)
    printf("ili9806e_hlt_hd720_dsi lk -- ili9806e 0x%x , 0x%x , 0x%x, 0x%x,%d,%d\n", id_high, id_midd, id_low, id,lcm_adc,lcm_vol);
#else
    printk("ili9806e_hlt_hd720_dsi kernel -- ili9806e 0x%x , 0x%x , 0x%x, 0x%x,%d,%d\n", id_high, id_midd, id_low, id,lcm_adc,lcm_vol);
#endif

    if((LCM_ID_ILI9806E == id) && (lcm_vol > MIN_VOLTAGE) && (lcm_vol < MAX_VOLTAGE))
        return 1;
	else
		return 0;
#else
#if defined(BUILD_LK)
    printf("ili9806e_hlt_hd720_dsi lk -- ili9806e 0x%x , 0x%x , 0x%x, 0x%x\n", id_high, id_midd, id_low, id);
#else
    printk("ili9806e_hlt_hd720_dsi kernel -- ili9806e 0x%x , 0x%x , 0x%x, 0x%x\n", id_high, id_midd, id_low, id);
#endif

    if((LCM_ID_ILI9806E == id))
        return 1;
	else
		return 0;
#endif
}


static unsigned int rgk_lcm_compare_id(void)    //coco add 
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
		printf("[adc_uboot]: get data error\n");
	#endif
		return 0;
			   
	}
#endif
	
	lcm_vol = data[0]*1000+data[1]*10;
	printk(" ===========44 lcm_vol=%d\n",lcm_vol);
#ifdef BUILD_LK
	printf("[adc_uboot]: lcm_vol= %d\n",lcm_vol);
#else
	printk("[adc_kernel]: lcm_vol= %d\n",lcm_vol);
#endif
	if (lcm_vol>=MIN_VOLTAGE &&lcm_vol <= MAX_VOLTAGE && lcm_compare_id())
	{
	    printk(" ===========55\n");
		return 1;
	}
		printk(" ===========66\n");
		return 0;
}



static void lcm_init(void)
{
	SET_RESET_PIN(1);
	MDELAY(10);
	SET_RESET_PIN(0);
	MDELAY(50);
	SET_RESET_PIN(1);
	MDELAY(120);
	init_lcm_registers();
}
static void lcm_resume(void)
{
	lcm_init();
	
//	push_table(lcm_sleep_out_setting, sizeof(lcm_sleep_out_setting) / sizeof(struct LCM_setting_table), 1);
}

//  Get LCM Driver Hooks
// ---------------------------------------------------------------------------
static unsigned int lcm_ata_check(unsigned char *buffer)
{
	printk(" ===========ili9806e_hlt_hd720_dsi lcm_ata_check\n"); //coco add 
    //return lcm_compare_id();
	return 1;  //coco add 
}
LCM_DRIVER ili9806e_dsi_vdo_fwvga_ivo_ykl_d5278_drv = {
	.name = "ili9806e_dsi_vdo_fwvga_ivo_ykl_d5278",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params = lcm_get_params,
	.init = lcm_init,
	.suspend = lcm_suspend,
	.resume = lcm_resume,
	.compare_id = rgk_lcm_compare_id,
	.ata_check	= lcm_ata_check,
#if (LCM_DSI_CMD_MODE)
	.set_backlight	= lcm_setbacklight,
    .update         = lcm_update,
#endif
};

