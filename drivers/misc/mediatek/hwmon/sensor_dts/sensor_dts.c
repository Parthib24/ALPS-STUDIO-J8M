/*
* Copyright (C) 2011-2014 MediaTek Inc.
*
* This program is free software: you can redistribute it and/or modify it under the terms of the
* GNU General Public License version 2 as published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
* without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See the GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License along with this program.
* If not, see <http://www.gnu.org/licenses/>.
*/

#include <linux/of.h>
#include <linux/of_irq.h>
#include <cust_alsps.h>
#include <cust_acc.h>
#include <cust_gyro.h>
#include <cust_mag.h>
#include <cust_baro.h>
#include <cust_hmdy.h>
#ifdef CONFIG_RGK_HW_BOM_COMPATIBLE
#include <cust_bomdetect.h>
#endif


#define SENSOR_TAG				  "[Sensor dts] "
#define SENSOR_ERR(fmt, args...)	pr_err(SENSOR_TAG fmt, ##args)
#define SENSOR_LOG(fmt, args...)	pr_debug(SENSOR_TAG fmt, ##args)

struct acc_hw *get_accel_dts_func(const char *name, struct acc_hw *hw)
{
	int i, ret;
	u32 i2c_num[] = {0};
	u32 i2c_addr[G_CUST_I2C_ADDR_NUM] = {0};
	u32 direction[] = {0};
	u32 power_id[] = {0};
	u32 power_vol[] = {0};
	u32 firlen[] = {0};
	u32 is_batch_supported[] = {0};
	struct device_node *node = NULL;

	SENSOR_LOG("Device Tree get accel info!\n");
	if (name == NULL)
		return NULL;

	node = of_find_compatible_node(NULL, NULL, name);
	if (node) {
		ret = of_property_read_u32_array(node , "i2c_num", i2c_num, ARRAY_SIZE(i2c_num));
		if (ret == 0)
			hw->i2c_num	=	i2c_num[0];

		ret = of_property_read_u32_array(node , "i2c_addr", i2c_addr, ARRAY_SIZE(i2c_addr));
		if (ret == 0) {
			for (i = 0; i < G_CUST_I2C_ADDR_NUM; i++)
				hw->i2c_addr[i] = i2c_addr[i];
		}

		ret = of_property_read_u32_array(node , "direction", direction, ARRAY_SIZE(direction));
		if (ret == 0)
			hw->direction = direction[0];

		ret = of_property_read_u32_array(node , "power_id", power_id, ARRAY_SIZE(power_id));
		if (ret == 0) {
			if (power_id[0] == 0xffff)
				hw->power_id = -1;
			else
				hw->power_id	=	power_id[0];
		}

		ret = of_property_read_u32_array(node , "power_vol", power_vol, ARRAY_SIZE(power_vol));
		if (ret == 0)
			hw->power_vol	=	power_vol[0];

		ret = of_property_read_u32_array(node , "firlen", firlen, ARRAY_SIZE(firlen));
		if (ret == 0)
			hw->firlen	=	firlen[0];

		ret = of_property_read_u32_array(node , "is_batch_supported",
			is_batch_supported, ARRAY_SIZE(is_batch_supported));
		if (ret == 0)
			hw->is_batch_supported		 = is_batch_supported[0];
	} else {
		SENSOR_ERR("Device Tree: can not find accel node!. Go to use old cust info\n");
		return NULL;
	}

	return hw;
}
EXPORT_SYMBOL_GPL(get_accel_dts_func);

struct alsps_hw *get_alsps_dts_func(const char *name, struct alsps_hw *hw)
{
	int i, ret;
	u32 i2c_num[] = {0};
	u32 i2c_addr[C_CUST_I2C_ADDR_NUM] = {0};
	u32 power_id[] = {0};
	u32 power_vol[] = {0};
	u32 polling_mode_ps[] = {0};
	u32 polling_mode_als[] = {0};
	u32 als_level[C_CUST_ALS_LEVEL-1] = {0};
	u32 als_value[C_CUST_ALS_LEVEL] = {0};
	u32 ps_threshold_high[] = {0};
	u32 ps_threshold_low[] = {0};
	u32 is_batch_supported_ps[] = {0};
	u32 is_batch_supported_als[] = {0};
	struct device_node *node = NULL;

