#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/ioctl.h>
#include <linux/delay.h>
#include <linux/string.h>
#include <linux/ctype.h>
#include <linux/module.h>
#include <linux/input.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>
#include <linux/switch.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>
#include<linux/kthread.h>
#include <sensors_io.h>
#include <hwmsen_helper.h>
#include <batch.h>
#include <hwmsensor.h>
#include <hwmsen_dev.h>
/*******************modify  by wanghanfeng start*********************************/
#define KEY_MHALL_COVER_OPEN			KEY_F1
#define KEY_MHALL_COVER_CLOSE			KEY_F2
//#define KEY_MHALL_COVER_OPEN			KEY_OPTION
//#define KEY_MHALL_COVER_CLOSE			KEY_POWER2
/*******************modify  by wanghanfeng end*********************************/
#ifdef RGK_MHALL_AK8789_SUPPORT
#define MHALL_DEVICE_NAME				"AK8789"
#elif defined(RGK_MHALL_OCH165_SUPPORT)
#define MHALL_DEVICE_NAME				"OCH165"
#else
#define MHALL_DEVICE_NAME				"UNKNOWN"
#endif

#define MHALL_DEVNAME 				"RGK_MHALL"
#define MHALL_DEBUG
#ifdef MHALL_DEBUG
#define MHALL_LOG(fmt, args...)			printk("[MHALL-" MHALL_DEVICE_NAME "] " fmt, ##args)
#else
#define MHALL_LOG(fmt, args...)
#endif

#define MHALL_NAME						"mhall"
//add by xuhongming for holl sense start
#ifdef RGK_TOUCHPANEL_HALL_MODE
extern int g_tpd_opencover_fun(void);
extern int g_tpd_closecover_fun(void);
#endif
//add by xuhongming for holl sense end
static bool mhall = 0;
core_param(mhall, mhall, bool, 0444);
static bool pull_u = 0;
core_param(pull_u, pull_u, bool, 0444);
static bool pull_d = 0;
core_param(pull_d, pull_d, bool, 0444);
bool mhall_flag = 0;
core_param(mhall_flag, mhall_flag, bool, 0444);

static struct input_dev *mhall_input_dev;
struct device_node *mhall_node;
static DECLARE_WAIT_QUEUE_HEAD(mhall_thread_wq);
static atomic_t mhall_wakeup_flag = ATOMIC_INIT(0);
static int mhall_cover_open_gpio_value = 1;

static unsigned int mhall_pin;
static unsigned int mhall_number;

u32 ints[2] = {0, 0};


static struct platform_device *rgk_mhall  = NULL;
struct pinctrl *mhallctl;
struct pinctrl_state *mhall_int;

struct switch_dev switch_hall;

#ifdef CONFIG_OF
static const struct of_device_id mhall_of_match[] = {
	{.compatible = "mediatek,rgkmhall",},
	{},
};
#endif

static struct class *mhall_class;
static ssize_t enable_mhall_show(struct class *dev, struct class_attribute *attr, char *buf)
{
	MHALL_LOG("enable_mhall_show\n");
	return sprintf(buf, "%d\n", mhall_flag);
}

static ssize_t enable_mhall_store(struct class *class, struct class_attribute *attr, const char *buf, size_t count)
{
	unsigned long state;

	MHALL_LOG("enable_mhall_store\n");
	return count;
}

static CLASS_ATTR(state, 0664, enable_mhall_show, enable_mhall_store);
static int mhall_open(struct inode *inode, struct file *file)
{
	return 0; 
}

static int mhall_release(struct inode *inode, struct file *file)
{
	return 0;
}

static long mhall_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	MHALL_LOG("mhall_ioctl\n");
    switch (cmd) {
		case MHALL_IOCTL_GET_STATUS:              
			if (copy_to_user((void __user *)arg, &mhall_flag, sizeof(mhall_flag))) {
				MHALL_LOG("mhall_ioctl: copy_to_user failed\n");
				return -EFAULT;
			}
		break;
		default:
		break;
	}
	return 0;
}

static struct file_operations mhall_fops = {
	.owner = THIS_MODULE,
	.open = mhall_open,
	.release = mhall_release,
	.unlocked_ioctl = mhall_ioctl,
};

static struct miscdevice mhall_misc_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "mhall",
	.fops = &mhall_fops,
};

static ssize_t mhall_show_status(struct device* dev, struct device_attribute *attr, char *buf)
{
	ssize_t res;

	res = snprintf(buf, PAGE_SIZE, "%d\n", mhall_flag);
	return res;    
}

