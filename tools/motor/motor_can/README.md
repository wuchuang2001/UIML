# CAN电机类

## 简介

本项目对抽象电机类的具体实现，由于大疆的三款电机仅在减速比、发送can数据帧的id以及反馈的数据略有不同外，其他大致相似，因此3种电机的实现大致相似。下面以3508电机实现举例

## 依赖项

项目对freertos的`cmsis_os.h`有依赖，使用了freertos的软件定时器，以及对本项目的`config.h`、`softbus.h`、`motor.h`、`pid.h`有依赖，读取配置表里的参数初始化电机以及对使用pid对电机进行控制和使用软总线跟can总线进行通信

## 子类电机说明

电机类是唯一一个需要和与其他模块通过软总线通信的工具类，该类订阅can总线信息更新电机数据，同时创建软件定时器计算pid完成闭环控制，同时也会进行堵转检测，当发生电机堵转是会广播该事件(6020电机未添加堵转检测)，默认话题名为`"/motor/stall"`，如需订阅该事件强烈建议在配置电机时添加`name`配置项，使其话题名重映射为`"/name/stall"`

## 在`sys_conf.h`中的配置

```c
{"motor", CF_DICT{
	{"type", "M3508"},
	{"id", IM_PTR(uint16_t, 1)},				//电调id(电调灯闪几下就是几)
	{"canX", IM_PTR(uint8_t, 1)},
	{"name", "xxxMotor"}						//当需要订阅电机堵转广播事件时，添加该配置使堵转广播名重映射为"/xxxMotor/stall"
	{"reductionRatio", IM_PTR(float, 19.2)},   //若使用改装减速箱或者拆掉减速箱的电机则修改此参数，若使用原装电机则无需配置此参数
	{"speedPID", CF_DICT{                  //速度单级pid示例，若需要速度模式就配置速度pid，需要角度模式就配置角度pid，若两个模式需要来回切换，则两个都配置
		{"p", IM_PTR(float, 10)},
		{"i", IM_PTR(float, 1)},
		{"d", IM_PTR(float, 0)},
		{"maxI", IM_PTR(float, 10000)},
		{"maxOut", IM_PTR(float, 20000)},
		CF_DICT_END
	}},
	{"anglePID", CF_DICT{                  //角度串级pid示例，如需使用串级pid照此模板配置即可
		{"inner", CF_DICT{
			{"p", IM_PTR(float, 10)},
			{"i", IM_PTR(float, 1)},
			{"d", IM_PTR(float, 0)},
			{"maxI", IM_PTR(float, 10000)},
			{"maxOut", IM_PTR(float, 20000)},
			CF_DICT_END
		}},
		{"outer", CF_DICT{
			{"p", IM_PTR(float, 0.5)},
			{"i", IM_PTR(float, 0)},
			{"d", IM_PTR(float, 0)},
			{"maxI", IM_PTR(float, 25)},
			{"maxOut", IM_PTR(float, 50)},
			CF_DICT_END
		}},
		CF_DICT_END
	}},
	CF_DICT_END
}},
```
