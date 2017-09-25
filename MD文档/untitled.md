# Android Sensor分析


## 目标:检测device的状态
- **非法状态关闭光机**
- **非法移动关闭手势**

## 步骤分为三步：
- *1.sensor移植(accelerometer and gyro)*
- *2.framework中注册sensorListenor*
- *3.sensor calibration*

### sensor移植
本文基于[Linux3.18](https://www.kernel.org/)，CPU为[MT8163](https://www.mediatek.com/products/tablets/mt8163).
第一步是驱动部分的移植，Arm Linux内核从3.x引入[DTS](http://www.cnblogs.com/aaronLinux/p/5496559.html#t1).所以需要修改DTS文件.dts修改部分参照原来的配置方案，主要分为两部分，一部分直接挂在root节点下，一部分挂在i2c2控制器下。**也就是说,一个Gsensor(or GYRO)会同时作为i2c device和platform device挂载到i2c总线和platform总线上。** 至于原因下面慢慢分析。
```dts
/ {
	...
    cust_gyro@0 {
    compatible = "mediatek,mpu6050gy";
    i2c_num = <2>;
    i2c_addr = <0x68 0 0 0>;
    direction = <7>;
    power_id = <0xffff>;
    power_vol = <0>;
    firlen = <0>;
    is_batch_supported = <0>;
    };
    cust_accel@0 {
    compatible = "mediatek,mpu6050g";
    i2c_num = <2>;
    i2c_addr = <0x68 0 0 0>;
    direction = <7>;
    power_id = <0xffff>;
    power_vol = <0>;
    firlen = <0>;
    is_batch_supported = <0>;
    };
    ...
    &i2c2 {
      gsensor@68 {
          compatible = "mediatek,gsensor";
          reg = <0x68>;
      };

      gyro@69 {
          compatible = "mediatek,gyro";
          reg = <0x69>;
      };
    }
```
这里只分析acce，gyro的原理是一样的。源码位置[kernel-3.18/drivers/misc/mediatek/accelerometer/mpu6050g-new/mpu6050.c](https://github.com/xuwangly/CSDNBlog/blob/master/Android%20Sensor%E5%88%86%E6%9E%90/accelerometer/mpu6050g-new/mpu6050.c).先来init函数
```c
static int __init mpu6050gse_init(void)
{
	const char *name = "mediatek,mpu6050g";

	hw = get_accel_dts_func(name, hw);
	if (!hw)
		GSE_ERR("get dts info fail\n");

	acc_driver_add(&mpu6050_init_info);
	return 0;
}
```
内容很少，只有两个函数。先get_accel_dts_func将直接挂在root节点下的dts node属性读取出来保存到hw中。再通过**acc_driver_add**来注册driver，下面重点分析这个函数。这个函数在[kernel-3.18/drivers/misc/mediatek/accelerometer/accel.c](https://github.com/xuwangly/CSDNBlog/blob/master/Android%20Sensor%E5%88%86%E6%9E%90/accelerometer/accel.c)中实现。
```c
int acc_driver_add(struct acc_init_info *obj)
{
	int err = 0;
	int i = 0;

	if (!obj) {
		ACC_ERR("ACC driver add fail, acc_init_info is NULL\n");
		return -1;
	}
	for (i = 0; i < MAX_CHOOSE_G_NUM; i++) {
		if ((i == 0) && (NULL == gsensor_init_list[0])) {
			ACC_LOG("register gensor driver for the first time\n");
			if (platform_driver_register(&gsensor_driver))
				ACC_ERR("failed to register gensor driver already exist\n");
		}

		if (NULL == gsensor_init_list[i]) {
			obj->platform_diver_addr = &gsensor_driver;
			gsensor_init_list[i] = obj;
			break;
		}
	}
	if (i >= MAX_CHOOSE_G_NUM) {
		ACC_ERR("ACC driver add err\n");
		err = -1;
	}

	return err;
}
```
关注里面的platform_driver_register，这里就是会和dts中直接挂在root下的node匹配了，那么i2c driver呢？不要急我们继续找。这个函数中关注一下**gsensor_init_list[i] = obj**，我们只有一个gsensor，所以这里的i等于0.去瞅瞅obj是个什么东东。在mpu6050gse_init看到是mpu6050_init_info的地址，那么我们回到[mpu6050.c](https://github.com/xuwangly/CSDNBlog/blob/master/Android%20Sensor%E5%88%86%E6%9E%90/accelerometer/mpu6050g-new/mpu6050.c)
```c
static struct acc_init_info mpu6050_init_info = {
		.name = "mpu6050g",
		.init = mpu6050_local_init,
		.uninit = mpu6050_remove,
};
```
看这个结构体中的成员变量，init和uninit。这肯定是初始化函数嘛，那么它在哪里被执行呢？之前看到它的地址被传到了[accel.c](https://github.com/xuwangly/CSDNBlog/blob/master/Android%20Sensor%E5%88%86%E6%9E%90/accelerometer/accel.c)中，那么去那边看看。accel的初始化函数
```c
static int __init acc_init(void)
{
	ACC_LOG("acc_init\n");

	if (acc_probe()) {
		ACC_ERR("failed to register acc driver\n");
		return -ENODEV;
	}

	return 0;
}

static int acc_probe(void)
{

	int err;

	acc_context_obj = acc_context_alloc_object();
	if (!acc_context_obj) {
		err = -ENOMEM;
		ACC_ERR("unable to allocate devobj!\n");
		goto exit_alloc_data_failed;
	}
	/* init real acceleration driver */
	err = acc_real_driver_init();
	if (err) {
		ACC_ERR("acc real driver init fail\n");
		goto real_driver_init_fail;
	}
	/* init acc common factory mode misc device */
	err = acc_factory_device_init();
	if (err)
		ACC_ERR("acc factory device already registed\n");
	/* init input dev */
	err = acc_input_init(acc_context_obj);
	if (err) {
		ACC_ERR("unable to register acc input device!\n");
		goto exit_alloc_input_dev_failed;
	}

	ACC_LOG("----accel_probe OK !!\n");
	return 0;
	...
}
```
acc_probe函数做了很多事，关注**acc_real_driver_init**其他是input时间上报等，可一自行研究
```c
static int acc_real_driver_init(void)
{
	int i = 0;
	int err = 0;

	ACC_LOG(" acc_real_driver_init +\n");
	for (i = 0; i < MAX_CHOOSE_G_NUM; i++) {
		ACC_LOG(" i=%d\n", i);
		if (0 != gsensor_init_list[i]) {
			ACC_LOG(" acc try to init driver %s\n", gsensor_init_list[i]->name);
			err = gsensor_init_list[i]->init();
			if (0 == err) {
				ACC_LOG(" acc real driver %s probe ok\n",
					gsensor_init_list[i]->name);
				break;
			}
		}
	}

	if (i == MAX_CHOOSE_G_NUM) {
		ACC_LOG(" acc_real_driver_init fail\n");
		err = -1;
	}
	return err;
}
```
找到了！这边执行了**gsensor_init_list[i]->init()**
```c
static int  mpu6050_local_init(void)
{
	MPU6050_power(hw, 1);
	if (i2c_add_driver(&mpu6050g_i2c_driver)) {
		GSE_ERR("add driver error\n");
		return -1;
	}
	if (-1 == mpu6050_init_flag)
		return -1;

	return 0;
}

```
这边注册了i2c driver，至此platform和i2c driver都已经注册完成，之后的数据获取比较简单(无非就是[i2c](https://baike.baidu.com/item/I2C%E6%80%BB%E7%BA%BF/918424?fr=aladdin&fromid=1727975&fromtitle=I2C)通信)，就不贴了。至此，sensor的driver已经调通，并且会上报input事件(**acc_input_init**),有兴趣自己瞅瞅，标准的input设备框架套用，下面进入framework


### framework中注册sensorListenor
framework中比较简单，直接调用[android sensor API](https://developer.android.com/reference/android/hardware/Sensor.html)即可。
```java
private Sensor mGravitySensor;
private final SensorEventListener mListener = new SensorEventListener() {
        @Override
        public void onSensorChanged(SensorEvent event) {
            float acceleroX = event.values[0];
            float acceleroY = event.values[1];
            float acceleroZ = event.values[2];

           //do what you want
        
        }
         @Override
        public void onAccuracyChanged(Sensor sensor, int accuracy) {
        }			  
 };
 //init
 mGravitySensor = sensorManager.getDefaultSensor(Sensor.TYPE_ACCELEROMETER);
 //register
 if (sensorManager.registerListener(mListener, mGravitySensor,SensorManager.SENSOR_DELAY_NORMAL)){
 	Slog.e(TAG, "mGravitySensor!!!");
 }
    
```
这么简单就可一获取到acce的数据了，但是得到数据怎么用就看你自己咯

### sensor calibration
一般我们的sensor在出厂前需要做很多操作，其中就有一个sensor校准
这边给一个mtk平台通过adb进入工程模式进行sensor校准的命令
```sh
adb shell am start -n com.mediatek.engineermode/.EngineerMode
```
再贴一个MTKsensor校准代码
[sensor_cali](https://github.com/xuwangly/CSDNBlog/tree/master/sensor_adb)

