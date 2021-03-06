# usb gadget usb host数据传输
-----------------------

### pc作为host：				使用libusb库提供的接口开发
android作为gadget：	在/kernel/driver/usb/gadget/android.c的基础上，添加function（即一个interface，包含两个endpoint）




```c
static int __init init(void)
{
	int ret;

	INIT_LIST_HEAD(&android_dev_list);
	android_dev_count = 0;

	ret = platform_driver_register(&android_platform_driver);
	...
}
```

在android.c中，初始化android_dev_list列表，现在只有一个dev，android_dev_count是对android_dev的记数。下面注册platform总线，关注android_platform_driver

```c
static struct platform_driver android_platform_driver = {
	.driver = {
		.name = "android_usb",
		.of_match_table = usb_android_dt_match,
	},
	.probe = android_probe,
	.remove = android_remove,
	.id_table = android_id_table,
};
```

platform总线注册，重点进android_probe函数

```c
static int android_probe(struct platform_device *pdev)
{
	struct android_usb_platform_data *pdata;
	...
	if (pdev->dev.of_node) {
		pdata = devm_kzalloc(&pdev->dev, sizeof(*pdata), GFP_KERNEL);
	...
		of_get_property(pdev->dev.of_node, "qcom,pm-qos-latency",
								&prop_len);
	...

	len = of_property_count_strings(pdev->dev.of_node,
			"qcom,supported-func");
	...
		android_class = class_create(THIS_MODULE, "android_usb");
	...
	android_dev = kzalloc(sizeof(*android_dev), GFP_KERNEL);
	android_dev->name = pdev->name;
	android_dev->disable_depth = 1;
	android_dev->functions = 
		supported_list ? supported_list : default_functions;
	android_dev->pdata = pdata;
	list_add_tail(&android_dev->list_item, &android_dev_list);
	...
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (res) {
		diag_dload = devm_ioremap(&pdev->dev, res->start,
							resource_size(res));
		if (!diag_dload) {
			dev_err(&pdev->dev, "ioremap failed\n");
			ret = -ENOMEM;
			goto err_dev;
		}
	}
	...
	if (pdata)
		android_usb_driver.gadget_driver.usb_core_id =
						pdata->usb_core_id;
	ret = android_create_device(android_dev,
			android_usb_driver.gadget_driver.usb_core_id);
	...
	ret = usb_composite_probe(&android_usb_driver);
	if (ret) {
		/* Perhaps UDC hasn't probed yet, try again later */
		if (ret == -ENODEV)
			ret = -EPROBE_DEFER;
		else
			pr_err("%s(): Failed to register android composite driver\n",
				__func__);
		goto err_probe;
	}

}
```

这个函数在注册platform总线时调用，很长，慢慢看。
1.首先为android_usb_platform_data申请内存，不深究了。
2.之后通过of_get_property，of_property_count_strings等读取dts的信息。
3.创建名为android_usb的sys文件，在/sys/class/android_usb 该节点下，会用来存放所有的usb信息,类似：android0代表第一个之前说的第一个android_dev,android0下又会存放该设备的所有信息，待会再看

```sh
nedplus:/sys/class/android_usb # ls
android0    f_audio        f_charging f_ecm_qc f_loopback     f_mtp f_qdss  f_rndis_qc  f_usb_mbim 
f_accessory f_audio_source f_diag     f_ffs    f_mass_storage f_ncm f_rmnet f_serial    f_video    
f_acm       f_ccid         f_ecm      f_gps    f_midi         f_ptp f_rndis f_uac2_func
```

4.回到probe函数，创建好android_usb后，需要初始化android_dev的信息，然后添加到android_dev_list列表中去
5.platform_get_resource获取io资源，以便初始化及使用
6.进入到android_create_device

