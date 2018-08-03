#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/kobject.h>
//#include <linux/earlysuspend.h>
#include <linux/platform_device.h>
#include <asm/atomic.h>
#include <linux/module.h>
#include <linux/gpio.h>
//#include <cust_acc.h>
//#include <linux/hwmsensor.h>
//#include <linux/hwmsen_dev.h>
//#include <linux/sensors_io.h>
//#include <linux/hwmsen_helper.h>
//#include <linux/xlog.h>


#include <mt-plat/mt_typedefs.h>
#include <mt-plat/mt_gpio.h>
//#include <mt-plat/mt_pm_ldo.h>

#include "psc5415a.h"
#include "cust_charging.h"
#include <mt-plat/charging.h>
/*#include "psc5415a.h"
#include <asm/uaccess.h>
#include <asm/atomic.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/init.h>
#include <linux/kobject.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#endif
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/workqueue.h>

#include <mt-plat/charging.h>
#include <mt-plat/mt_gpio.h>*/

#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#endif


/**********************************************************
  *
  *   [I2C Slave Setting] 
  *
  *********************************************************/
#define psc5415a_SLAVE_ADDR_WRITE   0xD4
#define psc5415a_SLAVE_ADDR_Read    0xD5

static struct i2c_client *new_client = NULL;
static const struct i2c_device_id psc5415a_i2c_id[] = {{"psc5415a",0},{}};   
kal_bool chargin_hw_init_done = KAL_FALSE; 
static int psc5415a_driver_probe(struct i2c_client *client, const struct i2c_device_id *id);
int start_flag = 0;

#ifdef CONFIG_OF
static const struct of_device_id psc5415a_of_match[] = {
	{.compatible = "mediatek,SWITHING_CHARGER",},
	{},
};

MODULE_DEVICE_TABLE(of, psc5415a_of_match);
#endif
static struct i2c_driver psc5415a_driver = {
    .driver = {
        .name    = "psc5415a",
#ifdef CONFIG_OF
		   .of_match_table = psc5415a_of_match,
#endif
    },
    .probe       = psc5415a_driver_probe,
    .id_table    = psc5415a_i2c_id,
};

/**********************************************************
  *
  *   [Global Variable] 
  *
  *********************************************************/
u8 psc5415a_reg[psc5415a_REG_NUM] = {0};

static DEFINE_MUTEX(psc5415a_i2c_access);
/**********************************************************
  *
  *   [I2C Function For Read/Write psc5415a] 
  *
  *********************************************************/
int psc5415a_read_byte(u8 cmd, u8 *returnData)
{
    char     cmd_buf[1]={0x00};
    char     readData = 0;
    int      ret=0;

    mutex_lock(&psc5415a_i2c_access);
    
    //new_client->addr = ((new_client->addr) & I2C_MASK_FLAG) | I2C_WR_FLAG;    
    new_client->ext_flag=((new_client->ext_flag ) & I2C_MASK_FLAG ) | I2C_WR_FLAG | I2C_DIRECTION_FLAG;

    cmd_buf[0] = cmd;
    ret = i2c_master_send(new_client, &cmd_buf[0], (1<<8 | 1));
    if (ret < 0) 
    {    
        //new_client->addr = new_client->addr & I2C_MASK_FLAG;
        new_client->ext_flag=0;

        mutex_unlock(&psc5415a_i2c_access);
        return 0;
    }
    
    readData = cmd_buf[0];
    *returnData = readData;

    // new_client->addr = new_client->addr & I2C_MASK_FLAG;
    new_client->ext_flag=0;
    
    mutex_unlock(&psc5415a_i2c_access);    
    return 1;
}

int psc5415a_write_byte(u8 cmd, u8 writeData)
{
    char    write_data[2] = {0};
    int     ret=0;
    
    mutex_lock(&psc5415a_i2c_access);
    
    write_data[0] = cmd;
    write_data[1] = writeData;
    
    new_client->ext_flag=((new_client->ext_flag ) & I2C_MASK_FLAG ) | I2C_DIRECTION_FLAG;
    
    ret = i2c_master_send(new_client, write_data, 2);
    if (ret < 0) 
    {
       
        new_client->ext_flag=0;
        mutex_unlock(&psc5415a_i2c_access);
        return 0;
    }
    
    new_client->ext_flag=0;
    mutex_unlock(&psc5415a_i2c_access);
    return 1;
}

