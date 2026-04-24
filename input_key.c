#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/gpio/consumer.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/timer.h>
#include <linux/jiffies.h>

struct input_key_dev{
	struct device* dev;//方便打印日志
	struct gpio_desc* key_gpio;//gpio描述符
	int irq;//中断号
	struct input_dev* input_dev;//输入设备
	struct timer_list  timer;//定时器消抖
	int last_state; //记录上一次稳定状态
};

//定时器回调函数
static void input_key_timer_func(struct timer_list* t)
{
	struct input_key_dev* key_dev;
	int value;

	//由成员找到结构体，类似于container_of宏
	key_dev = from_timer(key_dev, t, timer);

	//读取按键稳定后的逻辑值
	value = gpiod_get_value(key_dev->key_gpio);

	if(value != key_dev->last_state)
	{
		//上报给input子系统
		input_report_key(key_dev->input_dev, KEY_0, value);
		input_sync(key_dev->input_dev);
		
		key_dev->last_state = value;//更新state,避免重复上报

		dev_info(key_dev->dev, "按键值已上报：%d\n",value);
	}
}

//中断处理函数
static irqreturn_t input_key_irq_handler(int irq, void* dev_id)
{
	struct input_key_dev* key_dev = dev_id;//进入中断拿到私有结构体
	
	//中断处理函数只开启定时器，其余逻辑在定时器回调函数中实现
	mod_timer(&key_dev->timer, jiffies + msecs_to_jiffies(20));//计时20ms，重复启动定时器会刷新计时时间

	return IRQ_HANDLED;
}

static int input_key_probe(struct platform_device* pdev)
{
	struct device* dev = &pdev->dev;//方便资源获取和打印日志
	struct input_key_dev* key_dev;//先声明私有结构体指针，后面分配内存
	int ret; //返回值

	//打印日志，表示开始执行probe
	dev_info(dev,"进入probe\n");

	//为私有结构体分配内存
	key_dev = devm_kmalloc(dev,sizeof(*key_dev),GFP_KERNEL);
	if(!key_dev) //如果key_dev为NULL就返回-ENOMEM
		return -ENOMEM;

	//初始化私有结构体里面的dev
	key_dev->dev = dev;

	platform_set_drvdata(pdev,key_dev);

	key_dev->key_gpio = devm_gpiod_get(dev,"key",GPIOD_IN);
	if(IS_ERR(key_dev->key_gpio))
	{
		ret = PTR_ERR(key_dev->key_gpio);
		dev_err(dev,"获取GPIO失败，错误码：%d\n",ret);
		return ret;
	}

	//将gpio描述符转化为中断号
	key_dev->irq = gpiod_to_irq(key_dev->key_gpio);
	if(key_dev->irq < 0)
	{
		dev_err(dev, "获取中断号失败:%d\n",key_dev->irq);
		return key_dev->irq;
	}

	//初始化定时器timer
	timer_setup(&key_dev->timer, input_key_timer_func, 0);
	key_dev->last_state = gpiod_get_value(key_dev->key_gpio);//记录一下初始电平
	dev_info(dev,"初始GPIO电平(是否有效)：%d\n", key_dev->last_state);

	//初始化input设备
	key_dev->input_dev = devm_input_allocate_device(dev);
	if(!key_dev->input_dev)
	{
		dev_err(dev,"申请输入设备失败\n");
		return -ENOMEM;
	}
	
	//设置input设备的基本信息
	key_dev->input_dev->name = "xlp-input-key";
	key_dev->input_dev->phys = "xlp/input-key0";
	key_dev->input_dev->dev.parent = dev;

	//这个设备支持EV_KEY按键类事件，并且支持KEY_0这个按键
	input_set_capability(key_dev->input_dev, EV_KEY, KEY_0);

	//把这个input设备注册到input core
	ret = input_register_device(key_dev->input_dev);
	if(ret)
	{
		dev_err(dev, "注册输入设备失败\n");
		return ret;
	}

	dev_info(dev, "输入设备注册成功\n");

	//拿到中断号之后申请中断,双边沿触发,中断名称为设备树节点名
	//一旦申请中断，中断就有可能被触发，因此尽量把申请中断放在最靠后的位置，先将其他资源初始化
	ret = devm_request_irq(dev, key_dev->irq, input_key_irq_handler, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, dev_name(dev), key_dev);
	if(ret)
	{
		dev_err(dev, "申请中断失败，中断号：%d\n",key_dev->irq);
		return ret;
	}
	dev_info(dev,"中断号的值：%d\n",key_dev->irq);

	dev_info(dev, "probe成功\n");
	return 0;
}

static int input_key_remove(struct platform_device* pdev)
{

	struct input_key_dev* key_dev = platform_get_drvdata(pdev);
	struct device* dev = &pdev->dev;

	dev_info(dev, "准备remove\n");

	//一定要删除定时器，防止模块已经卸载，定时器还在计时
	//如果定时器回调函数正在运行，那就等他运行完在删除
	del_timer_sync(&key_dev->timer);

	dev_info(dev,"remove成功\n");

	return 0;
}

static const struct of_device_id input_key_match[] = {
	{.compatible = "xlp,input-key"},
	{}
};
MODULE_DEVICE_TABLE(of,input_key_match);

static struct platform_driver input_key_driver = {
	.probe = input_key_probe,
	.remove = input_key_remove,
	.driver = {
		.name = "xlp-input-key",
		.of_match_table = input_key_match,
	},
};
module_platform_driver(input_key_driver);

MODULE_LICENSE("GPL");