```c
static int android_create_device(struct android_dev *dev, u8 usb_core_id)
{
	struct device_attribute **attrs = android_usb_attributes;
	struct device_attribute *attr;
	...
	dev->dev = device_create(android_class, NULL, MKDEV(0, usb_core_id),
		NULL, device_node_name);
	...
	while ((attr = *attrs++)) {
		err = device_create_file(dev->dev, attr);
	}
}
static struct device_attribute *android_usb_attributes[] = {
	&dev_attr_idVendor,
	&dev_attr_idProduct,
	&dev_attr_bcdDevice,
	&dev_attr_bDeviceClass,
	&dev_attr_bDeviceSubClass,
	&dev_attr_bDeviceProtocol,
	&dev_attr_iManufacturer,
	&dev_attr_iProduct,
	&dev_attr_iSerial,
	&dev_attr_functions,
	&dev_attr_enable,
	&dev_attr_pm_qos,
	&dev_attr_up_pm_qos_sample_sec,
	&dev_attr_down_pm_qos_sample_sec,
	&dev_attr_up_pm_qos_threshold,
	&dev_attr_down_pm_qos_threshold,
	&dev_attr_idle_pc_rpm_no_int_secs,
	&dev_attr_pm_qos_state,
	&dev_attr_state,
	&dev_attr_remote_wakeup,
	NULL
};
```

6.1先device_create创建android0节点，再在android0下根据android_usb_attributes创建其属性文件，以便和user交互

```sh
nedplus:/sys/class/android_usb/android0 # ls
bDeviceClass           f_audio_source f_midi      f_usb_mbim              pm_qos_state         
bDeviceProtocol        f_ccid         f_mtp       f_video                 power                
bDeviceSubClass        f_charging     f_ncm       functions               remote_wakeup        
bcdDevice              f_diag         f_ptp       iManufacturer           state                
down_pm_qos_sample_sec f_ecm          f_qdss      iProduct                subsystem            
down_pm_qos_threshold  f_ecm_qc       f_rmnet     iSerial                 uevent               
enable                 f_ffs          f_rndis     idProduct               up_pm_qos_sample_sec 
f_accessory            f_gps          f_rndis_qc  idVendor                up_pm_qos_threshold  
f_acm                  f_loopback     f_serial    idle_pc_rpm_no_int_secs 
f_audio                f_mass_storage f_uac2_func pm_qos 
```

7.回到probe函数，最后调用usb_composite_probe(&android_usb_driver)在注册usb驱动，又到了重点：android_usb_driver。看下usb_composite_probe函数的说明先，总体把握下

```c
/**
 * usb_composite_probe() - register a composite driver
 * @driver: the driver to register
 *
 * Context: single threaded during gadget setup
 *
 * This function is used to register drivers using the composite driver
 * framework.  The return value is zero, or a negative errno value.
 * Those values normally come from the driver's @bind method, which does
 * all the work of setting up the driver to match the hardware.
 *
 * On successful return, the gadget is ready to respond to requests from
 * the host, unless one of its components invokes usb_gadget_disconnect()
 * while it was binding.  That would usually be done in order to wait for
 * some userspace participation.
 */
```

继续跟进android_usb_driver

```c
static struct usb_composite_driver android_usb_driver = {
	.name		= "android_usb",
	.dev		= &device_desc,
	.strings	= dev_strings,
	.bind		= android_bind,
	.unbind		= android_usb_unbind,
	.disconnect	= android_disconnect,
	.max_speed	= USB_SPEED_SUPER
};
```

结构体很简单，主要是要实现里面的函数。dev代表usb_device_descriptor。关于描述符，可以参考http://www.cnblogs.com/tianchiyuyin/p/5139948.html。
再看usb_composite_probe函数介绍时了解到了他的bind函数很重要

```c
static int android_bind(struct usb_composite_dev *cdev)
{
	...
	/* Allocate string descriptor numbers ... note that string
	 * contents can be overridden by the composite_dev glue.
	 */
	id = usb_string_id(cdev);
	...
		ret = android_init_functions(dev->functions, cdev);
}
```

usb_string_id,应该是用来保存配置及interface信息的，无需深究。主要看android_init_functions

```c
static int android_init_functions(struct android_usb_function **functions,
				  struct usb_composite_dev *cdev)
{
	...
	for (; (f = *functions++); index++) {
		f->dev_name = kasprintf(GFP_KERNEL, "f_%s", f->name);
		f->android_dev = NULL;
		f->dev = device_create(android_class, dev->dev,
				MKDEV(0, index), f, f->dev_name);
		if (f->init) {
			err = f->init(f, cdev);
		}

		attrs = f->attributes;
		if (attrs) {
			while ((attr = *attrs++) && !err)
				err = device_create_file(f->dev, attr);
		}
	}
}
```

