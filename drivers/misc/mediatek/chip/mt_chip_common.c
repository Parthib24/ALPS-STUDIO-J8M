/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#define pr_fmt(fmt) "["KBUILD_MODNAME"] " fmt
#include <linux/module.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/uaccess.h>
#include <linux/mm.h>
#include <linux/kfifo.h>

#include <linux/firmware.h>
#include <linux/syscalls.h>
#include <linux/uaccess.h>
#include <linux/platform_device.h>
#include <linux/proc_fs.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/printk.h>

#include <asm/setup.h>
#include "mt_chip_common.h"


struct mt_chip_drv g_chip_drv = {
	.info_bit_mask = CHIP_INFO_BIT(CHIP_INFO_ALL)
};

struct mt_chip_drv *get_mt_chip_drv(void)
{
	return &g_chip_drv;
}

struct chip_inf_entry {
	const char *name;
	unsigned int id;
	int (*to_str)(char *buf, size_t len, int val);
};

static int hex2str(char *buf, size_t len, int val)
{
	return snprintf(buf, len, "%04X", val);
}

static int dec2str(char *buf, size_t len, int val)
{
	return snprintf(buf, len, "%04d", val);
}

static int date2str(char *buf, size_t len, int val)
{
	unsigned int year = ((val & 0x3C0) >> 6) + 2012;
	unsigned int week = (val & 0x03F);

	return snprintf(buf, len, "%04d%02d", year, week);
}

#define __chip_info(id) ((g_chip_drv.get_chip_info) ? (g_chip_drv.get_chip_info(id)) : (0x0000))

static struct proc_dir_entry *chip_proc;
static struct chip_inf_entry chip_ent[] = {
	{"hw_code", CHIP_INFO_HW_CODE, hex2str},
	{"hw_subcode", CHIP_INFO_HW_SUBCODE, hex2str},
	{"hw_ver", CHIP_INFO_HW_VER, hex2str},
	{"sw_ver", CHIP_INFO_SW_VER, hex2str},
	{"code_func", CHIP_INFO_FUNCTION_CODE, hex2str},
	{"code_date", CHIP_INFO_DATE_CODE, date2str},
	{"code_proj", CHIP_INFO_PROJECT_CODE, dec2str},
	{"code_fab", CHIP_INFO_FAB_CODE, hex2str},
	{"wafer_big_ver", CHIP_INFO_WAFER_BIG_VER, hex2str},
	{"info", CHIP_INFO_ALL, NULL}
};

static int chip_proc_show(struct seq_file *s, void *v)
{
	struct chip_inf_entry *ent = s->private;

	if ((ent->id > CHIP_INFO_NONE) && (ent->id < CHIP_INFO_MAX)) {
		seq_printf(s, "%04X\n", __chip_info(ent->id));
	} else {
		int idx = 0;
		char buf[16];

		for (idx = 0; idx < ARRAY_SIZE(chip_ent); idx++) {
			struct chip_inf_entry *ent = &chip_ent[idx];
			unsigned int val = __chip_info(ent->id);

			if (!CHIP_INFO_SUP(g_chip_drv.info_bit_mask, ent->id))
				continue;
			else if (!ent->to_str)
				continue;
			else if (0 < ent->to_str(buf, sizeof(buf), val))
				seq_printf(s, "%-16s : %s (%04x)\n", ent->name, buf, val);
			else
				seq_printf(s, "%-16s : %s (%04x)\n", ent->name, "NULL", val);
		}
		seq_printf(s, "%-16s : %04X %04X %04X %04X\n", "reg",
			   __chip_info(CHIP_INFO_REG_HW_CODE),
			   __chip_info(CHIP_INFO_REG_HW_SUBCODE),
			   __chip_info(CHIP_INFO_REG_HW_VER), __chip_info(CHIP_INFO_REG_SW_VER));
	}
	return 0;
}

static int chip_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, chip_proc_show, PDE_DATA(file_inode(file)));
}

