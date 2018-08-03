/*
* Copyright (C) 2016 MediaTek Inc.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See http://www.gnu.org/licenses/gpl-2.0.html for more details.
*/

#ifndef __CUST_BOMDETECT_H__
#define __CUST_BOMDETECT_H__

#include <linux/types.h>

#define C_CUST_BOM_MAX_NUM	16


struct bomdetect_hw {
	unsigned int pin_detect_num;  	              	/*!<How many pins use to detect pcb bom>*/
	unsigned int bom_num;														/*!<How many bom defined in bom value>*/
	unsigned int bom_value[C_CUST_BOM_MAX_NUM]; 		/*!<The value read from detect pin>*/
	unsigned int bom_name1[C_CUST_BOM_MAX_NUM]; 		/*!<The first byte of bom name>*/
	unsigned int bom_name2[C_CUST_BOM_MAX_NUM]; 		/*!<The second byte of bom name>*/
	unsigned int pa_type[C_CUST_BOM_MAX_NUM];   		/*!<Pa type which each bom config*/
};

struct bomdetect_hw *get_bom_detect_dts_func(const char *, struct bomdetect_hw*);

#endif