/**********************************************************
  *
  *   [Read / Write Function] 
  *
  *********************************************************/
u32 psc5415a_read_interface (u8 RegNum, u8 *val, u8 MASK, u8 SHIFT)
{
    u8 psc5415a_reg = 0;
    int ret = 0;

   battery_xlog_printk(BAT_LOG_FULL,"--------------------------------------------------\n");

    ret = psc5415a_read_byte(RegNum, &psc5415a_reg);

	battery_xlog_printk(BAT_LOG_FULL,"[psc5415a_read_interface] Reg[%x]=0x%x\n", RegNum, psc5415a_reg);
	
    psc5415a_reg &= (MASK << SHIFT);
    *val = (psc5415a_reg >> SHIFT);
	
	battery_xlog_printk(BAT_LOG_FULL,"[psc5415a_read_interface] val=0x%x\n", *val);
	
    return ret;
}

u32 psc5415a_config_interface (u8 RegNum, u8 val, u8 MASK, u8 SHIFT)
{
    u8 psc5415a_reg = 0;
    int ret = 0;

    battery_xlog_printk(BAT_LOG_FULL,"--------------------------------------------------\n");

    ret = psc5415a_read_byte(RegNum, &psc5415a_reg);
    battery_xlog_printk(BAT_LOG_FULL,"[psc5415a_config_interface] Reg[%x]=0x%x\n", RegNum, psc5415a_reg);
    
    psc5415a_reg &= ~(MASK << SHIFT);
    psc5415a_reg |= (val << SHIFT);

	if(RegNum == psc5415a_CON4 && val == 1 && MASK ==CON4_RESET_MASK && SHIFT == CON4_RESET_SHIFT)
	{
		// RESET bit
	}
	else if(RegNum == psc5415a_CON4)
	{
		psc5415a_reg &= ~0x80;	//RESET bit read returs 1, so clear it
	}
	 

    ret = psc5415a_write_byte(RegNum, psc5415a_reg);
    battery_xlog_printk(BAT_LOG_FULL,"[psc5415a_config_interface] write Reg[%x]=0x%x\n", RegNum, psc5415a_reg);

    // Check
    //psc5415a_read_byte(RegNum, &psc5415a_reg);
    //printk("[psc5415a_config_interface] Check Reg[%x]=0x%x\n", RegNum, psc5415a_reg);

    return ret;
}

//write one register directly
u32 psc5415a_config_interface_liao (u8 RegNum, u8 val)
{   
    int ret = 0;
    
    ret = psc5415a_write_byte(RegNum, val);

    return ret;
}

/**********************************************************
  *
  *   [Internal Function] 
  *
  *********************************************************/
//CON0----------------------------------------------------

void psc5415a_set_tmr_rst(u32 val)
{
    u32 ret=0;    

    ret=psc5415a_config_interface(   (u8)(psc5415a_CON0), 
                                    (u8)(val),
                                    (u8)(CON0_TMR_RST_MASK),
                                    (u8)(CON0_TMR_RST_SHIFT)
                                    );
}

u32 psc5415a_get_otg_status(void)
{
    u32 ret=0;
    u8 val=0;

    ret=psc5415a_read_interface(     (u8)(psc5415a_CON0), 
                                    (&val),
                                    (u8)(CON0_OTG_MASK),
                                    (u8)(CON0_OTG_SHIFT)
                                    );
    return val;
}

void psc5415a_set_en_stat(u32 val)
{
    u32 ret=0;    

    ret=psc5415a_config_interface(   (u8)(psc5415a_CON0), 
                                    (u8)(val),
                                    (u8)(CON0_EN_STAT_MASK),
                                    (u8)(CON0_EN_STAT_SHIFT)
                                    );
}

u32 psc5415a_get_chip_status(void)
{
    u32 ret=0;
    u8 val=0;

    ret=psc5415a_read_interface(     (u8)(psc5415a_CON0), 
                                    (&val),
                                    (u8)(CON0_STAT_MASK),
                                    (u8)(CON0_STAT_SHIFT)
                                    );
    return val;
}