	SENSOR_LOG("Device Tree get alsps info!\n");
	if (name == NULL)
		return NULL;

	node = of_find_compatible_node(NULL, NULL, name);
	if (node) {
		ret = of_property_read_u32_array(node , "i2c_num", i2c_num, ARRAY_SIZE(i2c_num));
	if (ret == 0)
		hw->i2c_num	=	i2c_num[0];

	ret = of_property_read_u32_array(node , "i2c_addr", i2c_addr, ARRAY_SIZE(i2c_addr));
	if (ret == 0) {
		for (i = 0; i < C_CUST_I2C_ADDR_NUM; i++)
			hw->i2c_addr[i]   = i2c_addr[i];
	}

	ret = of_property_read_u32_array(node , "power_id", power_id, ARRAY_SIZE(power_id));
	if (ret == 0) {
		if (power_id[0] == 0xffff)
			hw->power_id = -1;
		else
			hw->power_id	=	power_id[0];
	}

	ret = of_property_read_u32_array(node , "power_vol", power_vol, ARRAY_SIZE(power_vol));
	if (ret == 0)
		hw->power_vol	=	power_vol[0];

	ret = of_property_read_u32_array(node , "als_level", als_level, ARRAY_SIZE(als_level));
	if (ret == 0) {
		for (i = 0; i < ARRAY_SIZE(als_level); i++)
			hw->als_level[i]		 = als_level[i];
	}

	ret = of_property_read_u32_array(node , "als_value", als_value, ARRAY_SIZE(als_value));
	if (ret == 0) {
		for (i = 0; i < ARRAY_SIZE(als_value); i++)
			hw->als_value[i]		 = als_value[i];
	}

	ret = of_property_read_u32_array(node , "polling_mode_ps", polling_mode_ps, ARRAY_SIZE(polling_mode_ps));
	if (ret == 0)
		hw->polling_mode_ps		 = polling_mode_ps[0];

	ret = of_property_read_u32_array(node , "polling_mode_als", polling_mode_als, ARRAY_SIZE(polling_mode_als));
	if (ret == 0)
		hw->polling_mode_als		 = polling_mode_als[0];

	ret = of_property_read_u32_array(node , "ps_threshold_high", ps_threshold_high, ARRAY_SIZE(ps_threshold_high));
	if (ret == 0)
		hw->ps_threshold_high		 = ps_threshold_high[0];

	ret = of_property_read_u32_array(node , "ps_threshold_low", ps_threshold_low, ARRAY_SIZE(ps_threshold_low));
	if (ret == 0)
		hw->ps_threshold_low		 = ps_threshold_low[0];

	ret = of_property_read_u32_array(node , "is_batch_supported_ps", is_batch_supported_ps,
		ARRAY_SIZE(is_batch_supported_ps));
	if (ret == 0)
		hw->is_batch_supported_ps		 = is_batch_supported_ps[0];

	ret = of_property_read_u32_array(node , "is_batch_supported_als", is_batch_supported_als,
		ARRAY_SIZE(is_batch_supported_als));
	if (ret == 0)
		hw->is_batch_supported_als		 = is_batch_supported_als[0];
	} else {
		SENSOR_ERR("Device Tree: can not find alsps node!. Go to use old cust info\n");
		return NULL;
	}
	return hw;
}
EXPORT_SYMBOL_GPL(get_alsps_dts_func);

struct mag_hw *get_mag_dts_func(const char *name, struct mag_hw *hw)
{
	int i, ret;
	u32 i2c_num[] = {0};
	u32 i2c_addr[M_CUST_I2C_ADDR_NUM] = {0};
	u32 direction[] = {0};
	u32 power_id[] = {0};
	u32 power_vol[] = {0};
	u32 is_batch_supported[] = {0};
	struct device_node *node = NULL;

	SENSOR_LOG("Device Tree get mag info!\n");
	if (name == NULL)
		return NULL;

