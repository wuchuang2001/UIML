# CAN电机类

## 模块简介

1. 本项目对抽象电机类的具体实现，由于大疆的三款电机仅在减速比、发送can数据帧的id以及反馈的数据略有不同外，其他大致相似，因此3种电机的实现大致相似
2. 电机类是唯一一个需要和与其他模块通过软总线通信的工具类，该类订阅can总线信息更新电机数据，同时创建软件定时器计算pid完成闭环控制，同时也会进行堵转检测，当发生电机堵转是会广播该事件(6020电机未添加堵转检测)，默认话题名为`"/motor/stall"`，如需订阅该事件强烈建议在配置电机时添加`name`配置项，使其话题名重映射为`"/name/stall"`

## 模块依赖项

1. 文件依赖

    - 本项目文件
      	- `softbus.c、h`、`config.c/h`、`sys_conf.h`、`motor.c/h`、`pid.h`
  	- 标准库文件
    	- `stdint.h`、`stdlib.h`、`string.h`
    - hal库文件 
        - `cmsis_os.h`

## 模块配置项

1. 模块配置项
    
    | 配置名 | (数值类型)默认值 | 说明 |
    | :---: | :---: | :---: |
    | `type`            | (char*)NULL | 电机类型，类型有：[>>](../README.md/#模块配置项) |
    | `id`              | (uint16_t)0 | 电调id(电调灯闪几下就是几) |
	| `can-x`           | (uint16_t)0 | 挂载在哪条can总线上 |
	| `name`            | (char*)"motor" | 如需要订阅电机堵转广播，需要配置此信息 |
	| `reduction-ratio` | (float)默认原装电机减速比 | 电机减速比，使用原装电机则无需配置此参数 |
	| `speed-pid`       | [>>](../../controller/README.md/#模块配置项) | 速度单级pid |
	| `angle-pid`       | [>>](#motor2) | 角度串级pid |

2. <span id='motor2'/>`angle-pid`配置项

    | 配置名 | (数值类型)默认值 | 说明 |
    | :---: | :---: | :---: |
    | `inner` | [>>](../../controller/README.md/#模块配置项) | 内环pid |
    | `outer` | [>>](../../controller/README.md/#模块配置项) | 外环pid |

### 示例：

```c
{"motor", CF_DICT{
	{"type", "M3508"},
	{"id", IM_PTR(uint16_t, 1)},				//电调id(电调灯闪几下就是几)
	{"can-x", IM_PTR(uint8_t, 1)},
	{"name", "xxx-motor"}						//当需要订阅电机堵转广播事件时，添加该配置使堵转广播名重映射为"/xxxMotor/stall"
	{"reduction-ratio", IM_PTR(float, 19.2)},   //若使用改装减速箱或者拆掉减速箱的电机则修改此参数，若使用原装电机则无需配置此参数
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