这里的functions，主要是probe函数中赋值的:android_dev->functions = supported_list ? supported_functions : default_functions;在android_usb创建f_开头的节点，如果函数声明了init函数则调用，并且在该节点下创建属性文件，让user来配置读取信息的老手段。
到了这里是不是感觉结束了，并没有，这个时候该请出我们的init.qcom.usb.rc文件了，关于init.rc,网上很多说明，直接问度娘

```sh
on property:sys.usb.config=mtp,diag,adb && property:sys.usb.configfs=0
    write /sys/class/android_usb/android0/enable 0
    write /sys/class/android_usb/android0/iSerial ${ro.serialno}
    write /sys/class/android_usb/android0/idVendor 05C6
    write /sys/class/android_usb/android0/idProduct 903A
    write /sys/class/android_usb/android0/f_diag/clients diag
    write /sys/class/android_usb/android0/functions mtp,diag,adb,loopback
    write /sys/class/android_usb/android0/enable  1
    start adbd
    setprop sys.usb.state ${sys.usb.config}
```

截取了其中的一段，当sys.usb.config属性被设置为mtp,diag,adb时候执行，这些设备节点是不是很熟悉，没错，就是刚刚在probe中调用android_create_device创建的android_usb_attributes，可以回头查看。我们一句一句的跟进

```c
static ssize_t enable_store(struct device *pdev, struct device_attribute *attr,
			    const char *buff, size_t size)
{
	struct android_dev *dev = dev_get_drvdata(pdev);
	...
	sscanf(buff, "%d", &enabled);
	if (enabled && !dev->enabled) {

		...

	} else if (!enabled && dev->enabled) {
		android_disable(dev);
		list_for_each_entry(conf, &dev->configs, list_item)
			list_for_each_entry(f_holder, &conf->enabled_functions,
						enabled_list) {
				if (f_holder->f->disable)
					f_holder->f->disable(f_holder->f);
			}
		dev->enabled = false;
	} 
	...
}
```

先往enable中写0,调用android_disable，从字面意思就可以看出是关闭usb_dev的功能，里面主要调用usb_gadget_disconnect。之后便是遍历每个config，循环遍历config下的functions(这个function可以简单理解为interface),最后调用每个被function的disable函数。ok，继续
往iSerial，idVendor，idProduct，中写值很简单，就是改变device_desc中的变量
往f_diag/clients写值，这个和配置function相关，关注functions属性节点

```c
static ssize_t
functions_store(struct device *pdev, struct device_attribute *attr,
			       const char *buff, size_t size)
{
	...
	/* Clear previous enabled list */
	list_for_each_entry(conf, &dev->configs, list_item) {
		while (conf->enabled_functions.next !=
				&conf->enabled_functions) {
			f_holder = list_entry(conf->enabled_functions.next,
					typeof(*f_holder),
					enabled_list);
			f_holder->f->android_dev = NULL;
			list_del(&f_holder->enabled_list);
			kfree(f_holder);
		}
		INIT_LIST_HEAD(&conf->enabled_functions);
	}
	...
	while (b) {
		...
		while (conf_str) {
			name = strsep(&conf_str, ",");
			is_ffs = 0;
			strlcpy(aliases, dev->ffs_aliases, sizeof(aliases));
			a = aliases;

			while (a) {
				char *alias = strsep(&a, ",");
				if (alias && !strcmp(name, alias)) {
					is_ffs = 1;
					break;
				}
			}

			if (is_ffs) {
				if (ffs_enabled)
					continue;
				err = android_enable_function(dev, conf, "ffs");
				if (err)
					pr_err("android_usb: Cannot enable ffs (%d)",
									err);
				else
					ffs_enabled = 1;
				continue;
			}

			if (!strcmp(name, "rndis") &&
				!strcmp(strim(rndis_transports), "BAM2BAM_IPA"))
				name = "rndis_qc";

			err = android_enable_function(dev, conf, name);
			if (err)
				pr_err("android_usb: Cannot enable '%s' (%d)",
							name, err);
		}
	}

	/* Free uneeded configurations if exists */
	while (curr_conf->next != &dev->configs) {
		conf = list_entry(curr_conf->next,
				  struct android_configuration, list_item);
		free_android_config(dev, conf);
	}

	mutex_unlock(&dev->mutex);

	return size;
}
```

