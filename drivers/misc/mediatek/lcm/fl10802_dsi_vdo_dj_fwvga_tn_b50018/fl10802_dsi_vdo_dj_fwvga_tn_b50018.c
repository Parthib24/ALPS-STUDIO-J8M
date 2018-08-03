
/*****************************************************************************
*  Copyright Statement:
*  --------------------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of MediaTek Inc. (C) 2008
*
*  BY OPENING THIS FILE, BUYER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
*  THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
*  RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO BUYER ON
*  AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
*  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
*  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
*  NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
*  SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
*  SUPPLIED WITH THE MEDIATEK SOFTWARE, AND BUYER AGREES TO LOOK ONLY TO SUCH
*  THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. MEDIATEK SHALL ALSO
*  NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE RELEASES MADE TO BUYER'S
*  SPECIFICATION OR TO CONFORM TO A PARTICULAR STANDARD OR OPEN FORUM.
*
*  BUYER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND CUMULATIVE
*  LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
*  AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
*  OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY BUYER TO
*  MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
*
*  THE TRANSACTION CONTEMPLATED HEREUNDER SHALL BE CONSTRUED IN ACCORDANCE
*  WITH THE LAWS OF THE STATE OF CALIFORNIA, USA, EXCLUDING ITS CONFLICT OF
*  LAWS PRINCIPLES.  ANY DISPUTES, CONTROVERSIES OR CLAIMS ARISING THEREOF AND
*  RELATED THERETO SHALL BE SETTLED BY ARBITRATION IN SAN FRANCISCO, CA, UNDER
*  THE RULES OF THE INTERNATIONAL CHAMBER OF COMMERCE (ICC).
*
*****************************************************************************/
//
#if 0
#ifdef BUILD_LK
#include <platform/mt_gpio.h>
#else
#include <linux/string.h>
#if defined(BUILD_UBOOT)
#include <asm/arch/mt_gpio.h>
#else
//#include <mach/mt_gpio.h>
#endif
#endif
#include "lcm_drv.h"

//#include <cust_adc.h>   
#endif 
////////////////////////////////

#ifdef BUILD_LK
#else
#include <linux/string.h>
#endif

#include "lcm_drv.h"
#ifdef BUILD_LK
    #include <platform/mt_gpio.h>
#elif defined(BUILD_UBOOT)
    #include <asm/arch/mt_gpio.h>
#else
  //  #include <mach/mt_gpio.h>
#endif


 	
#define MIN_VOLTAGE (400)     
#define MAX_VOLTAGE (600)     
#define LCM_COMPARE_BY_ADC (0) 

#define FRAME_WIDTH  		(480)
#define FRAME_HEIGHT 		(854)
#define REGFLAG_DELAY       0XFE
#define REGFLAG_END_OF_TABLE      	0xFFF   
#define LCM_ID  3
#define LCM_DSI_CMD_MODE	0

#ifndef TRUE
    #define   TRUE     1
#endif
#ifndef FALSE
    #define   FALSE    0
#endif

static LCM_UTIL_FUNCS lcm_util = {0};
#define SET_RESET_PIN(v)    								(lcm_util.set_reset_pin((v)))
#define UDELAY(n) 											(lcm_util.udelay(n))
#define MDELAY(n) 											(lcm_util.mdelay(n))
#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)	        lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)		        lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)										lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)					lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg											lcm_util.dsi_read_reg()
#define read_reg_v2(cmd, buffer, buffer_size)				        lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)

extern int IMM_GetOneChannelValue(int dwChannel, int data[4], int* rawdata);

 struct LCM_setting_table {
    unsigned cmd;
    unsigned char count;
    unsigned char para_list[64];
};
#if 0
static void ILI9806C_set_reset_pin(int high){
	mt_set_gpio_mode(GPIO_DISP_LRSTB_PIN, GPIO_MODE_GPIO);
	if(1 == high)
		mt_set_gpio_out(GPIO_DISP_LRSTB_PIN, GPIO_OUT_ONE);
	else
		mt_set_gpio_out(GPIO_DISP_LRSTB_PIN, GPIO_OUT_ZERO);
}
#define SET_RESET_PIN(v)    (ILI9806C_set_reset_pin(v))
#endif


#ifndef BUILD_LK
extern atomic_t ESDCheck_byCPU;
#endif

static struct LCM_setting_table lcm_initialization_setting[] = {
	//****************************************************************************//
	//****************************** Page 1 Command ******************************//
	//****************************************************************************//
	