static const struct file_operations chip_proc_fops = {
	.open = chip_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static void __init create_procfs(void)
{
	int idx;

	chip_proc = proc_mkdir_data("chip", 0, NULL, NULL);
	if (NULL == chip_proc) {
		pr_err("create /proc/chip fails\n");
		return;
	}

	pr_debug("create /proc/chip(%x)\n", g_chip_drv.info_bit_mask);

	for (idx = 0; idx < ARRAY_SIZE(chip_ent); idx++) {
		struct chip_inf_entry *ent = &chip_ent[idx];

		if (!CHIP_INFO_SUP(g_chip_drv.info_bit_mask, ent->id))
			continue;
		if (NULL == proc_create_data(ent->name, S_IRUGO, chip_proc, &chip_proc_fops, ent)) {
			pr_err("create /proc/chip/%s fail\n", ent->name);
			return;
		}
	}
}

#ifdef CONFIG_RGK_HW_BOM_COMPATIBLE
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <cust_bomdetect.h>


#define BOM_DETECT_PIN_MAX 4
#define PA_CLASS_TYPE_MAX 3
#define COMPATIABLE_NAME "mediatek,bomdetect"

static struct bomdetect_hw bomdetect_cust;
static struct bomdetect_hw *hw = &bomdetect_cust;
static unsigned int pcb_bom_detect_pins[BOM_DETECT_PIN_MAX] = {0};
static const char* bom_detect_pin_name[BOM_DETECT_PIN_MAX] = {"bom_detect_pin0", "bom_detect_pin1", "bom_detect_pin2", "bom_detect_pin3"};
static const char* pa_class_type[PA_CLASS_TYPE_MAX] = {"classD", "ClassAB", "External"};
static unsigned int pcb_bom_value = 0x0;
static int bom_flag = -1;

struct bom_inf_entry {
	const char *name;
	unsigned int id;
	int (*to_str)(char *buf, size_t len, int val);
};

static struct proc_dir_entry *bom_proc;
static struct bom_inf_entry bom_ent[] = {
	{"bom_name", BOM_INFO_NAME, NULL},
	{"pa_type", BOM_INFO_PA_TYPE, NULL},
	{"isexpa", BOM_IS_EX_PA, NULL},
	{"info", CHIP_INFO_ALL, NULL}
};

int parse_dts(void)
{
    struct device_node *node;
    //struct platform_device *pdev = NULL;
    //struct pinctrl *pinctrl = NULL;
    int i, ret = 0;

    node = of_find_compatible_node(NULL, NULL, COMPATIABLE_NAME);
    if (!node) {
        pr_err("%s device node is null\n", __func__);
    }
    
    for(i = 0; i < hw->pin_detect_num; i++) {
    	pcb_bom_detect_pins[i] = of_get_named_gpio(node, bom_detect_pin_name[i], 0);
    	gpio_request(pcb_bom_detect_pins[i], bom_detect_pin_name[i]);
    }
    return ret;
}

void mach_pcb_bom_value(void)
{
	int i;
	for(i = 0; i < hw->pin_detect_num; i++){
		pcb_bom_value = (pcb_bom_value << i) | gpio_get_value(pcb_bom_detect_pins[i]);
	}
	
	if(hw != NULL) {
		for(i = 0; i < hw->bom_num; i ++){
			if(hw->bom_value[i] == pcb_bom_value) {
				bom_flag = i;
				break;
			}
		}
	}
	
	pr_err("(%s)PCB BOM value is 0x%x, bom_flag is %d\n", __func__, pcb_bom_value, bom_flag);
}
void dump_hw_cust(void) 
{
	
	if(hw != NULL){
		pr_err("***********************************************************************************************\n");
		pr_err("(%s)PCB BOM HW CUST DUMP: Detect pin num is(%d), Total bom num is(%d)\n", __func__, hw->pin_detect_num, hw->bom_num);
		pr_err("(%s)PCB BOM HW CUST DUMP: bom value is->0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n", __func__,
			hw->bom_value[0], hw->bom_value[1], hw->bom_value[2], hw->bom_value[3], hw->bom_value[4], hw->bom_value[5], hw->bom_value[6], hw->bom_value[7],
			hw->bom_value[8], hw->bom_value[9], hw->bom_value[10], hw->bom_value[11], hw->bom_value[12], hw->bom_value[13], hw->bom_value[14], hw->bom_value[15]);

		pr_err("(%s)PCB BOM HW CUST DUMP: bom name_1->0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n", __func__,
		hw->bom_name1[0], hw->bom_name1[1], hw->bom_name1[2], hw->bom_name1[3], hw->bom_name1[4], hw->bom_name1[5], hw->bom_name1[6], hw->bom_name1[7],
		hw->bom_name1[8], hw->bom_name1[9], hw->bom_name1[10], hw->bom_name1[11], hw->bom_name1[12], hw->bom_name1[13], hw->bom_name1[14], hw->bom_name1[15]);

		pr_err("(%s)PCB BOM HW CUST DUMP: bom name_2->0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n", __func__,
		hw->bom_name2[0], hw->bom_name2[1], hw->bom_name2[2], hw->bom_name2[3], hw->bom_name2[4], hw->bom_name2[5], hw->bom_name2[6], hw->bom_name2[7],
		hw->bom_name2[8], hw->bom_name2[9], hw->bom_name2[10], hw->bom_name2[11], hw->bom_name2[12], hw->bom_name2[13], hw->bom_name2[14], hw->bom_name2[15]);

		pr_err("(%s)PCB BOM HW CUST DUMP: bom pa type->%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n", __func__,
		hw->pa_type[0], hw->pa_type[1], hw->pa_type[2], hw->pa_type[3], hw->pa_type[4], hw->pa_type[5], hw->pa_type[6], hw->pa_type[7],
		hw->pa_type[8], hw->pa_type[9], hw->pa_type[10], hw->pa_type[11], hw->pa_type[12], hw->pa_type[13], hw->pa_type[14], hw->pa_type[15]);
		pr_err("***********************************************************************************************\n");
	}
}
	
void init_gpio_and_read_bom_value(void)
{
	const char *name = COMPATIABLE_NAME;
	
	hw = get_bom_detect_dts_func(name, hw);
	dump_hw_cust();
	parse_dts();
	mach_pcb_bom_value();
}

void show_bom_name(struct seq_file *s)
{
	if(bom_flag != -1 && 
		hw->bom_name1[bom_flag] != 0xFF && 
		hw->bom_name2[bom_flag] != 0xFF){
		seq_printf(s, "%c%d\n", hw->bom_name1[bom_flag],hw->bom_name2[bom_flag]);
	} else {
		seq_printf(s, "Unknown hardware bom config.\n");
	}
}

void show_pa_type(struct seq_file *s)
{
	if(bom_flag != -1 && 
		hw->pa_type[bom_flag] != 0xFF &&
		hw->pa_type[bom_flag] < PA_CLASS_TYPE_MAX) {
		seq_printf(s, "%s\n", pa_class_type[hw->pa_type[bom_flag]]);
	} else {
		seq_printf(s, "Unknown type.\n");
	}
}

static int bom_proc_show(struct seq_file *s, void *v)
{
	struct bom_inf_entry *ent = s->private;

	switch(ent->id){
		case BOM_INFO_NAME:
			show_bom_name(s);
			break;
		case BOM_INFO_PA_TYPE:
			show_pa_type(s);
			break;
		case BOM_IS_EX_PA:
			if(hw->pa_type[bom_flag] == 0x02){
				seq_printf(s, "1\n"); //use external pa.
			} else {
				seq_printf(s, "0\n"); //use internal pa.
			}
		default:
			pr_err("Wrong commend type %d\n", ent->id);
			break;
	}
	return 0;
}

static int bom_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, bom_proc_show, PDE_DATA(file_inode(file)));
}

