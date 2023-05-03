# PWM电机类

## 简介

本项目对抽象电机类的具体实现.

## 依赖项

项目对freertos的`cmsis_os.h`有依赖，使用了freertos的软件定时器，以及对本项目的`config.h`、`softbus.h`、`motor.h`、`pid.h`有依赖，读取配置表里的参数初始化电机以及对使用pid对电机进行控制和使用软总线跟tim进行通信

## 子类电机说明

电机类是唯一一个需要和与其他模块通过软总线通信的工具类，直流电机通过创立软件定时器定时获取编码器数据计算速度，同时进行pid控制，舵机根据用户设定的角度设置pwm占空比

## 在`sys_conf.h`中的配置

- 直流电机

	```c
	{"dc-motor", CF_DICT{
		{"type", "DcMotor"},
		{"maxEncode", IM_PTR(float, 48)},		//倍频后编码器转一圈的最大值
		{"reductionRatio", IM_PTR(float, 18)},	//减速比
		{"posRotateTim", CF_DICT{       		//正转pwm配置信息
			{"tim-x", IM_PTR(uint8_t, 8)},
			{"channel-x", IM_PTR(uint8_t, 1)},
			CF_DICT_END
		}},
		{"negRotateTim", CF_DICT{				//反转转pwm配置信息      
			{"tim-x", IM_PTR(uint8_t, 8)},
			{"channel-x", IM_PTR(uint8_t, 2)},
			CF_DICT_END
		}},
		{"encodeTim", CF_DICT{					//编码器配置信息
			{"tim-x", IM_PTR(uint8_t, 1)},
			CF_DICT_END
		}},
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
- 舵机

	```c
	{"servo", CF_DICT{
		{"type", "Servo"},
		{"timX", IM_PTR(uint8_t, 1)},
		{"channelX", IM_PTR(uint8_t, 2)},
		{"maxAngle", IM_PTR(float, 180)},   //舵机最大转角
		{"maxDuty", IM_PTR(float, 0.125f)},	//舵机最大转角对应的占空比
		{"minDuty", IM_PTR(float, 0.025f)},	//舵机0°对应的占空比
		CF_DICT_END
	}},
	```
