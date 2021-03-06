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

#ifndef __MT_CHIP_COMMON_H__
#define __MT_CHIP_COMMON_H__
#include <mt-plat/mt_chip.h>

#define CHIP_INFO_BIT(ID) (2 << ID)
#define CHIP_INFO_SUP(MASK, ID) ((MASK & CHIP_INFO_BIT(ID)) ? (1) : (0))

#define C_UNKNOWN_CHIP_ID (0x0000FFFF)

#define MEM_INFO  //add by xujiwei-----2016-10-25

struct mt_chip_drv {
    /* raw information */
	unsigned int info_bit_mask;
	unsigned int (*get_chip_info)(unsigned int id);
};

typedef unsigned int (*chip_info_cb)(void);
struct mt_chip_drv *get_mt_chip_drv(void);

#ifdef CONFIG_RGK_HW_BOM_COMPATIBLE
enum bom_info_id {
	BOM_INFO_NONE = 0,
	
	BOM_INFO_NAME,
	BOM_INFO_PA_TYPE,
	BOM_IS_EX_PA,


	BOM_INFO_MAX,
	BOM_INFO_ALL,
};
#endif

#endif
