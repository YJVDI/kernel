#if  0
#include <dt-bindings/gpio/gpio.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/ioctl.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/wait.h>
#include <linux/types.h>
#include <linux/proc_fs.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/err.h> 
#include <linux/workqueue.h> //添加需要用到的头文件

static int gpio_led; //定义一个GPIO变量，用gpio_led表示
static int gpio_led_wd; //定义一个GPIO变量，用gpio_led表示
static int rb_switch_value = 0;//定义一个GPIO电平变量值

static struct delayed_work gpioled_event; //定义一个delay_work

static void gpioled_event_workq(struct work_struct *work)  //定义你要延时执行的工作队列函数
{

	//printk("#%s#: led color blue or red,rb_switch_value=%d\n", __func__, rb_switch_value);
	gpio_direction_output(gpio_led, rb_switch_value); //设置GPIO电平
	rb_switch_value = !rb_switch_value; //设置rb_switch_value取反,以此实现高低电平切换设置
	schedule_delayed_work(&gpioled_event, msecs_to_jiffies(1000)); //添加这句之后每隔1秒执行一次
}

static ssize_t gpio_led_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", gpio_get_value(gpio_led)); //获取GPIO的电平，1为高电平，0为低电平
}


static ssize_t gpio_led_store(struct device *dev, struct device_attribute *attr,
						const char *buf, size_t size)
{
	int val;
	int ret;

	ret = kstrtoint(buf, 10, &val); //输入一个字符然后转换成10进制整形数

        if (ret) {
		printk("%s: kstrtoint error return %d\n", __func__, ret);
		return -1;
        }

	if (val== 1) { //如果echo 1 到节点则调用

		printk("-czd-: _%s_ :gpio pull up\n", __func__);
		gpio_direction_output(gpio_led, val); //设置GPIO电平为高电平

	} else if (val == 0) { //如果echo 0 到节点则调用

		printk("-czd-: _%s_ :gpio pull down\n", __func__);
		gpio_direction_output(gpio_led, val); //设置GPIO电平为低电平

	} else {

		printk("I only support 0 or 1 to ctrl led\n");

	}
	return size;
}

static DEVICE_ATTR(gpio_led, 0664, gpio_led_show, gpio_led_store); //调用DEVICE_ATTR创建设备节点

static int led_ctrl_probe(struct platform_device *pdev) //compatible的属性和dts的compatible一致，就会调用probe函数
{

	struct device_node *led_ctrl_node = pdev->dev.of_node;
	enum of_gpio_flags flags;
	int gpio_value;

	enum of_gpio_flags flags_wd;
	int gpio_value_wd;

	printk("[%d] enter %s start..\n", __LINE__, __func__); //printk打印，kernel一般调用这个函数打印log

	gpio_led = of_get_named_gpio_flags(led_ctrl_node, "gpio_led", 0, &flags); //解析dts的gpio
    gpio_led_wd = of_get_named_gpio_flags(led_ctrl_node, "gpio_led_wd", 0, &flags_wd); //解析dts的gpio
	printk("gpio_led is %d --\n", gpio_led);

	gpio_value = (flags == GPIO_ACTIVE_HIGH)? 1:0;

	if (!gpio_is_valid(gpio_led)) { //判断GPIO是否合法能用
		printk("gpio_led: %d is invalid\n", gpio_led);
		return -ENODEV;
	}

	if (gpio_request(gpio_led, "gpio_led")) { //申请GPIO口资源
		printk("gpio_led: %d request failed!\n", gpio_led);
		gpio_free(gpio_led); //如果申请失败，要释放GPIO的占用
		return -ENODEV;
	}

	 gpio_value_wd = (flags_wd == GPIO_ACTIVE_HIGH)? 1:0;

	if (!gpio_is_valid(gpio_led_wd)) { //判断GPIO是否合法能用
		printk("gpio_led_wd: %d is invalid\n", gpio_value_wd);
		return -ENODEV;
	}

	if (gpio_request(gpio_led_wd, "gpio_led_wd")) { //申请GPIO口资源
		printk("gpio_led_wd: %d request failed!\n", gpio_value_wd);
		gpio_free(gpio_led_wd); //如果申请失败，要释放GPIO的占用
		return -ENODEV;
	}

   

	gpio_direction_output(gpio_led, gpio_value); //设置GPIO初始电平为低电平
	printk("gpio_led pin level is %d\n", gpio_value); //这里gpio_value取反!gpio_value，是低电平


	gpio_direction_output(gpio_led_wd, gpio_value_wd); //设置GPIO初始电平为低电平
	printk("gpio_led_wd pin level is %d\n", gpio_value_wd); //这里gpio_value取反!gpio_value，是低电平


	device_create_file(&pdev->dev, &dev_attr_gpio_led);

	INIT_DELAYED_WORK(&gpioled_event, gpioled_event_workq); //初始化工作队列
	schedule_delayed_work(&gpioled_event, msecs_to_jiffies(2000)); //添加到延时工作队列，这里延时2秒

	printk("[%d]: ___%s___ sucess!\n", __LINE__, __func__);

	return 0;
}