u32 psc5415a_get_boost_status(void)
{
    u32 ret=0;
    u8 val=0;

    ret=psc5415a_read_interface(     (u8)(psc5415a_CON0), 
                                    (&val),
                                    (u8)(CON0_BOOST_MASK),
                                    (u8)(CON0_BOOST_SHIFT)
                                    );
    return val;
}

u32 psc5415a_get_fault_status(void)
{
    u32 ret=0;
    u8 val=0;

    ret=psc5415a_read_interface(     (u8)(psc5415a_CON0), 
                                    (&val),
                                    (u8)(CON0_FAULT_MASK),
                                    (u8)(CON0_FAULT_SHIFT)
                                    );
    return val;
}

//CON1----------------------------------------------------

void psc5415a_set_input_charging_current(u32 val)
{
    u32 ret=0;    

    ret=psc5415a_config_interface(   (u8)(psc5415a_CON1), 
                                    (u8)(val),
                                    (u8)(CON1_LIN_LIMIT_MASK),
                                    (u8)(CON1_LIN_LIMIT_SHIFT)
                                    );
}

void psc5415a_set_v_low(u32 val)
{
    u32 ret=0;    

    ret=psc5415a_config_interface(   (u8)(psc5415a_CON1), 
                                    (u8)(val),
                                    (u8)(CON1_LOW_V_MASK),
                                    (u8)(CON1_LOW_V_SHIFT)
                                    );
}

void psc5415a_set_te(u32 val)
{
    u32 ret=0;    

    ret=psc5415a_config_interface(   (u8)(psc5415a_CON1), 
                                    (u8)(val),
                                    (u8)(CON1_TE_MASK),
                                    (u8)(CON1_TE_SHIFT)
                                    );
}

void psc5415a_set_ce(u32 val)
{
    u32 ret=0;    

    ret=psc5415a_config_interface(   (u8)(psc5415a_CON1), 
                                    (u8)(val),
                                    (u8)(CON1_CE_MASK),
                                    (u8)(CON1_CE_SHIFT)
                                    );
}

void psc5415a_set_hz_mode(u32 val)
{
    u32 ret=0;    

    ret=psc5415a_config_interface(   (u8)(psc5415a_CON1), 
                                    (u8)(val),
                                    (u8)(CON1_HZ_MODE_MASK),
                                    (u8)(CON1_HZ_MODE_SHIFT)
                                    );
}

void psc5415a_set_opa_mode(u32 val)
{
    u32 ret=0;    

    ret=psc5415a_config_interface(   (u8)(psc5415a_CON1), 
                                    (u8)(val),
                                    (u8)(CON1_OPA_MODE_MASK),
                                    (u8)(CON1_OPA_MODE_SHIFT)
                                    );
}

//CON2----------------------------------------------------

void psc5415a_set_oreg(u32 val)
{
    u32 ret=0;    

    ret=psc5415a_config_interface(   (u8)(psc5415a_CON2), 
                                    (u8)(val),
                                    (u8)(CON2_OREG_MASK),
                                    (u8)(CON2_OREG_SHIFT)
                                    );
}

void psc5415a_set_otg_pl(u32 val)
{
    u32 ret=0;    

    ret=psc5415a_config_interface(   (u8)(psc5415a_CON2), 
                                    (u8)(val),
                                    (u8)(CON2_OTG_PL_MASK),
                                    (u8)(CON2_OTG_PL_SHIFT)
                                    );
}

void psc5415a_set_otg_en(u32 val)
{
    u32 ret=0;    

    ret=psc5415a_config_interface(   (u8)(psc5415a_CON2), 
                                    (u8)(val),
                                    (u8)(CON2_OTG_EN_MASK),
                                    (u8)(CON2_OTG_EN_SHIFT)
                                    );
}