static const struct file_operations bom_proc_fops = {
	.open = bom_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static void __init create_pcb_bom_procfs(void)
{
	int idx;

	bom_proc = proc_mkdir_data("pcb_bom", 0, NULL, NULL);
	if (NULL == bom_proc) {
		pr_err("create /proc/pcb_bom fails\n");
		return;
	}

	pr_debug("create /proc/chippcb_bom\n");

	for (idx = 0; idx < ARRAY_SIZE(bom_ent); idx++) {
		struct bom_inf_entry *ent = &bom_ent[idx];

		if (NULL == proc_create_data(ent->name, S_IRUGO, bom_proc, &bom_proc_fops, ent)) {
			pr_err("create /proc/pcb_bom/%s fail\n", ent->name);
			return;
		}
	}
}

int get_internal_pa_type(void)
{
	if(hw->pa_type[bom_flag] == 0x0 ||
		hw->pa_type[bom_flag] == 0x1){
		return hw->pa_type[bom_flag];
	} else {
		return 1;
	}
}

EXPORT_SYMBOL_GPL(get_internal_pa_type);
#endif

#ifdef MEM_INFO
//rgk xujiwei add for *#*#9375#*#* start (2016-10-25)
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

char rgk_mem_name[100]= {0};
static int __init init_rgk_mem_name(char *str)
{
       strncpy(rgk_mem_name, str, sizeof(rgk_mem_name)-1);
       rgk_mem_name[sizeof(rgk_mem_name)-1] = '\0';
       return 1;
}
__setup("rgk_memname=", init_rgk_mem_name);

static int rgk_mem_info_show(struct seq_file *m, void *v)
{
       seq_printf(m, "%s\n", rgk_mem_name);   
       printk("create /proc/wanghanfeng/%s \n", rgk_mem_name);
       return 0;
}

static int rgk_mem_info_open(struct inode *inode, struct file *file)
{
       return single_open(file, rgk_mem_info_show, NULL);
}

static const struct file_operations rgk_mem_info_proc_fops = { 
       .owner      = THIS_MODULE,
       .open       = rgk_mem_info_open,
       .read       = seq_read,
       .llseek     = seq_lseek,
       .release    = single_release,
};

int rgk_creat_proc_mem_info(void)
{
	     
       struct proc_dir_entry *mem_info_entry = proc_create("rgk_memInfo", 0444, NULL, &rgk_mem_info_proc_fops);
      
       if (mem_info_entry == NULL)
               printk("create /proc/mem_info_entry fail\n");
       
       return 0;
}

#endif//rgk wanghanfeng add for *#*#9375#*#* end (2015-11-30)


int __init chip_common_init(void)
{
#ifdef CONFIG_RGK_HW_BOM_COMPATIBLE
		init_gpio_and_read_bom_value();
		create_pcb_bom_procfs();
#endif	
		rgk_creat_proc_mem_info();

	create_procfs();
	return 0;
}

arch_initcall(chip_common_init);
MODULE_DESCRIPTION("MTK Chip Common");
MODULE_LICENSE("GPL");