	node = of_find_compatible_node(NULL, NULL, name);
	if (node) {
		ret = of_property_read_u32_array(node , "i2c_num", i2c_num, ARRAY_SIZE(i2c_num));
		if (ret == 0)
			hw->i2c_num	=	i2c_num[0];

		ret = of_property_read_u32_array(node , "i2c_addr", i2c_addr, ARRAY_SIZE(i2c_addr));
		if (ret == 0) {
			for (i = 0; i < M_CUST_I2C_ADDR_NUM; i++)
				hw->i2c_addr[i]   = i2c_addr[i];
		}

		ret = of_property_read_u32_array(node , "direction", direction, ARRAY_SIZE(direction));
		if (ret == 0)
			hw->direction = direction[0];

		ret = of_property_read_u32_array(node , "power_id", power_id, ARRAY_SIZE(power_id));
		if (ret == 0) {
			if (power_id[0] == 0xffff)
				hw->power_id = -1;
			else
				hw->power_id	=	 power_id[0];
		}

		ret = of_property_read_u32_array(node , "power_vol", power_vol, ARRAY_SIZE(power_vol));
		if (ret == 0)
			hw->power_vol	 =	  power_vol[0];

		ret = of_property_read_u32_array(node , "is_batch_supported", is_batch_supported,
			ARRAY_SIZE(is_batch_supported));
		if (ret == 0)
			hw->is_batch_supported		   = is_batch_supported[0];
	} else {
		SENSOR_ERR("Device Tree: can not find mag node!. Go to use old cust info\n");
		return NULL;
	}
	return hw;
}
EXPORT_SYMBOL_GPL(get_mag_dts_func);

struct gyro_hw *get_gyro_dts_func(const char *name, struct gyro_hw *hw)
{
	int i, ret;
	u32 i2c_num[] = {0};
	u32 i2c_addr[C_CUST_I2C_ADDR_NUM] = {0};
	u32 direction[] = {0};
	u32 power_id[] = {0};
	u32 power_vol[] = {0};
	u32 firlen[] = {0};
	u32 is_batch_supported[] = {0};
	struct device_node *node = NULL;

	SENSOR_LOG("Device Tree get gyro info!\n");
	if (name == NULL)
		return NULL;

	node = of_find_compatible_node(NULL, NULL, name);
	if (node) {
		ret = of_property_read_u32_array(node , "i2c_num", i2c_num, ARRAY_SIZE(i2c_num));
		if (ret == 0)
			hw->i2c_num	=	i2c_num[0];

		ret = of_property_read_u32_array(node , "i2c_addr", i2c_addr, ARRAY_SIZE(i2c_addr));
		if (ret == 0) {
			for (i = 0; i < GYRO_CUST_I2C_ADDR_NUM; i++)
				hw->i2c_addr[i] = i2c_addr[i];
		}

		ret = of_property_read_u32_array(node , "direction", direction, ARRAY_SIZE(direction));
		if (ret == 0)
			hw->direction = direction[0];

		ret = of_property_read_u32_array(node , "power_id", power_id, ARRAY_SIZE(power_id));
		if (ret == 0) {
			if (power_id[0] == 0xffff)
				hw->power_id = -1;
			else
				hw->power_id	=	power_id[0];
		}

		ret = of_property_read_u32_array(node , "power_vol", power_vol, ARRAY_SIZE(power_vol));
		if (ret == 0)
			hw->power_vol	=	power_vol[0];

		ret = of_property_read_u32_array(node , "firlen", firlen, ARRAY_SIZE(firlen));
		if (ret == 0)
			hw->firlen	=	firlen[0];

		ret = of_property_read_u32_array(node , "is_batch_supported", is_batch_supported,
			ARRAY_SIZE(is_batch_supported));
		if (ret == 0)
			hw->is_batch_supported		 = is_batch_supported[0];
	} else {
		SENSOR_ERR("Device Tree: can not find gyro node!. Go to use old cust info\n");
		return NULL;
	}
	return hw;
}
EXPORT_SYMBOL_GPL(get_gyro_dts_func);