Clear previous enabled list,清除之前的enable functions。解析传进的值，先判断是不是adb，是的话开启ffs函数。总之会调用到android_enable_function来打开需要开启的function

```c
static int android_enable_function(struct android_dev *dev,
				   struct android_configuration *conf,
				   char *name)
{
	struct android_usb_function **functions = dev->functions;
	struct android_usb_function_holder *f_holder;
	...
	while ((f = *functions++)) {
		if (!strcmp(name, f->name)) {
				...
				f_holder = kzalloc(sizeof(*f_holder),
						GFP_KERNEL);
				if (!f_holder) {
					pr_err("Failed to alloc f_holder\n");
					return -ENOMEM;
				}

				f->android_dev = dev;
				f_holder->f = f;
				list_add_tail(&f_holder->enabled_list,
					      &conf->enabled_functions);
				...
			}
		}
	}
	return -EINVAL;
}
```

根据name，和android_dev下的functions进行逐个对比直至找到那个function，做的很简单，就是申请f_holder并和android_dev以及找到的function绑定，添加到conf->enabled_functions中去一边在往enable中写1的时候使用

```c
static ssize_t enable_store(struct device *pdev, struct device_attribute *attr,
			    const char *buff, size_t size)
{
	...
	if (enabled && !dev->enabled) {
		...
		list_for_each_entry(conf, &dev->configs, list_item)
			list_for_each_entry(f_holder, &conf->enabled_functions,
						enabled_list) {
				if (f_holder->f->enable)
					f_holder->f->enable(f_holder->f);
					...
			}
		err = android_enable(dev);
		...
		dev->enabled = true;
	}
}
```

又回到了enable_store函数，也很简单，调用各个function下的enable函数就ok了,之后调用到android_enable，这个函数可比android_disable有意思多了

```c
static int android_enable(struct android_dev *dev)
{
	struct usb_composite_dev *cdev = dev->cdev;
	struct android_configuration *conf;
	...
	if (--dev->disable_depth == 0) {

		list_for_each_entry(conf, &dev->configs, list_item) {
			err = usb_add_config(cdev, &conf->usb_config,
						android_bind_config);
			...
			}
		}
	...
	return err;
}
```

感觉list_for_each_entry用的很多啊，遍历dev->configs链表，usb_add_config来为dev_desc添加usb_configuration。这边将函数android_bind_config作为参数传了进去。跟进usb_add_config

```c
int usb_add_config(struct usb_composite_dev *cdev,
		struct usb_configuration *config,
		int (*bind)(struct usb_configuration *))
{
	...
	status = bind(config);
	...
	return status;
}
```

调用了我们传进去的android_bind_config函数，go go go

```c
static int android_bind_config(struct usb_configuration *c)
{
	...
	ret = android_bind_enabled_functions(dev, c);
	...
	return 0;
}
```

继续...

```c
static int
android_bind_enabled_functions(struct android_dev *dev,
			       struct usb_configuration *c)
{
	struct android_usb_function_holder *f_holder;
	struct android_configuration *conf =
		container_of(c, struct android_configuration, usb_config);
	int ret;

	list_for_each_entry(f_holder, &conf->enabled_functions, enabled_list) {
		ret = f_holder->f->bind_config(f_holder->f, c);
		if (ret) {
			pr_err("%s: %s failed\n", __func__, f_holder->f->name);
			while (!list_empty(&c->functions)) {
				struct usb_function		*f;

				f = list_first_entry(&c->functions,
					struct usb_function, list);
				if (f->config) {
					list_del(&f->list);
					if (f->unbind)
						f->unbind(c, f);
				}`这里写代码片`
			}
			if (c->unbind)
				c->unbind(c);
			return ret;
		}
		f_holder->f->bound = true;
	}
	return 0;
}
```