static DEVICE_ATTR(status, S_IWUSR | S_IRUGO, mhall_show_status, NULL);

static irqreturn_t mhall_eint_interrupt_handler(int irq, void *dev_id)
{
	MHALL_LOG("mhall eint interrupt\n");
	disable_irq_nosync(mhall_number);
    	atomic_set(&mhall_wakeup_flag, 1);
	wake_up_interruptible(&mhall_thread_wq);
	return IRQ_HANDLED;
}

static int mhall_thread_kthread(void *x)
{
	while(1)
	{
		unsigned int mhall_value;

		wait_event_interruptible(mhall_thread_wq, atomic_read(&mhall_wakeup_flag));
		atomic_set(&mhall_wakeup_flag, 0);
		
	       mhall_value = gpio_get_value(mhall_pin);
		mdelay(50);
		MHALL_LOG("gpio value=%d\n", mhall_value);
		if(mhall_value == mhall_cover_open_gpio_value) {
//			switch_set_state((struct switch_dev *)&switch_hall, 0);
			input_report_key(mhall_input_dev, KEY_MHALL_COVER_OPEN, 1);
			input_sync(mhall_input_dev);
			mdelay(10);
			input_report_key(mhall_input_dev, KEY_MHALL_COVER_OPEN, 0);
			input_sync(mhall_input_dev);
	              //irq_set_irq_type(mhall_number,IRQ_TYPE_EDGE_FALLING);
			mdelay(10);
			enable_irq(mhall_number);
	
			mhall_flag = 1;
			//add by xuhongming for holl sense start
			#ifdef RGK_TOUCHPANEL_HALL_MODE
			g_tpd_opencover_fun();
			#endif
			//add by xuhongming for holl sense end
			switch_set_state((struct switch_dev *)&switch_hall, 0);
			MHALL_LOG("cover open\n");
		} else {
//			switch_set_state((struct switch_dev *)&switch_hall, 48);
			input_report_key(mhall_input_dev, KEY_MHALL_COVER_CLOSE, 1);
			input_sync(mhall_input_dev);
			mdelay(10);
			input_report_key(mhall_input_dev, KEY_MHALL_COVER_CLOSE, 0);
			input_sync(mhall_input_dev);

			//irq_set_irq_type(mhall_number,IRQ_TYPE_EDGE_RISING);
			mdelay(10);
			enable_irq(mhall_number);
			mhall_flag = 0;
			//add by xuhongming for holl sense start
			#ifdef RGK_TOUCHPANEL_HALL_MODE
			g_tpd_opencover_fun();
			#endif
			//add by xuhongming for holl sense end
	switch_set_state((struct switch_dev *)&switch_hall, 48);
			MHALL_LOG("cover close\n");
		}
		
		//switch_set_state((struct switch_dev *)&switch_hall, mhall_flag);
	}
	return 0;
}


