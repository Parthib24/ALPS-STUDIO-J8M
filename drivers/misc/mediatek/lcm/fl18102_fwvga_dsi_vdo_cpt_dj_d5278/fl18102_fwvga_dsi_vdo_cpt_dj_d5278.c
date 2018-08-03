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

#define MIN_VOLTAGE (500)
#define MAX_VOLTAGE (700)
// ---------------------------------------------------------------------------
//  Local Constantsq
// ---------------------------------------------------------------------------

#define FRAME_WIDTH  (480)
#define FRAME_HEIGHT (854)

#define REGFLAG_DELAY             	0XFFE
#define REGFLAG_END_OF_TABLE      	0xFFF   // END OF REGISTERS MARKER
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
#if 0
{0xB9,0x03,{0xF1,0x08,0x01}},
{0xB2,0x01,{0x22}},   
{0xB3,0x08,{0x00,0x00,0x06,0x06,0x20,0x20,0x30,0x30}},   
{0xBA,0x11,{0x31,0x00,0x44,0x25,0x91,0x0A,0x00,0x00,0xC1,0x00,0x00,0x00,0x0D,0x02,0x4F,0xB9,0xEE}},   
{0xE3,0x05,{0x04,0x04,0x01,0x01,0x00}},    
{0xB4,0x01,{0x00}},
{0xB5,0x02,{0x05,0x05}}, 
{0xB6,0x02,{0x57,0x43}},   
{0xB8,0x02,{0x64,0x20}},    
{0xCC,0x01,{0x00}},    
{0xBC,0x01,{0x47}},   
{0xE9,0x33,{0x00,0x00,0x07,0x00,0x00,0x81,0x89,0x12,0x31,0x23,0x23,0x07,0x81,0x80,0x23,0x00,0x00,
		    0x10,0x00,0x00,0x00,0x0F,0x89,0x13,0x18,0x88,0x88,0x88,0x88,0x88,0x88,0x89,0x02,0x08,
		    0x88,0x88,0x88,0x88,0x88,0x88,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}},
{0xEA,0x16,{0x90,0x00,0x00,0x00,0x88,0x02,0x09,0x88,0x88,0x88,0x88,0x88,0x88,0x88,0x13,0x19,0x88,
		    0x88,0x88,0x88,0x88,0x88}}, 
{0xE0,0x22,{0x00,0x06,0x0E,0x1B,0x1D,0x29,0x33,0x40,0x08,0x11,0x12,0x16,0x18,0x16,0x17,0x12,0x17,
		    0x00,0x06,0x0E,0x1B,0x1D,0x29,0x33,0x40,0x08,0x11,0x12,0x16,0x18,0x16,0x17,0x12,0x17}},
{0x11,1,{0x00}},  
{REGFLAG_DELAY, 120, {0}},	
{0x29,1,{0x00}},  
{REGFLAG_DELAY, 10, {0}},
#else
{0xB9, 0x03, { 0xF1, 0x08, 0x01, 	}},
{0xB1, 0x07, { 0x26, 0x18, 0x18, 0x87, 0x30, 0x01, 0xAB, 	}},
{0xB2, 0x01, { 0x02, 	}},
{0xB3, 0x08, { 0x00, 0x00, 0x06, 0x06, 0x20, 0x20, 0x30, 0x30, 	}},
{0xBA, 0x11, { 0x31, 0x00, 0x44, 0x25, 0x91, 0x0A, 0x00, 0x00, 0xC1, 0x00, 0x00, 0x00, 0x0D, 0x02, 0x4F, 0xB9, 0xEE, 	}},
{0xE3, 0x05, { 0x04, 0x04, 0x01, 0x01, 0x00,   	}},
{0xB4, 0x01, { 0x00, 	}},
{0xB5, 0x02, { 0x05, 0x05, 		}},
{0xB6, 0x02, { 0x6C, 0x54, 		}},
{0xB8, 0x02, { 0x64, 0x22, 	}},
{0xCC, 0x01, { 0x00, 	}},
{0xBC, 0x01, { 0x47, 	}},
{0xE9, 0x33, { 0x00, 0x00, 0x07, 0x00, 0x00, 0x81, 0x89, 0x12, 0x31, 0x23, 0x23, 0x07, 0x81, 0x80, 0x23, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x0F, 0x89, 0x13, 0x18, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x89, 0x02, 0x08, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, }},
{0xEA, 0x16, { 0x90, 0x00, 0x00, 0x00, 0x88, 0x02, 0x09, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x13, 0x19, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, }},
{0xE0, 0x22, { 0x00, 0x00, 0x00, 0x19, 0x19, 0x3F, 0x27, 0x3C, 0x00, 0x0D, 0x14, 0x18, 0x19, 0x16, 0x16, 0x12, 0x16, 0x00, 0x00, 0x00, 0x19, 0x19, 0x3F, 0x27, 0x3C, 0x00, 0x0D, 0x14, 0x18, 0x19, 0x16, 0x16, 0x12, 0x16, }},
{0x11,1,{0x00}},  
{REGFLAG_DELAY, 120, {0}},	
{0x29,1,{0x00}},  
{REGFLAG_DELAY, 10, {0}},
#endif
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
    params->dbi.te_mode 			= LCM_DBI_TE_MODE_VSYNC_ONLY;
    params->dbi.te_edge_polarity		= LCM_POLARITY_RISING;
    params->dsi.mode = SYNC_PULSE_VDO_MODE;//BURST_VDO_MODE; SYNC_PULSE_VDO_MODE SYNC_EVENT_VDO_MODE
    params->dsi.esd_check_enable = 1;
    params->dsi.customization_esd_check_enable = 0;
    params->dsi.lcm_esd_check_table[0].cmd = 0x0a;
    params->dsi.lcm_esd_check_table[0].count = 1;
    params->dsi.lcm_esd_check_table[0].para_list[0] = 0x9c;
	
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
    params->dsi.vertical_sync_active =6;// 4;
    params->dsi.vertical_backporch = 7;//10;
    params->dsi.vertical_frontporch = 20;
    params->dsi.vertical_active_line = FRAME_HEIGHT;
    params->dsi.horizontal_sync_active = 10;
    params->dsi.horizontal_backporch = 80;//20;
    params->dsi.horizontal_frontporch = 80;//30;
    params->dsi.horizontal_active_pixel = FRAME_WIDTH;
    params->dsi.PLL_CLOCK = 210;//dsi clock customization: should config clock value directly
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