static int led_ctrl_remove(struct platform_device *pdv)
{
	printk("___%s___\n", __func__);
	return 0;
}

static struct of_device_id led_ctrl_match_table[] = {
	{ .compatible = "led_ctrl",},
	{},
	};

static struct platform_driver led_ctrl_driver = {

	.driver = {
		.name = "led_ctrl",
		.owner = THIS_MODULE,
		.of_match_table = led_ctrl_match_table,
	},
	.probe = led_ctrl_probe,
	.remove = led_ctrl_remove,
	};

static int led_ctrl_init(void)
{
	printk("#led_ctrl#: ___%s___\n", __func__);
	platform_driver_register(&led_ctrl_driver);
	return 0;
}

static void led_ctrl_exit(void)
{
	printk("#led_ctrl#: ___%s___,\n", __func__);
	cancel_delayed_work_sync(&gpioled_event);  //取消延时工作队列
	platform_driver_unregister(&led_ctrl_driver);

}

module_init(led_ctrl_init); //模块加载函数
module_exit(led_ctrl_exit); //模块卸载函数

MODULE_AUTHOR("czd,214241976@qq.com");
MODULE_DESCRIPTION("Driver for control sysled");
MODULE_LICENSE("GPL");  //许可声明, 描述内核模块的许可权限，如果不声明LICENSE，模块被加载时，将收到kernel tainted的警告

#else
#include <dt-bindings/gpio/gpio.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/ioctl.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/wait.h>
#include <linux/types.h>
#include <linux/proc_fs.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/err.h> 


static int gpio_led; //定义一个GPIO变量，用gpio_led表示
static int gpio_led_wd; //定义一个GPIO变量，用gpio_led表示

static ssize_t gpio_led_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", gpio_get_value(gpio_led)); //获取GPIO的电平，1为高电平，0为低电平
}


static ssize_t gpio_led_store(struct device *dev, struct device_attribute *attr,
						const char *buf, size_t size)
{
	int val;
	int ret;

	ret = kstrtoint(buf, 10, &val); //输入一个字符然后转换成10进制整形数

        if (ret) {
		printk("%s: kstrtoint error return %d\n", __func__, ret);
		return -1;
        }

	if (val== 1) { //如果echo 1 到节点则调用

		printk("-czd-: _%s_ :gpio pull up\n", __func__);
		gpio_direction_output(gpio_led, val); //设置GPIO电平为高电平

	} else if (val == 0) { //如果echo 0 到节点则调用

		printk("-czd-: _%s_ :gpio pull down\n", __func__);
		gpio_direction_output(gpio_led, val); //设置GPIO电平为低电平

	} else {
		printk("I only support 0 or 1 to ctrl led\n");

	}
	return size;
}

static DEVICE_ATTR(gpio_led, 0664, gpio_led_show, gpio_led_store); //调用DEVICE_ATTR创建设备节点