函数不是很长，重点在链表的遍历。遍历conf->enabled_functions，调用各个function下的bind_config函数
至此，我们调用了dev functions下的init，enable，bind_config 整个function的使能工作就完成了，但是一个function的各个函数怎么定义呢，这个时候就出现了android_usb_function结构体了，ok定义一个简单的function来实验一下

```c
static struct android_usb_function loopback_function = {
	.name		= "loopback",
	//.init		= loopback_function_init,
	//.enable		= loopback_function_enable,
	//.disable	= loopback_function_disable,
	//.cleanup	= loopback_function_cleanup,
	.bind_config	= loopback_function_bind_config,
	//.attributes	= ffs_function_attributes,
};
```

我们定义了一个android_usb_function名叫loopback_function，对结构体赋值，名字叫loopback，回忆一下，之前往/sys/class/android_usb/android0/functions写使能的function时需要与其对比。之后主要实现bind_config函数
我在参考了别的function后写了一个简单的，来看一下

```c
static int loopback_function_bind_config(struct android_usb_function *f,
				    struct usb_configuration *c)
{
	int ret;
	struct functionfs_config *config = 
			kzalloc(sizeof(struct functionfs_config), GFP_KERNEL);
	if (!config){
		pr_err("[LOL] loopback_function_bind_config kzalloc failed\n");
			return -ENOMEM;
	}
	f->config = config;
		//config = f->config;
	config->fi = loopback_alloc_instance();
	if (IS_ERR(config->fi)){
		pr_err("[LOL] loopback_function_bind_config usb_get_function_instance failed\n");
		return PTR_ERR(config->fi);
	}

	config->func = loopback_alloc(config->fi);
	if (IS_ERR(config->func)){
		pr_err("[LOL] loopback_function_bind_config usb_get_function failed\n");
		return PTR_ERR(config->func);
	}

	ret = usb_add_function(c, config->func);
	if (ret) {
		pr_err("%s(): usb_add_function() fails (err:%d) for ffs\n",
							__func__, ret);

		usb_put_function(config->func);
		config->func = NULL;
	}

	return ret;
}
```

首先为我们的functions_config申请内存，并绑定到android_usb_function上去，调用loopback_alloc_instance获取usb_function_instance，根据usb_function_instance获取usb_function，得到了一个usb_function结构体。用usb_add_function(c, config->func);添加到usb_configuration中去，usb_add_function是composite.c实现的函数我们不要关心，我们要关心的是我们得到usb_function是什么样的，我们如果要修改其功能需要怎么做呢？上面的loopback_alloc函数是在kernel/drivers/usb/gadget/function/loopback.c中，源码可以访问https://github.com/torvalds/linux/blob/master/drivers/usb/gadget/function/f_loopback.c 
f_loopback.c中用来获取usb_function

```c
struct usb_function *loopback_alloc(struct usb_function_instance *fi)
{
	struct f_loopback	*loop;
	struct f_lb_opts	*lb_opts;

	loop = kzalloc(sizeof *loop, GFP_KERNEL);
	if (!loop)
		return ERR_PTR(-ENOMEM);

	lb_opts = container_of(fi, struct f_lb_opts, func_inst);

	mutex_lock(&lb_opts->lock);
	lb_opts->refcnt++;
	mutex_unlock(&lb_opts->lock);

	buflen = lb_opts->bulk_buflen;
	qlen = lb_opts->qlen;
	if (!qlen)
		qlen = 32;

	loop->function.name = "loopback";
	loop->function.bind = loopback_bind;
	loop->function.set_alt = loopback_set_alt;
	loop->function.disable = loopback_disable;
	loop->function.strings = loopback_strings;

	loop->function.free_func = lb_free_func;

	return &loop->function;
}
```

主要看bind，set_alt，这两个函数已经被我修改过了，变得更简单的，源码请访问https://github.com/torvalds/linux/blob/master