	{0xB9,0x03,{0xF1,0x08,0x01}},
	{0xB1,0x07,{0x2A,0x1A,0x1A,0x87,0x30,0x03,0xAB}},	
	{0xB8,0x02,{0x64,0x20}},	
	{0xB6,0x02,{0x32,0x32}},	
	{0xB5,0x02,{0x07,0x07}},	
	{0xE3,0x05,{0x09,0x09,0x03,0x03,0x00}},  //	
	{0xCC,0x01,{0x00}},  // 00:column,02:2-dot	
	{0xB2,0x01,{0x22}},  //4.5		V1.0_151026 	//-4.5		V1.0_151026
	{0xB4,0x01,{0x00}},	// Set VCOM	//VCOM OFFSET   38	V1.0_151026 //VCOM OFFSET		V1.0_151026
	{0xB3,0x08,{0x01,0x00,0x06,0x06,0x18,0x13,0x39,0x35}},	
	{0xBA,0x11,{0x31,0x00,0x44,0x25,0xb1,0x0A,0x00,0x00,0xC1,0x00,0x00,0x00,0x0D,0x02,0x4F,0xB9,0xEE}},	
	{0xBC,0x01,{0x44}},  //02	
	{0xC7,0x01,{0xA0}},  //02		
	{0xC8,0x02,{0x10,0x00}},
	
	{0xE9,0x33,{0x00,0x00,0x09,0x03,0x65,0x0A,0x8A,0x10,0x01,0x00,0x37,0x0D,0x0A,0x8A,
	0x37,0x00,0x00,0x18,0x00,0x00,0x00,0x25,0x09,0x80,0X40,0x00,0X42,0x60,0x00,0x00,0x00,
	0x09,0x81,0x50,0x01,0x53,0x70,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}},	//51  CKS   	//GIP                     
			
	{0xEA,0x12,{0x94,0x00,0x00,0x00,0x08,0x95,0x10,0x07,0x35,0x10,0x00,0x00,0x00,0x08,0x94,0x00,0x06,0x24}},   //22  //COSR17,COS18                	
			
	{0xE0,0x22,{0x00,0x00,0x11,0x0F,0x10,0x2A,0x26,0x34,0x0C,0x0F,0x10,0x14,0x17,0x14,0x16,0x13,
	0x17,0x00,0x00,0x11,0x0F,0x10,0x2A,0x26,0x34,0x0C,0x0F,0x10,0x14,0x17,0x14,0x16,0x13,0x17}},   //17 //Gamma	    
	
	{0x11,0,{}},	
	{REGFLAG_DELAY,120,{}},
							
	{0x29,0,{}},		
	{REGFLAG_DELAY,10,{}},		
	
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};

