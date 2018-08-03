#include <linux/delay.h>
#include <linux/workqueue.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/timer.h>
#include <linux/err.h>

#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#endif

#include "gf_spi.h"

#if defined(USE_SPI_BUS)
#include <linux/spi/spi.h>
#include <linux/spi/spidev.h>
#elif defined(USE_PLATFORM_BUS)
#include <linux/platform_device.h>
#endif

/*GPIO pins reference.*/
int gf_parse_dts(struct gf_dev* gf_dev)
{
    struct device_node *node;
    struct platform_device *pdev = NULL;
    int ret = 0;

    node = of_find_compatible_node(NULL, NULL, "mediatek,mt6735m-finger");
    if (node) {
        pdev = of_find_device_by_node(node);
        if (pdev) {
            gf_dev->pinctrl_gpios = devm_pinctrl_get(&pdev->dev);
            if (IS_ERR(gf_dev->pinctrl_gpios)) {
                ret = PTR_ERR(gf_dev->pinctrl_gpios);
                pr_info("%s can't find fingerprint pinctrl\n", __func__);
                return ret;
            }
        } else {
            pr_info("%s platform device is null\n", __func__);
        }
    } else {
        pr_info("%s device node is null\n", __func__);
    }

    gf_dev->pins_irq = pinctrl_lookup_state(gf_dev->pinctrl_gpios, "fingerprint_pin_irq");
    if (IS_ERR(gf_dev->pins_irq)) {
        ret = PTR_ERR(gf_dev->pins_irq);
        pr_info("%s can't find fingerprint pinctrl irq\n", __func__);
        return ret;
    }
    gf_dev->pins_reset_high = pinctrl_lookup_state(gf_dev->pinctrl_gpios, "fingerprint_reset_high");
    if (IS_ERR(gf_dev->pins_reset_high)) {
        ret = PTR_ERR(gf_dev->pins_reset_high);
        pr_info("%s can't find fingerprint pinctrl reset_high\n", __func__);
        return ret;
    }
    gf_dev->pins_reset_low = pinctrl_lookup_state(gf_dev->pinctrl_gpios, "fingerprint_reset_low");
    if (IS_ERR(gf_dev->pins_reset_low)) {
        ret = PTR_ERR(gf_dev->pins_reset_low);
        pr_info("%s can't find fingerprint pinctrl reset_low\n", __func__);
        return ret;
    }
    pr_info("%s, get pinctrl success!\n", __func__);
    return 0;
}

void gf_cleanup(struct gf_dev* gf_dev)
{
    pr_info("[info] %s\n",__func__);
    if (gpio_is_valid(gf_dev->irq_gpio))
    {
        gpio_free(gf_dev->irq_gpio);
        pr_info("remove irq_gpio success\n");
    }
    if (gpio_is_valid(gf_dev->reset_gpio))
    {
        gpio_free(gf_dev->reset_gpio);
        pr_info("remove reset_gpio success\n");
    }
    
    if(!(IS_ERR(gf_dev->pinctrl_gpios))){
    			devm_pinctrl_put(gf_dev->pinctrl_gpios);
    		pr_info("realease spi pinctrl\n");	
    }
    
    
}



/*power management*/
#ifdef FINGERPRINT_VGP1_POWER
const char *vdd_name = "vgp1";
struct regulator *g_RegVdd = NULL;
#endif
int gf_power_on(struct gf_dev* gf_dev)
{
    int rc = 0;
//	hwPowerOn(MT6331_POWER_LDO_VIBR, VOL_2800, "fingerprint");
#ifdef FINGERPRINT_VGP1_POWER
		pr_info("nasri...%s poweron:%d,%d\n",__func__,gf_dev->poweron,__LINE__);

		if (0 == gf_dev->poweron){
			msleep(5);
			if(g_RegVdd == NULL){
				g_RegVdd = regulator_get(&(gf_dev->spi->dev), vdd_name);
				regulator_set_voltage(g_RegVdd, 2800000, 2800000); 
			}
			rc = regulator_enable(g_RegVdd);
			
			if (rc)
				printk("regulator_enable() failed!\n");
				gf_dev->poweron = 1;
				msleep(5);
		}
#endif
    return rc;
}

int gf_power_off(struct gf_dev* gf_dev)
{
    int rc = 0;
//	hwPowerDown(MT6331_POWER_LDO_VIBR, "fingerprint");
#ifdef FINGERPRINT_VGP1_POWER
		if (1 == gf_dev->poweron){
			rc = regulator_disable(g_RegVdd);
			gf_dev->poweron = 0;
			msleep(2);
		}
#endif
    return rc;
}


/********************************************************************
*CPU output low level in RST pin to reset GF. This is the MUST action for GF.
*Take care of this function. IO Pin driver strength / glitch and so on.
********************************************************************/
int gf_hw_reset(struct gf_dev *gf_dev, unsigned int delay_ms)
{
    pinctrl_select_state(gf_dev->pinctrl_gpios, gf_dev->pins_reset_low);
    mdelay((delay_ms > 3)?delay_ms:3);
    pinctrl_select_state(gf_dev->pinctrl_gpios, gf_dev->pins_reset_high);
	return 0;
}

int gf_irq_num(struct gf_dev *gf_dev)
{
    struct device_node *node;

    pinctrl_select_state(gf_dev->pinctrl_gpios, gf_dev->pins_irq);

    node = of_find_compatible_node(NULL, NULL, "mediatek,fingerprint-eint");
    if (node) {
        gf_dev->irq_num = irq_of_parse_and_map(node, 0);
        pr_info("%s, gf_irq = %d\n", __func__, gf_dev->irq_num);
        gf_dev->irq = gf_dev->irq_num;
    } else
        pr_info("%s can't find compatible node\n", __func__);
    return gf_dev->irq;
}