```c
static int loopback_bind(struct usb_configuration *c, struct usb_function *f)
{
	struct usb_composite_dev *cdev = c->cdev;
	struct f_loopback	*loop = func_to_loop(f);
	int			id;
	int ret;
	/* allocate interface ID(s) */
	id = usb_interface_id(c, f);
	if (id < 0)
		return id;
	loopback_intf.bInterfaceNumber = id;

	id = usb_string_id(cdev);
	if (id < 0)
		return id;
	strings_loopback[0].id = id;
	loopback_intf.iInterface = id;

	/* allocate endpoints */

	loop->in_ep = usb_ep_autoconfig(cdev->gadget, &fs_loop_source_desc);
	if (!loop->in_ep) {
autoconf_fail:
		ERROR(cdev, "%s: can't autoconfigure on %s\n",
			f->name, cdev->gadget->name);
		return -ENODEV;
	}
	loop->in_ep->driver_data = cdev;	/* claim */

	loop->out_ep = usb_ep_autoconfig(cdev->gadget, &fs_loop_sink_desc);
	if (!loop->out_ep)
		goto autoconf_fail;
	loop->out_ep->driver_data = cdev;	/* claim */

	/* support high speed hardware */
	hs_loop_source_desc.bEndpointAddress =
		fs_loop_source_desc.bEndpointAddress;
	hs_loop_sink_desc.bEndpointAddress = fs_loop_sink_desc.bEndpointAddress;

	/* support super speed hardware */
	ss_loop_source_desc.bEndpointAddress =
		fs_loop_source_desc.bEndpointAddress;
	ss_loop_sink_desc.bEndpointAddress = fs_loop_sink_desc.bEndpointAddress;

	ret = usb_assign_descriptors(f, fs_loopback_descs, hs_loopback_descs,
			ss_loopback_descs);
	if (ret)
		return ret;
	DBG(cdev, "%s speed %s: IN/%s, OUT/%s\n",
	    (gadget_is_superspeed(c->cdev->gadget) ? "super" :
	     (gadget_is_dualspeed(c->cdev->gadget) ? "dual" : "full")),
			f->name, loop->in_ep->name, loop->out_ep->name);
	return 0;
}
```

先allocate interface ID，这个是必须的为该function申请一个interface编号，之后便是使用usb_ep_autoconfig申请了两个端点，一个输出一个输入。为每个端点的driver_data赋值是必要的

```c
/**
 * usb_ep_autoconfig() - choose an endpoint matching the
 * descriptor
 * @gadget: The device to which the endpoint must belong.
 * @desc: Endpoint descriptor, with endpoint direction and transfer mode
 *	initialized.  For periodic transfers, the maximum packet
 *	size must also be initialized.  This is modified on success.
 *
 * By choosing an endpoint to use with the specified descriptor, this
 * routine simplifies writing gadget drivers that work with multiple
 * USB device controllers.  The endpoint would be passed later to
 * usb_ep_enable(), along with some descriptor.
 *
 * That second descriptor won't always be the same as the first one.
 * For example, isochronous endpoints can be autoconfigured for high
 * bandwidth, and then used in several lower bandwidth altsettings.
 * Also, high and full speed descriptors will be different.
 *
 * Be sure to examine and test the results of autoconfiguration on your
 * hardware.  This code may not make the best choices about how to use the
 * USB controller, and it can't know all the restrictions that may apply.
 * Some combinations of driver and hardware won't be able to autoconfigure.
 *
 * On success, this returns an un-claimed usb_ep, and modifies the endpoint
 * descriptor bEndpointAddress.  For bulk endpoints, the wMaxPacket value
 * is initialized as if the endpoint were used at full speed.  To prevent
 * the endpoint from being returned by a later autoconfig call, claim it
 * by assigning ep->driver_data to some non-null value.
 *
 * On failure, this returns a null endpoint descriptor.
 */
```

成功返回un-claimed usb_ep，To prevent the endpoint from being returned by a later autoconfig call, claim it by assigning ep->driver_data to some non-null value.
上面还说需要usb_ep_enable()进行时能别急，我们在进入loopback_set_alt