static int mhall_probe(struct platform_device *dev)
{
	int ret;
	int pin;
	rgk_mhall = dev;
	mhall_cover_open_gpio_value = 1;
// set gpio state  start
 	mhallctl = devm_pinctrl_get(&rgk_mhall->dev);
	if (IS_ERR(mhallctl)) {
		MHALL_LOG("Cannot find mhall pinctrl!");
		ret = PTR_ERR(mhallctl);
		return ret;
	}
	mhall_int = pinctrl_lookup_state(mhallctl, "mhall_irq_init");
	if (IS_ERR(mhall_int)) {
		ret = PTR_ERR(mhall_int);
		MHALL_LOG("%s : pinctrl err, flash_en0\n", __func__);
		return ret;
	}

	pinctrl_select_state(mhallctl, mhall_int);
//set gpio state end
	MHALL_LOG("detecetd hall success\n");

	mhall_input_dev = input_allocate_device();
	if (!mhall_input_dev) {
		MHALL_LOG("alloc input device failed\n");
		return -ENOMEM;
	}
	
	set_bit(EV_KEY, mhall_input_dev->evbit);
	set_bit(KEY_MHALL_COVER_OPEN, mhall_input_dev->keybit);
	set_bit(KEY_MHALL_COVER_CLOSE, mhall_input_dev->keybit);
	
	mhall_input_dev->id.bustype = BUS_HOST;
	mhall_input_dev->name = MHALL_NAME;
	if(ret = input_register_device(mhall_input_dev)) {
		MHALL_LOG("input device register failed\n");
		return ret;
	}
	MHALL_LOG("input register success\n");

	if(ret = misc_register(&mhall_misc_device)) {
		MHALL_LOG("misc device register failed\n");
		return ret;
	}
	MHALL_LOG("misc device register success\n");
	
	if(ret = device_create_file(mhall_misc_device.this_device,&dev_attr_status)) {
		MHALL_LOG("dev attr status register failed\n");
		return ret;
	}
	mhall_class = class_create(THIS_MODULE, "hall");
	ret = class_create_file(mhall_class, &class_attr_state);
#if 1	
	switch_hall.name = "hall";
	switch_hall.index = 0;
	switch_hall.state = 0;
	ret = switch_dev_register(&switch_hall);
	if(ret)
	{
		MHALL_LOG("switch attr status register filed\n");
	}
	else
	{
		MHALL_LOG("switch attr status register success\n");
	}
#endif    
	if (IS_ERR(kthread_run(mhall_thread_kthread, NULL, "mhall_thread_kthread"))) {
		MHALL_LOG("kthread create failed\n");
		return -1;
	}
	MHALL_LOG("kthread create success\n");


	mhall_node = of_find_compatible_node(NULL, NULL, "mediatek,MHALL-eint");
	if (mhall_node == NULL) {
		pr_err("mhall - get USB0 mhall failed\n");
	} 
	of_property_read_u32_array(mhall_node, "debounce", ints, ARRAY_SIZE(ints));
	mhall_pin = ints[0];
	MHALL_LOG("mhall_pin = %d\n",mhall_pin);
	gpio_set_debounce(ints[0], ints[1]);
	mhall_number = irq_of_parse_and_map(mhall_node, 0);
	if(mhall_number > 0)
	{
		MHALL_LOG("mhall_number = %d\n",mhall_number);
	}
	MHALL_LOG("mhall ready to request irq\n");
	pin = gpio_get_value(mhall_pin);	
		MHALL_LOG("pin = %d\n",pin);
	
	ret = request_irq(mhall_number, mhall_eint_interrupt_handler, IRQ_TYPE_EDGE_BOTH, "MHALL",NULL);
	if (ret > 0)
		MHALL_LOG("Mhall IRQ LINE not available!!\n");
	else
		MHALL_LOG("Mhall IRQ LINE available!!\n");
  	enable_irq(mhall_number);

	
	MHALL_LOG("setup eint success\n");

	ret = gpio_get_value(mhall_pin);
	if(ret == mhall_cover_open_gpio_value) {
		mhall_flag = 1;
		MHALL_LOG("cover open\n");
	} else {
		mhall_flag = 0;
		MHALL_LOG("cover close\n");
	}

	return 0;
}
static int mhall_remove(struct platform_device *dev)
{
	 int err;
	 input_unregister_device(mhall_input_dev);
	 input_free_device(mhall_input_dev);
	if((err = misc_deregister(&mhall_misc_device)))
	{
		MHALL_LOG("misc_deregister fail: %d\n", err);
	}
	
	disable_irq(mhall_number);
}
static void mhall_shutdown(struct platform_device *dev)
{
}
static  struct platform_driver mhall_platform_driver = {
	.probe = mhall_probe,
	.remove = mhall_remove,
	.shutdown = mhall_shutdown,
	.driver = {
	.name = MHALL_DEVNAME,
	.owner = THIS_MODULE,
#ifdef CONFIG_OF
	.of_match_table = mhall_of_match,
#endif
	 },
};
static struct platform_device mhall_platform_device = {
	.name = MHALL_DEVNAME,
	.id = 0,
	.dev = {},
};
static int __init mhall_init(void)
{
	int ret;
	ret = platform_driver_register(&mhall_platform_driver);
	if(ret < 0)
	{	
		MHALL_LOG("mhall_platform_driver register error\n");
		return ret;
	}
//	ret = platform_device_register(&mhall_platform_device);
	if(ret < 0)
	{	
		MHALL_LOG("mhall_platform_device register error\n");
		return ret;
	}
	return 0;
}
static void __exit mhall_exit(void)
{
	platform_driver_unregister(&mhall_platform_driver);
	platform_device_unregister(&mhall_platform_device);
	return ;
}

module_init(mhall_init);
module_exit(mhall_exit);

MODULE_AUTHOR("wanwei.jiang@ragentek.com");
MODULE_DESCRIPTION("mhall driver for level sense chip");
MODULE_LICENSE("GPL");
MODULE_ALIAS("mhall sense level");