//sym add for otg in 20170418
/*void psc5415a_set_chg_en(kal_uint32 val)
{
    kal_uint32 ret=0;

    ret=psc5415a_config_interface((kal_uint8)(psc5415a_CON2),
    								(kal_uint8)(val),
    								(kal_uint8)(CON2_CHG_EN_MASK),
    								(kal_uint8)(CON2_CHG_EN_SHIFT)
    								);
}
*/
kal_uint32 psc5415a_get_otg_en(void)
{
    kal_uint32 ret=0;
    kal_uint8 val=0;

    ret=psc5415a_read_interface(     (kal_uint8)(psc5415a_CON2), 
                                    (&val),
                                    (kal_uint8)(CON3_VENDER_CODE_MASK),
                                    (kal_uint8)(CON3_VENDER_CODE_SHIFT)
                                    );    
        printk("%d\n", ret);
    
    return val;
}

kal_uint32 psc5415a_get_opa_mode(void)
{
    kal_uint32 ret=0;
    kal_uint8 val=0;

    ret=psc5415a_read_interface(     (kal_uint8)(psc5415a_CON1), 
                                    (&val),
                                    (kal_uint8)(CON1_OPA_MODE_MASK),
                                    (kal_uint8)(CON1_OPA_MODE_SHIFT)
                                    );    
        printk("%d\n", ret);
    
    return val;
}

//sym add for otg in 20170418

//CON3----------------------------------------------------

u32 psc5415a_get_vender_code(void)
{
    u32 ret=0;
    u8 val=0;

    ret=psc5415a_read_interface(     (u8)(psc5415a_CON3), 
                                    (&val),
                                    (u8)(CON3_VENDER_CODE_MASK),
                                    (u8)(CON3_VENDER_CODE_SHIFT)
                                    );
    return val;
}

u32 psc5415a_get_pn(void)
{
    u32 ret=0;
    u8 val=0;

    ret=psc5415a_read_interface(     (u8)(psc5415a_CON3), 
                                    (&val),
                                    (u8)(CON3_PIN_MASK),
                                    (u8)(CON3_PIN_SHIFT)
                                    );
    return val;
}

u32 psc5415a_get_revision(void)
{
    u32 ret=0;
    u8 val=0;

    ret=psc5415a_read_interface(     (u8)(psc5415a_CON3), 
                                    (&val),
                                    (u8)(CON3_REVISION_MASK),
                                    (u8)(CON3_REVISION_SHIFT)
                                    );
    return val;
}

//CON4----------------------------------------------------

void psc5415a_set_reset(u32 val)
{
    u32 ret=0;    

    ret=psc5415a_config_interface(   (u8)(psc5415a_CON4), 
                                    (u8)(val),
                                    (u8)(CON4_RESET_MASK),
                                    (u8)(CON4_RESET_SHIFT)
                                    );
}

void psc5415a_set_iocharge(u32 val)
{
    u32 ret=0;    

    ret=psc5415a_config_interface(   (u8)(psc5415a_CON4), 
                                    (u8)(val),
                                    (u8)(CON4_I_CHR_MASK),
                                    (u8)(CON4_I_CHR_SHIFT)
                                    );
}

void psc5415a_set_iterm(u32 val)
{
    u32 ret=0;    

    ret=psc5415a_config_interface(   (u8)(psc5415a_CON4), 
                                    (u8)(val),
                                    (u8)(CON4_I_TERM_MASK),
                                    (u8)(CON4_I_TERM_SHIFT)
                                    );
}

//CON5----------------------------------------------------

void psc5415a_set_dis_vreg(u32 val)
{
    u32 ret=0;    

    ret=psc5415a_config_interface(   (u8)(psc5415a_CON5), 
                                    (u8)(val),
                                    (u8)(CON5_DIS_VREG_MASK),
                                    (u8)(CON5_DIS_VREG_SHIFT)
                                    );
}

void psc5415a_set_io_level(u32 val)
{
    u32 ret=0;    

    ret=psc5415a_config_interface(   (u8)(psc5415a_CON5), 
                                    (u8)(val),
                                    (u8)(CON5_IO_LEVEL_MASK),
                                    (u8)(CON5_IO_LEVEL_SHIFT)
                                    );
}

u32 psc5415a_get_sp_status(void)
{
    u32 ret=0;
    u8 val=0;

    ret=psc5415a_read_interface(     (u8)(psc5415a_CON5), 
                                    (&val),
                                    (u8)(CON5_SP_STATUS_MASK),
                                    (u8)(CON5_SP_STATUS_SHIFT)
                                    );
    return val;
}

