# PWM电机类

## 模块简介

1. 本模块时对抽象电机类的具体实现
2. 电机类是唯一一个需要和与其他模块通过软总线通信的工具类，直流电机通过创建软件定时器定时获取编码器数据计算速度，同时进行pid控制，舵机根据用户设定的角度设置pwm占空比

## 模块依赖项

1. 文件依赖

    - 本项目文件
      	- `softbus.c、h`、`config.c/h`、`sys_conf.h`、`motor.c/h`、`pid.h`
  	- 标准库文件
    	- `stdint.h`、`stdio.h`、`string.h`
    - hal库文件 
        - `cmsis_os.h`

## 模块配置项

- 直流电机

1. 模块配置项
    
    | 配置名 | (数值类型)默认值 | 说明 |
    | :---: | :---: | :---: |
    | `type`            | (char*)NULL   | 电机类型，类型有：[>>](../README.md) |
    | `max-encode`      | (float)48     | 倍频后编码器转一圈的最大值 |
	| `reduction-ratio` | (float)19     | 电机减速比 |
	| `pos-rotate-tim`  | [>>](#motor2) | 正转pwm配置信息 |
	| `neg-rotate-tim`  | [>>](#motor3) | 反转pwm配置信息 |
	| `encode-tim`      | [>>](#motor4) | 编码器配置信息 |
	| `speed-pid`       | [>>](../../controller/README.md) | 速度单级pid |
	| `angle-pid`       | [>>](#motor5) | 角度串级pid |

2. <span id='motor2'>`pos-rotate-tim`配置项</span>

    | 配置名 | (数值类型)默认值 | 说明 |
    | :---: | :---: | :---: |
    | `tim-x`     | (uint8_t)0 | 正转pwm使用的定时器几   |
    | `channel-x` | (uint8_t)0 | 正转pwm使用的定时器通道 |

3. <span id='motor3'>`neg-rotate-tim`配置项</span>

    | 配置名 | (数值类型)默认值 | 说明 |
    | :---: | :---: | :---: |
    | `tim-x`     | (uint8_t)0 | 反转pwm使用的定时器几   |
    | `channel-x` | (uint8_t)0 | 反转pwm使用的定时器通道 |

4. <span id='motor4'>`encode-tim`配置项</span>

    | 配置名 | (数值类型)默认值 | 说明 |
    | :---: | :---: | :---: |
    | `tim-x` | (uint8_t)0 | 编码器使用的定时器几 |

5. <span id='motor5'>`angle-pid`配置项</span>

    | 配置名 | (数值类型)默认值 | 说明 |
    | :---: | :---: | :---: |
    | `inner` | [>>](../../controller/README.md) | 内环pid |
    | `outer` | [>>](../../controller/README.md) | 外环pid |

	```c
	{"dc-motor", CF_DICT{
		{"type", "DcMotor"},
		{"max-encode", IM_PTR(float, 48)},		//倍频后编码器转一圈的最大值
		{"reduction-ratio", IM_PTR(float, 18)},	//减速比
		{"pos-rotate-tim", CF_DICT{       		//正转pwm配置信息
			{"tim-x", IM_PTR(uint8_t, 8)},
			{"channel-x", IM_PTR(uint8_t, 1)},
			CF_DICT_END
		}},
		{"neg-rotate-tim", CF_DICT{				//反转转pwm配置信息      
			{"tim-x", IM_PTR(uint8_t, 8)},
			{"channel-x", IM_PTR(uint8_t, 2)},
			CF_DICT_END
		}},
		{"encode-tim", CF_DICT{					//编码器配置信息
			{"tim-x", IM_PTR(uint8_t, 1)},
			CF_DICT_END
		}},
		{"speed-pid", CF_DICT{                  //速度单级pid示例，若需要速度模式就配置速度pid，需要角度模式就配置角度pid，若两个模式需要来回切换，则两个都配置
			{"p", IM_PTR(float, 10)},
			{"i", IM_PTR(float, 1)},
			{"d", IM_PTR(float, 0)},
			{"max-i", IM_PTR(float, 10000)},
			{"max-out", IM_PTR(float, 20000)},
			CF_DICT_END
		}},
		{"angle-pid", CF_DICT{                  //角度串级pid示例，如需使用串级pid照此模板配置即可
			{"inner", CF_DICT{
				{"p", IM_PTR(float, 10)},
				{"i", IM_PTR(float, 1)},
				{"d", IM_PTR(float, 0)},
				{"max-i", IM_PTR(float, 10000)},
				{"max-out", IM_PTR(float, 20000)},
				CF_DICT_END
			}},
			{"outer", CF_DICT{
				{"p", IM_PTR(float, 0.5)},
				{"i", IM_PTR(float, 0)},
				{"d", IM_PTR(float, 0)},
				{"max-i", IM_PTR(float, 25)},
				{"max-out", IM_PTR(float, 50)},
				CF_DICT_END
			}},
			CF_DICT_END
		}},
		CF_DICT_END
	}},
	```
- 舵机

1. 模块配置项
    
    | 配置名 | (数值类型)默认值 | 说明 |
    | :---: | :---: | :---: |
    | `type`      | (char*)NULL  | 电机类型，类型有：[>>](../README.md) |
    | `tim-x`     | (uint8_t)0   | 舵机使用的定时器几 |
	| `channel-x` | (uint8_t)0   | 舵机使用的定时器通道 |
	| `max-angle` | (float)180   | 舵机最大转角 |
	| `max-duty`  | (float)0.125 | 舵机最大转角对应的占空比 |
	| `min-duty`  | (float)0.025 | 舵机0°对应的占空比 |

	```c
	{"servo", CF_DICT{
		{"type", "Servo"},
		{"tim-x", IM_PTR(uint8_t, 1)},
		{"channel-x", IM_PTR(uint8_t, 2)},
		{"max-angle", IM_PTR(float, 180)},   //舵机最大转角
		{"max-duty", IM_PTR(float, 0.125f)},	//舵机最大转角对应的占空比
		{"min-duty", IM_PTR(float, 0.025f)},	//舵机0°对应的占空比
		CF_DICT_END
	}},
	```