static struct LCM_setting_table lcm_deep_sleep_mode_in_setting[] = {
	{0xE3,5,{0x09,0x09,0x03,0x03,0xc0}},
	{REGFLAG_DELAY, 10, {}},
	{0x28, 1, {0x00}},
	{REGFLAG_DELAY, 10, {}},
	{0x10, 1, {0x00}},
	{REGFLAG_DELAY, 120, {}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};

static struct LCM_setting_table lcm_sleep_out_setting[] = {
	/*{0xB9,0x03,{0xF1,0x08,0x01}},
	{0xE3,0x05,{0x09,0x09,0x03,0x03,0x00}},
	{0xB1,0x07,{0x2A,0x1A,0x1A,0x87,0x30,0x01,0xAB}},	*/
	{0xBA,0x11,{0x31,0x00,0x44,0x25,0x91,0x0A,0x00,0x00,0xC1,0x00,0x00,0x00,0x0D,0x02,0x4F,0xB9,0xEE}},	
	{REGFLAG_DELAY, 10, {}},
	{0x11, 1, {0x00}},
	{REGFLAG_DELAY, 120, {}},
	{0x29, 1, {0x00}},
	{REGFLAG_DELAY, 20, {}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};

//static int vcom = 0x20;
static void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
     unsigned int i;
    for(i = 0; i < count; i++) {
        unsigned cmd;
        cmd = table[i].cmd;
        switch (cmd) {
      /*  	case 0xb6:
        		table[i].para_list[0]=vcom;
        		table[i].para_list[1]=vcom;
        		dsi_set_cmdq_V2(cmd,table[i].count,table[i].para_list,force_update);
        		vcom+= 1;
        		break;
      */  		
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

		// enable tearing-free
		params->dbi.te_mode 		        = LCM_DBI_TE_MODE_DISABLED;//LCM_DBI_TE_MODE_VSYNC_ONLY;//LCM_DBI_TE_MODE_VSYNC_ONLY;LCM_DBI_TE_MODE_DISABLED
		params->dbi.te_edge_polarity		= LCM_POLARITY_RISING;

		params->dsi.mode   = SYNC_PULSE_VDO_MODE;
	
		// DSI
		/* Command mode setting */
		params->dsi.LANE_NUM				= LCM_TWO_LANE;
		//The following defined the fomat for data coming from LCD engine.
		params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
		params->dsi.data_format.trans_seq   = LCM_DSI_TRANS_SEQ_MSB_FIRST;
		params->dsi.data_format.padding     = LCM_DSI_PADDING_ON_LSB;
		params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

		// Highly depends on LCD driver capability.
		// Not support in MT6573
	   	params->dsi.packet_size=256;

		// Video mode setting		
		params->dsi.intermediat_buffer_num = 2;//because DSI/DPI HW design change, this parameters should be 0 when video mode in MT658X; or memory leakage

		params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
		params->dsi.word_count=480*3;	



		params->dsi.vertical_sync_active = 4;//--4
		params->dsi.vertical_backporch = 12;//--6
		params->dsi.vertical_frontporch = 10;//--6
		params->dsi.vertical_active_line = FRAME_HEIGHT; 


		 // 20120618 modify. for ID 01,8b,80,09
		 params->dsi.horizontal_sync_active = 30;//---6
		 params->dsi.horizontal_backporch =100;//33;//--30
		 params->dsi.horizontal_frontporch =100;//33;//--30
		 
		 params->dsi.horizontal_active_pixel = FRAME_WIDTH;
//ugrec_tky
		// Non-continuous clock 
		params->dsi.noncont_clock = 1; 
		params->dsi.noncont_clock_period = 1; 
		// Unit : frames
//ugrec_tky
		// Bit rate calculation
		//1 Every lane speed
		params->dsi.PLL_CLOCK = 229;//240
		params->dsi.ssc_disable  = 1;
		params->physical_width = 62;
		params->physical_height = 110;
	//	params->dsi.pll_div1=1;		// div1=0,1,2,3;div1_real=1,2,4,4 ----0: 546Mbps  1:273Mbpsfvco=fref*(fbk_div+1)*2/(div1_real*div2_real)
		//params->dsi.pll_div2=0;		// div2=0,1,2,3;div1_real=1,2,4,4	
	//	params->dsi.fbk_div =15;    //20 fref=26MHz, fvco=fref*(fbk_div+1)*2/(div1_real*div2_real)	
		params->dsi.esd_check_enable 										= 1;	
    params->dsi.noncont_clock  											= 1;
    params->dsi.noncont_clock_period                = 1;
    params->dsi.customization_esd_check_enable 			= 1;
    params->dsi.lcm_esd_check_table[0].cmd 					= 0x09;
    params->dsi.lcm_esd_check_table[0].count 				= 1;
    params->dsi.lcm_esd_check_table[0].para_list[0] = 0x80;

    
    params->dsi.lcm_esd_check_table[1].cmd 					= 0xba;
    params->dsi.lcm_esd_check_table[1].count 				= 10;
    params->dsi.lcm_esd_check_table[1].para_list[0] = 0x31;
    params->dsi.lcm_esd_check_table[1].para_list[1] = 0x00;
		params->dsi.lcm_esd_check_table[1].para_list[2] = 0x44;
		params->dsi.lcm_esd_check_table[1].para_list[3] = 0x25;
		params->dsi.lcm_esd_check_table[1].para_list[7] = 0x00;
		params->dsi.lcm_esd_check_table[1].para_list[8] = 0xc1;
		params->dsi.lcm_esd_check_table[1].para_list[9] = 0x00;
		
		params->dsi.lcm_esd_check_table[2].cmd 					= 0x0a;
    params->dsi.lcm_esd_check_table[2].count 				= 1;
    params->dsi.lcm_esd_check_table[2].para_list[0] = 0x9c;


}

static void lcm_init(void)
{
    SET_RESET_PIN(1);
     MDELAY(10);
    SET_RESET_PIN(0);
    MDELAY(10);
    SET_RESET_PIN(1);
    MDELAY(120);
	push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);

}

static void lcm_suspend(void)
{
	push_table(lcm_deep_sleep_mode_in_setting, sizeof(lcm_deep_sleep_mode_in_setting) / sizeof(struct LCM_setting_table), 1);
	
	  SET_RESET_PIN(1);
    MDELAY(10);
    SET_RESET_PIN(0);
    MDELAY(20);
    SET_RESET_PIN(1);
    MDELAY(120);
}



static void lcm_resume(void)
{

	lcm_init();
	
	//push_table(lcm_sleep_out_setting, sizeof(lcm_sleep_out_setting) / sizeof(struct LCM_setting_table), 1);
}


static struct LCM_setting_table lcm_compare_id_setting[] = {
	{0xFF,	5,      {0xFF,0x98,0x06,0x04,0x01}}, 
	{REGFLAG_DELAY, 10, {}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};

static unsigned int lcm_compare_id(void)
{
	unsigned int id = 0, id2 = 0;
 unsigned char buffer[4];
 unsigned int data_array[16];
  
 SET_RESET_PIN(1); //TE:should reset LCM firstly
 MDELAY(10);
 SET_RESET_PIN(0);
 MDELAY(10);
 SET_RESET_PIN(1);
 MDELAY(120);

     data_array[0]=0x00043902;//Enable external Command
 data_array[1]=0x0108F1B9; 
 dsi_set_cmdq(data_array, 2, 1);
 MDELAY(10);
 
  read_reg_v2(0xD0, buffer, 1);
 id = buffer[0];

#if defined(BUILD_LK)
 printf("fl10802 id = %x\n", buffer[0]); 
#else
 printk("fl10802 id = %x\n", buffer[0]); 
#endif
 #if LCM_COMPARE_BY_ADC
#ifdef AUXADC_LCM_VOLTAGE_CHANNEL
	    int data[4] = {0,0,0,0};
	    int res = 0;
	    int rawdata = 0;
	    int lcm_vol = 0;
            if(LCM_ID == id){
		    res = IMM_GetOneChannelValue(AUXADC_LCM_VOLTAGE_CHANNEL,data,&rawdata);
		    if(res < 0){ 
			#ifdef BUILD_LK
			        printf("luke: fl10802a  %d  ADC get LCM_ID voltage error! \n",__LINE__);
			#endif
			return 0;
		    }
		    lcm_vol = data[0]*1000+data[1]*10;
		    #ifdef BUILD_LK
		    		printf("luke: fl10802a  %d  ADC get LCM_ID :%d mv \n",__LINE__,lcm_vol);
		    #endif
		    if (lcm_vol>=MIN_VOLTAGE &&lcm_vol <= MAX_VOLTAGE){
			     return 1;
		    }else{
		   	     return 0;
		    }
          }else{
          	return 0;
          }
#endif
#else
//=============
       if(LCM_ID == id){
	            #ifdef BUILD_LK
	            printf("luke: ili9806e probe OK! %d  id:%x\n",__LINE__,id);
                    #endif
                   return 1;
       	}else{
       		    #ifdef BUILD_LK
	            printf("luke: ili9806e probe error! %d  id:%x\n",__LINE__,id);
                    #endif
                   return 0;
	}
#endif    

}

static unsigned int lcm_ata_check(unsigned char *buffer){
	
		#ifndef BUILD_LK
			int array[4];
			char buf[3];
			char id_high=0;
			char id_low=0;
			int id=0;
		
			//{0x39, 0xFF, 5, { 0xFF,0x98,0x06,0x04,0x01}}, // Change to Page 1 CMD
			array[0] = 0x00043902;    //////////
			array[1] = 0x000108F1;    //////////
			dsi_set_cmdq(array, 2, 1);
		
			array[0] = 0x00013700;
			dsi_set_cmdq(array, 1, 1);
			atomic_set(&ESDCheck_byCPU,1);
			read_reg_v2(0xDA, &buf[0], 1);  //0x81
			atomic_set(&ESDCheck_byCPU,0);
			id = buf[0];
			return (0x10 == id)?1:0;
#else
	return 1;
#endif
}


LCM_DRIVER fl10802_dsi_vdo_dj_fwvga_tn_b50018_lcm_drv = 
{
  .name = "fl10802_dsi_vdo_dj_fwvga_tn_b50018",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params     = lcm_get_params,
	.init           = lcm_init,
	.suspend        = lcm_suspend,
	.resume         = lcm_resume,
	.compare_id    = lcm_compare_id,	
	.ata_check     = lcm_ata_check 
#if (LCM_DSI_CMD_MODE)
	   //.set_backlight	= lcm_setbacklight,
        //.update         = lcm_update,
#endif
};