struct baro_hw *get_baro_dts_func(const char *name, struct baro_hw *hw)
{
	int i, ret;
	u32 i2c_num[] = {0};
	u32 i2c_addr[C_CUST_I2C_ADDR_NUM] = {0};
	u32 direction[] = {0};
	u32 power_id[] = {0};
	u32 power_vol[] = {0};
	u32 firlen[] = {0};
	u32 is_batch_supported[] = {0};
	struct device_node *node = NULL;

	SENSOR_LOG("Device Tree get gyro info!\n");
	if (name == NULL)
		return NULL;

	node = of_find_compatible_node(NULL, NULL, name);
	if (node) {
		ret = of_property_read_u32_array(node , "i2c_num", i2c_num, ARRAY_SIZE(i2c_num));
		if (ret == 0)
			hw->i2c_num	=	i2c_num[0];

		ret = of_property_read_u32_array(node , "i2c_addr", i2c_addr, ARRAY_SIZE(i2c_addr));
		if (ret == 0) {
			for (i = 0; i < GYRO_CUST_I2C_ADDR_NUM; i++)
				hw->i2c_addr[i] = i2c_addr[i];
		}

		ret = of_property_read_u32_array(node , "direction", direction, ARRAY_SIZE(direction));
		if (ret == 0)
			hw->direction = direction[0];

		ret = of_property_read_u32_array(node , "power_id", power_id, ARRAY_SIZE(power_id));
		if (ret == 0) {
			if (power_id[0] == 0xffff)
				hw->power_id = -1;
			else
				hw->power_id	=	power_id[0];
		}

		ret = of_property_read_u32_array(node , "power_vol", power_vol, ARRAY_SIZE(power_vol));
		if (ret == 0)
			hw->power_vol	=	power_vol[0];

		ret = of_property_read_u32_array(node , "firlen", firlen, ARRAY_SIZE(firlen));
		if (ret == 0)
			hw->firlen	=	firlen[0];

		ret = of_property_read_u32_array(node , "is_batch_supported", is_batch_supported,
			ARRAY_SIZE(is_batch_supported));
		if (ret == 0)
			hw->is_batch_supported		 = is_batch_supported[0];
	} else {
		SENSOR_ERR("Device Tree: can not find gyro node!. Go to use old cust info\n");
		return NULL;
	}
	return hw;
}
EXPORT_SYMBOL_GPL(get_baro_dts_func);

struct hmdy_hw *get_hmdy_dts_func(const char *name, struct hmdy_hw *hw)
{
	int i, ret;
	u32 i2c_num[] = {0};
	u32 i2c_addr[C_CUST_I2C_ADDR_NUM] = {0};
	u32 direction[] = {0};
	u32 power_id[] = {0};
	u32 power_vol[] = {0};
	u32 firlen[] = {0};
	u32 is_batch_supported[] = {0};
	struct device_node *node = NULL;

	SENSOR_LOG("Device Tree get gyro info!\n");
	if (name == NULL)
		return NULL;

	node = of_find_compatible_node(NULL, NULL, name);
	if (node) {
		ret = of_property_read_u32_array(node , "i2c_num", i2c_num, ARRAY_SIZE(i2c_num));
		if (ret == 0)
			hw->i2c_num	=	i2c_num[0];

		ret = of_property_read_u32_array(node , "i2c_addr", i2c_addr, ARRAY_SIZE(i2c_addr));
		if (ret == 0) {
			for (i = 0; i < GYRO_CUST_I2C_ADDR_NUM; i++)
				hw->i2c_addr[i] = i2c_addr[i];
		}

		ret = of_property_read_u32_array(node , "direction", direction, ARRAY_SIZE(direction));
		if (ret == 0)
			hw->direction = direction[0];

		ret = of_property_read_u32_array(node , "power_id", power_id, ARRAY_SIZE(power_id));
		if (ret == 0) {
			if (power_id[0] == 0xffff)
				hw->power_id = -1;
			else
				hw->power_id	=	power_id[0];
		}

		ret = of_property_read_u32_array(node , "power_vol", power_vol, ARRAY_SIZE(power_vol));
		if (ret == 0)
			hw->power_vol	=	power_vol[0];

		ret = of_property_read_u32_array(node , "firlen", firlen, ARRAY_SIZE(firlen));
		if (ret == 0)
			hw->firlen	=	firlen[0];

		ret = of_property_read_u32_array(node , "is_batch_supported", is_batch_supported,
			ARRAY_SIZE(is_batch_supported));
		if (ret == 0)
			hw->is_batch_supported		 = is_batch_supported[0];
	} else {
		SENSOR_ERR("Device Tree: can not find gyro node!. Go to use old cust info\n");
		return NULL;
	}
	return hw;
}
EXPORT_SYMBOL_GPL(get_hmdy_dts_func);