u32 psc5415a_get_en_level(void)
{
    u32 ret=0;
    u8 val=0;

    ret=psc5415a_read_interface(     (u8)(psc5415a_CON5), 
                                    (&val),
                                    (u8)(CON5_EN_LEVEL_MASK),
                                    (u8)(CON5_EN_LEVEL_SHIFT)
                                    );
    return val;
}

void psc5415a_set_vsp(u32 val)
{
    u32 ret=0;    

    ret=psc5415a_config_interface(   (u8)(psc5415a_CON5), 
                                    (u8)(val),
                                    (u8)(CON5_VSP_MASK),
                                    (u8)(CON5_VSP_SHIFT)
                                    );
}

//CON6----------------------------------------------------

void psc5415a_set_i_safe(u32 val)
{
    u32 ret=0;    

    ret=psc5415a_config_interface(   (u8)(psc5415a_CON6), 
                                    (u8)(val),
                                    (u8)(CON6_ISAFE_MASK),
                                    (u8)(CON6_ISAFE_SHIFT)
                                    );
}

void psc5415a_set_v_safe(u32 val)
{
    u32 ret=0;    

    ret=psc5415a_config_interface(   (u8)(psc5415a_CON6), 
                                    (u8)(val),
                                    (u8)(CON6_VSAFE_MASK),
                                    (u8)(CON6_VSAFE_SHIFT)
                                    );
}

/**********************************************************
  *
  *   [Internal Function] 
  *
  *********************************************************/
void psc5415a_dump_register(void)
{
    int i=0;
    printk("[psc5415a] ");
    for (i=0;i<psc5415a_REG_NUM;i++)
    {
        psc5415a_read_byte(i, &psc5415a_reg[i]);
        printk("[0x%x]=0x%x ", i, psc5415a_reg[i]);        
    }
    printk("\n");
}

#if 0
extern int g_enable_high_vbat_spec;
extern int g_pmic_cid;

void psc5415a_hw_init(void)
{    
    if(g_enable_high_vbat_spec == 1)
    {
        if(g_pmic_cid == 0x1020)
        {
            printk("[psc5415a_hw_init] (0x06,0x70) because 0x1020\n");
            psc5415a_config_interface_liao(0x06,0x70); // set ISAFE
        }
        else
        {
            printk("[psc5415a_hw_init] (0x06,0x77)\n");
            psc5415a_config_interface_liao(0x06,0x77); // set ISAFE and HW CV point (4.34)
        }
    }
    else
    {
        printk("[psc5415a_hw_init] (0x06,0x70) \n");
        psc5415a_config_interface_liao(0x06,0x70); // set ISAFE
    }
}
#endif

static int psc5415a_driver_probe(struct i2c_client *client, const struct i2c_device_id *id) 
{             
    int err=0; 
     static struct device_node *node;

    battery_xlog_printk(BAT_LOG_CRTI,"[psc5415a_driver_probe] \n");

    if (!(new_client = kmalloc(sizeof(struct i2c_client), GFP_KERNEL))) {
        err = -ENOMEM;
        goto exit;
    }    
    memset(new_client, 0, sizeof(struct i2c_client));

    new_client = client;    

    //---------------------
    /*node = of_find_compatible_node(NULL, NULL, "mediatek,charger_enable_node");
    charger_enable_pin = of_get_named_gpio(node, "charger_enable", 0);
		 gpio_request(charger_enable_pin, "charger_enable");
    gpio_direction_output(charger_enable_pin, 0);
	  gpio_set_value(charger_enable_pin, 1);*/
    //psc5415a_hw_init();
    psc5415a_dump_register();
    chargin_hw_init_done = KAL_TRUE;
	
    return 0;                                                                                       

exit:
    return err;

}

/**********************************************************
  *
  *   [platform_driver API] 
  *
  *********************************************************/