```c
static int loopback_set_alt(struct usb_function *f,
		unsigned intf, unsigned alt)
{
	
	struct f_loopback	*loop = func_to_loop(f);
	struct usb_composite_dev *cdev = f->config->cdev;

	/* we know alt is zero */
	if (loop->in_ep->driver_data){
		disable_loopback(loop);
	}
	return enable_loopback(cdev, loop);
}
```

进入enable_loopback

```c
static int
enable_loopback(struct usb_composite_dev *cdev, struct f_loopback *loop)
{
	int					result = 0;

	pr_err("[LOL]enable_loopback!!!\n");
	
	result = enable_endpoint(cdev, loop, loop->in_ep);
	if (result)
		goto out;

	result = enable_endpoint(cdev, loop, loop->out_ep);
	if (result)
		goto disable_in;

	result = alloc_requests(cdev, loop);
	if (result)
		goto disable_out;
	
	pr_err( "[LOL]%s enabled\n", loop->function.name);
	DBG(cdev, "%s enabled\n", loop->function.name);
	return result;
	
disable_out:
	usb_ep_disable(loop->out_ep);
disable_in:
	usb_ep_disable(loop->in_ep);
out:
	return result;
}
```
先进入enable_endpoint

```c
static int enable_endpoint(struct usb_composite_dev *cdev, struct f_loopback *loop,
		struct usb_ep *ep)
{
	int					result;
	/*
	 * one endpoint writes data back IN to the host while another endpoint
	 * just reads OUT packets
	 */
	result = config_ep_by_speed(cdev->gadget, &(loop->function), ep);
	if (result)
		goto fail0;
	result = usb_ep_enable(ep);
	if (result < 0)
		goto fail0;
	ep->driver_data = loop;
	return 0;
fail0:
	pr_err("[LOL]enable_endpoint failed!!!\n");
	return result;
}
struct send_data {
	struct usb_ep *ep;
	char c ;
};
```

这里为端点配置了传输速度，之后便是调用usb_ep_enable来使能端点了

```c
static int alloc_requests(struct usb_composite_dev *cdev,
			  struct f_loopback *loop)
{
	struct usb_request *in_req, *out_req;
	//int i;
	int result = 0;

	//send data
	in_req = lb_alloc_ep_req(loop->in_ep, 1024*1024*2);
	in_req->complete = loopback_complete;
	memset(in_req->buf, 0 , in_req->length);
	result = usb_ep_queue(loop->in_ep, in_req, GFP_ATOMIC);

	//recv_data
	out_req = lb_alloc_ep_req(loop->out_ep, 512);
	out_req->complete = loopback_complete;
	memset(out_req->buf, 0 , out_req->length);
	result = usb_ep_queue(loop->out_ep, out_req, GFP_ATOMIC);
	return result;
}
```

这个函数被我修改的不成人形了，并且没有做错误处理，不要学我...
我们在之前已经申请了两个用来传输的端点，那么怎么来使用它们呢？答案就在这里:
usb的传输需要一个usb_request结构体，跟进lb_alloc_ep_req会看到这个函数

```c
struct usb_request *alloc_ep_req(struct usb_ep *ep, int len, int default_len)
{
	struct usb_request      *req;

	req = usb_ep_alloc_request(ep, GFP_ATOMIC);
	if (req) {
		req->length = len ?: default_len;
		req->buf = kmalloc(req->length, GFP_ATOMIC);
		if (!req->buf) {
			usb_ep_free_request(ep, req);
			req = NULL;
		}
	}
	return req;
}
```

必须使用usb_ep_alloc_request来申请结构体，之后便是申请存放传输数据的内存
传输完成后回调用usb_request中的complete函数
最后便是usb_ep_queue将我们刚创建的usb_request放入到传输队列等到传输了
来看一下complete函数

```c
static void loopback_complete(struct usb_ep *ep, struct usb_request *req)
{
	static unsigned char c = 0;
	struct f_loopback	*loop = ep->driver_data;

	if (ep == loop->out_ep){
		c++;
		memset(req->buf, c , req->length);
		usb_ep_queue(ep, req, GFP_ATOMIC);

	}else if(ep == loop->in_ep){
		((char *)req->buf)[req->length-1] = '\0';
		//do what you want , copy
		usb_ep_queue(ep, req, GFP_ATOMIC);
	}
}
```