#ifdef CONFIG_RGK_HW_BOM_COMPATIBLE
struct bomdetect_hw *get_bom_detect_dts_func(const char * name, struct bomdetect_hw* hw)
{
	int i, ret;
	int bom_config_num = 0;
	u32 pin_detect_num[] = {0};
	u32 bom_value[C_CUST_BOM_MAX_NUM] = {0};
	u32 bom_name1[C_CUST_BOM_MAX_NUM] = {0};
	u32 bom_name2[C_CUST_BOM_MAX_NUM] = {0};
	u32 pa_type[C_CUST_BOM_MAX_NUM] = {0};
	struct device_node *node = NULL;
	
	SENSOR_LOG("Device Tree get bom detect info!\n");
	node = of_find_compatible_node(NULL, NULL, name);
	
	if(node) {
		
		ret = of_property_read_u32_array(node , "detectpin_num", pin_detect_num, ARRAY_SIZE(pin_detect_num));
		if(ret == 0) {
			hw->pin_detect_num = pin_detect_num[0];
			//SENSOR_ERR("pin detect num is %d\n", hw->pin_detect_num);
		}
		
		ret = of_property_read_u32_array(node , "bom_value", bom_value, ARRAY_SIZE(bom_value));
		if(ret == 0) {
			for(i = 0; i < C_CUST_BOM_MAX_NUM; i++) {
				hw->bom_value[i] = bom_value[i];
				if(bom_value[i] != 0xFF){
					bom_config_num++;
				}
				//SENSOR_ERR("pin bom value (%d) is: 0x%x\n", i, hw->bom_value[i]);
			}
			hw->bom_num = bom_config_num;
			//SENSOR_ERR("pin bom total num is: %d\n", hw->bom_num);
		}
		
		ret = of_property_read_u32_array(node , "bom_name1", bom_name1, ARRAY_SIZE(bom_name1));
		if(ret ==0) {
			for(i = 0; i < C_CUST_BOM_MAX_NUM; i++) {
				hw->bom_name1[i] = bom_name1[i];
				//SENSOR_ERR("pin bom name 1 (%d) is: 0x%x\n", i, hw->bom_name1[i]);
			}
		}
		
		ret = of_property_read_u32_array(node , "bom_name2", bom_name2, ARRAY_SIZE(bom_name2));
		if(ret == 0) {
			for(i = 0; i < C_CUST_BOM_MAX_NUM; i++) {
				hw->bom_name2[i] = bom_name2[i];
				//SENSOR_ERR("pin bom name 2 (%d) is: 0x%x\n", i, hw->bom_name2[i]);
			}
		}
		
		ret = of_property_read_u32_array(node , "pa_type", pa_type, ARRAY_SIZE(pa_type));
		if(ret == 0) {
			for(i = 0; i < C_CUST_BOM_MAX_NUM; i++) {
				hw->pa_type[i] = pa_type[i];
				//SENSOR_ERR("pin bom pa type (%d) is: 0x%x\n", i, hw->pa_type[i]);
			}
		}
	} else {
		SENSOR_ERR("Device Tree: can not find bom detect!. Go to use old cust info\n");
		return NULL;		
	}
	
	return hw;
}
EXPORT_SYMBOL_GPL(get_bom_detect_dts_func);
#endif