static int led_ctrl_probe(struct platform_device *pdev) //compatible的属性和dts的compatible一致，就会调用probe函数
{

	struct device_node *led_ctrl_node = pdev->dev.of_node;
	enum of_gpio_flags flags;
	int gpio_value;
	enum of_gpio_flags flagswd;
	int gpio_valuewd;

	printk("[%d] enter %s start..\n", __LINE__, __func__); //printk打印，kernel一般调用这个函数打印log

	gpio_led = of_get_named_gpio_flags(led_ctrl_node, "gpio_led", 0, &flags); //解析dts的gpio
	gpio_led_wd=of_get_named_gpio_flags(led_ctrl_node, "gpio_led_wd", 0, &flagswd); //解析dts的gpio
	printk("gpio_led is %d --\n", gpio_led);
    printk("gpio_led_wd is %d --\n", gpio_led_wd);


	gpio_value = (flags == GPIO_ACTIVE_HIGH)? 1:0;
	if (!gpio_is_valid(gpio_led)) { //判断GPIO是否合法能用
		printk("gpio_led: %d is invalid\n", gpio_led);
		return -ENODEV;
	}

    gpio_valuewd = (flagswd == GPIO_ACTIVE_HIGH)? 1:0;
	if (!gpio_is_valid(gpio_led_wd)) { //判断GPIO是否合法能用
		printk("gpio_led_wd: %d is invalid\n", gpio_led);
		return -ENODEV;
	}


	if (gpio_request(gpio_led, "gpio_led")) { //申请GPIO口资源
		printk("gpio_led: %d request failed!\n", gpio_led);
		gpio_free(gpio_led); //如果申请失败，要释放GPIO的占用
		return -ENODEV;
	}
	if (gpio_request(gpio_led_wd, "gpio_led_wd")) { //申请GPIO口资源
		printk("gpio_ledwd: %d request failed!\n", gpio_led_wd);
		gpio_free(gpio_led_wd); //如果申请失败，要释放GPIO的占用
		return -ENODEV;
	}

   //while(1)
  // {
        gpio_direction_output(gpio_led, gpio_value); //设置GPIO初始电平为低电平
	    printk("gpio_led pin level is %d\n", gpio_value); //这里gpio_value取反!gpio_value，是低电平
        gpio_direction_output(gpio_led_wd, gpio_valuewd); //设置GPIO初始电平为低电平
		printk("gpio_led_wd pin level is %d\n", gpio_valuewd); //这里gpio_value取反!gpio_value，是低电平
		//mdelay(2000);
        //gpio_direction_output(gpio_led, !gpio_value); //设置GPIO初始电平为低电平
	    //printk("gpio_led pin level is %d\n", gpio_value); //这里gpio_value取反!gpio_value，是低电平
        //gpio_direction_output(gpio_led_wd, !gpio_valuewd); //设置GPIO初始电平为低电平
		//mdelay(2000);
  // }	

	device_create_file(&pdev->dev, &dev_attr_gpio_led);

	printk("[%d]: ___%s___ sucess!\n", __LINE__, __func__);

	return 0;
}

static int led_ctrl_remove(struct platform_device *pdv)
{
	printk("___%s___\n", __func__);
	return 0;
}

static struct of_device_id led_ctrl_match_table[] = {
	{ .compatible = "led_ctrl",},
	{},
	};

static struct platform_driver led_ctrl_driver = {

	.driver = {
		.name = "led_ctrl",
		.owner = THIS_MODULE,
		.of_match_table = led_ctrl_match_table,
	},
	.probe = led_ctrl_probe,
	.remove = led_ctrl_remove,
	};

static int led_ctrl_init(void)
{
	printk("#led_ctrl#: ___%s___\n", __func__);
	platform_driver_register(&led_ctrl_driver);
	return 0;
}

static void led_ctrl_exit(void)
{
	printk("#led_ctrl#: ___%s___,\n", __func__);
	platform_driver_unregister(&led_ctrl_driver);

}

module_init(led_ctrl_init); //模块加载函数
module_exit(led_ctrl_exit); //模块卸载函数

MODULE_AUTHOR("czd,214241976@qq.com");
MODULE_DESCRIPTION("Driver for control sysled");
MODULE_LICENSE("GPL");  //许可声明, 描述内核模块的许可权限，如果不声明LICENSE，模块被加载时，将收到kernel tainted的警告
#endif