static struct LCM_setting_table lcm_readid_setting[] = {
	{0xB9,0x03,{0xF1,0x08,0x01}},
	{0xBA,0x11,{0x31,0x00,0x44,0x25,0x91,0x0A,0x00,0x00,0xC1,0x00,0x00,0x00,0x0D,0x02,0x4F,0xB9,0xEE}},   
};
static void init_readid_registers(void)
{
    push_table(lcm_readid_setting, sizeof(lcm_readid_setting) / sizeof(struct LCM_setting_table), 1);
}
static unsigned int lcm_compare_id(void)
{
#if 0 //def AUXADC_LCM_VOLTAGE_CHANNEL
    int data[4] = {0,0,0,0};
    int res = 0;
    int rawdata = 0;
    int lcm_vol = 0;
#endif
    char buffer[5];
    SET_RESET_PIN(1);
    MDELAY(10);
    SET_RESET_PIN(0);
    MDELAY(25);       
    SET_RESET_PIN(1);
    MDELAY(120);
    init_readid_registers();
	read_reg_v2(0xd0, buffer,1);
#if 0 //def AUXADC_LCM_VOLTAGE_CHANNEL
    res = IMM_GetOneChannelValue(AUXADC_LCM_VOLTAGE_CHANNEL,data,&rawdata);
    if(res < 0)
    { 
    	#ifdef BUILD_LK
    	printf("[adc_uboot]: get data error\n");
    	#endif
    	return 0;		   
    }
    lcm_vol = data[0]*1000+data[1]*10;
    #ifdef BUILD_LK
	printf("[adc_uboot]: lcm_vol= %d,buffer[0]=%d\n",lcm_vol,buffer[0]);
    #else
	printk("[adc_kernel]: lcm_vol= %d,buffer[0]=%d\n",lcm_vol,buffer[0]);
    #endif
    if (lcm_vol>=MIN_VOLTAGE &&lcm_vol <= MAX_VOLTAGE && buffer[0]== 0x3 /*&& lcm_compare_id()*/)
    {
		return 1;
    }

    return 0;
#else
	if (buffer[0]== 0x3){
		#if defined(BUILD_LK)
			  printf(" %s  OK!  id 0x%x", __func__,buffer[0]);
		#else
			  printk(" %s  OK!  id 0x%x", __func__,buffer[0]);
		#endif
		return 1;
	}else{
		#if defined(BUILD_LK)
			  printf(" %s  Fail!  id 0x%x", __func__,buffer[0]);
		#else
			  printk(" %s  Fail!  id 0x%x", __func__,buffer[0]);
		#endif
		return 0;
	}	
#endif
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
    if (lcm_vol>=MIN_VOLTAGE &&lcm_vol <= MAX_VOLTAGE /*&& lcm_compare_id()*/)
    {
	    printk(" ===========55\n");
		return 1;
    }
    printk(" ===========66\n");
    return 0;

}




//  Get LCM Driver Hooks
// ---------------------------------------------------------------------------
static unsigned int lcm_ata_check(unsigned char *buffer)
{
    printk(" ===========fl18102_fwvga_dsi_vdo_dj 11111\n");
	return rgk_lcm_compare_id();
    //return lcm_compare_id();
}
LCM_DRIVER fl18102_fwvga_dsi_vdo_cpt_dj_d5278_drv = {
	.name = "fl18102_fwvga_dsi_vdo_cpt_dj_d5278",
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