当端点是out_ep时，说明之前的发送数据完成了，我们改变发送的字符，继续将usb_request添加到队列中进行传输，至此整个分析流程就完成了，但是这里还有很多不完善的地方比如出错的处理，内存的回收，还需要改善啊，但是测试是够了，接下来就是host端了







### host:
host我是在libusb中的examples中的testlibusb基础上做了简单的修改，废话不多说直接上代码

```c
#include <stdio.h>
#include <string.h>
#include "libusb.h"
#define msleep(msecs) nanosleep(&(struct timespec){msecs / 1000, (msecs * 1000000) % 1000000000UL}, NULL);

int main (int argc, char *argv[])
{
	libusb_device **devs;		//pointer to pointer of device, used to retrieve a list of devices  
	libusb_context *ctx = NULL;	//a libusb session  
	int r;						//for return values  
	ssize_t cnt;				//holding number of devices in list  
	r = libusb_init (&ctx);		//initialize a library session  
	if (r < 0)
	{
		printf ("Init Error %d\n", r);	//there was an error  
		return 1;
	}
	libusb_set_debug (ctx, 3);	//set verbosity level to 3, as suggested in the documentation  
	cnt = libusb_get_device_list (ctx, &devs);	//get the list of devices  
	if (cnt < 0)
	{
		printf ("Get Device Error\n");	//there was an error  
	}

	libusb_device_handle *dev_handle;	//a device handle  
	dev_handle = libusb_open_device_with_vid_pid (ctx, 0x05c6, 0x9999);	//open device
	if (dev_handle == NULL)
	{
		printf ("Cannot open device\n");
		libusb_free_device_list (devs, 1);	//free the list, unref the devices in it  
		libusb_exit (ctx);		//close the session  
		return 0;
	}
	else
	{
		printf ("Device Opened\n");
		libusb_free_device_list (devs, 1);	//free the list, unref the devices in it  
		/*
		   if(libusb_kernel_driver_active(dev_handle, 3) == 1) { //find out if kernel driver is attached  
		   printf("Kernel Driver Active\n");  
		   if(libusb_detach_kernel_driver(dev_handle, 3) == 0) //detach it  
		   printf("Kernel Driver Detached!\n");  
		   }  
		 */
		r = libusb_claim_interface (dev_handle, 3);	//这边的3代表3号interface，claim interface 3 (the first) of device (mine had jsut 1)  
		if (r < 0)
		{
			printf ("Cannot Claim Interface\n");
			return 1;
		}
		printf ("Claimed Interface\n");
		int size;
		unsigned char read_buf[1024 * 1024 * 2] = "\0";
		unsigned char send_buf[1024] = "\0";
		struct timeval old_time, current_time;
		gettimeofday (&old_time, NULL);
		static unsigned long count = 0;
		while (1)
		{
			int i = 0;
			size = 0;
			int rr = 0;
			rr = libusb_bulk_transfer (dev_handle, 0x85, read_buf, sizeof (read_buf),	//1024*1024,  
				&size, 1000);
			count = size + count;
			gettimeofday (&current_time, NULL);
			if ((1000000 * (current_time.tv_sec - old_time.tv_sec) + current_time.tv_usec - old_time.tv_usec) > 1000000)
			{
				printf ("count:%lu ----\n", count / 1024);
				count = 0;
				old_time = current_time;
				/*
				   printf("libusb_bulk_transfer rr: %d \n" , rr);  
				   printf("size: %d\n" ,size);  
				   printf("data:  recv");
				   for(int j=0; j<size; j++)  
				   printf("%02x", (unsigned char)(read_buf[j]));  
				   printf("\n");
				 */
			}
		}

		r = libusb_release_interface (dev_handle, 3);	//release the claimed interface  
		if (r != 0)
		{
			printf ("Cannot Release Interface\n");
			return 1;
		}
		printf ("Released Interface\n");

		libusb_attach_kernel_driver (dev_handle, 3);
		libusb_close (dev_handle);
		libusb_exit (ctx);		//close the session  
		return 0;
	}
}
```