u8 g_reg_value_psc5415a=0;
static ssize_t show_psc5415a_access(struct device *dev,struct device_attribute *attr, char *buf)
{
    battery_xlog_printk(BAT_LOG_FULL,"[show_psc5415a_access] 0x%x\n", g_reg_value_psc5415a);
    return sprintf(buf, "%u\n", g_reg_value_psc5415a);
}
static ssize_t store_psc5415a_access(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    int ret=0;
    char *pvalue = NULL;
    unsigned int reg_value = 0;
    unsigned int reg_address = 0;
    
    battery_xlog_printk(BAT_LOG_FULL,"[store_psc5415a_access] \n");
    
    if(buf != NULL && size != 0)
    {
        battery_xlog_printk(BAT_LOG_FULL,"[store_psc5415a_access] buf is %s and size is %d \n",buf,size);
        reg_address = simple_strtoul(buf,&pvalue,16);
        
        if(size > 3)
        {        
            reg_value = simple_strtoul((pvalue+1),NULL,16);        
            battery_xlog_printk(BAT_LOG_FULL,"[store_psc5415a_access] write psc5415a reg 0x%x with value 0x%x !\n",reg_address,reg_value);
            ret=psc5415a_config_interface(reg_address, reg_value, 0xFF, 0x0);
        }
        else
        {    
            ret=psc5415a_read_interface(reg_address, &g_reg_value_psc5415a, 0xFF, 0x0);
            battery_xlog_printk(BAT_LOG_FULL,"[store_psc5415a_access] read psc5415a reg 0x%x with value 0x%x !\n",reg_address,g_reg_value_psc5415a);
            battery_xlog_printk(BAT_LOG_FULL,"[store_psc5415a_access] Please use \"cat psc5415a_access\" to get value\r\n");
        }        
    }    
    return size;
}
static DEVICE_ATTR(psc5415a_access, 0664, show_psc5415a_access, store_psc5415a_access); //664

static int psc5415a_user_space_probe(struct platform_device *dev)    
{    
    int ret_device_file = 0;

    battery_xlog_printk(BAT_LOG_CRTI,"******** psc5415a_user_space_probe!! ********\n" );
    
    ret_device_file = device_create_file(&(dev->dev), &dev_attr_psc5415a_access);
    
    return 0;
}

struct platform_device psc5415a_user_space_device = {
    .name   = "psc5415a-user",
    .id     = -1,
};

static struct platform_driver psc5415a_user_space_driver = {
    .probe      = psc5415a_user_space_probe,
    .driver     = {
        .name = "psc5415a-user",
    },
};


//static struct i2c_board_info __initdata i2c_psc5415a = { I2C_BOARD_INFO("psc5415a", (psc5415a_SLAVE_ADDR_WRITE>>1))};
unsigned int charger_enable_pin;
static int __init psc5415a_init(void)
{    
    int ret=0;
   
    
    battery_xlog_printk(BAT_LOG_CRTI,"[psc5415a_init] init start\n");
    
    //i2c_register_board_info(psc5415a_BUSNUM, &i2c_psc5415a, 1);

    if(i2c_add_driver(&psc5415a_driver)!=0)
    {
        battery_xlog_printk(BAT_LOG_CRTI,"[psc5415a_init] failed to register psc5415a i2c driver.\n");
    }
    else
    {
        battery_xlog_printk(BAT_LOG_CRTI,"[psc5415a_init] Success to register psc5415a i2c driver.\n");
    }

    // psc5415a user space access interface
    ret = platform_device_register(&psc5415a_user_space_device);
    if (ret) {
        battery_xlog_printk(BAT_LOG_CRTI,"****[psc5415a_init] Unable to device register(%d)\n", ret);
        return ret;
    }    
    ret = platform_driver_register(&psc5415a_user_space_driver);
    if (ret) {
        battery_xlog_printk(BAT_LOG_CRTI,"****[psc5415a_init] Unable to register driver (%d)\n", ret);
        return ret;
    }
     start_flag = 1;
    
    return 0;        
}

static void __exit psc5415a_exit(void)
{
    i2c_del_driver(&psc5415a_driver);
}

subsys_initcall(psc5415a_init);
module_exit(psc5415a_exit);
   

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("I2C psc5415a Driver");
MODULE_AUTHOR("James Lo<james.lo@mediatek.com>");
