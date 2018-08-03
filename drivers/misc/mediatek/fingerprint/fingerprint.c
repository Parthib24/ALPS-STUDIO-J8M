#include <linux/types.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/workqueue.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/input.h>
#include <linux/uaccess.h>

#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/pinctrl/consumer.h>
#include <linux/regulator/consumer.h>
#include <linux/wakelock.h>

static bool fp_driver_already_register = false;

bool getFP_driver_probe_status(void)
{
	return fp_driver_already_register;
}

void setFP_driver_probe_status(bool status)
{
	fp_driver_already_register = status;
}

bool power_init_done = false;
#if defined(FINGERPRINT_VGP1_POWER)
struct regulator *vgp1 = NULL;
const char *vdd_name = "vgp1";
#endif

static int rgk_fingerprint_power_init(struct device *dev)
{
	int ret = 0;
	
#if defined(FINGERPRINT_VGP1_POWER)
  vgp1= regulator_get(dev,vdd_name);
  if(IS_ERR(vgp1)) 
  {
  	ret = PTR_ERR(vgp1);
    printk("Regulator get failed vdd ret=%d\n", ret);
    return ret;
  }
	
  ret = regulator_set_voltage(vgp1, 2800000, 2800000);
  if (ret) 
  {
  	printk("regulator_set_voltage(%d) failed!\n", ret);
    regulator_put(vgp1);
  }
#else
	power_init_done = true;
#endif
	return ret;
}

int rgk_fingerprint_power_on(struct device *dev)
{
	if(!power_init_done)
	{
		rgk_fingerprint_power_init(dev);
	}
#if defined(FINGERPRINT_VGP1_POWER)
	int rc;
	rc = regulator_enable(vgp1); 
	if (rc) { 
			printk("regulator_enable() failed!\n"); 
			return rc;
	}
  return rc;
#else
	return 0;
#endif
}

int rgk_fingerprint_power_off(void)
{
	printk("rgk_fingerprint_power_off\n");
#if defined(FINGERPRINT_VGP1_POWER)
	int rc;
	rc = regulator_disable(vgp1); 
	if (rc) { 
			printk("regulator_disable() failed!\n"); 
			return rc;
	}
  return rc;
#else
	return 0;
#endif
}