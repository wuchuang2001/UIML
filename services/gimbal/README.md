# 云台模块

---

## 模块介绍

本模块描述了一个装有陀螺仪的二轴云台，用户可以指定云台所指向的 yaw / pitch 目标角度，本模块计算后会向电机模块发送控制指令

**控制方法**：两个云台电机都被配置为速度模式，本模块中分别使用一个单级PID通过陀螺仪角度偏差求得电机目标速度，然后发送给电机模块

## 模块依赖项

### 模块依赖

- 服务类模块
	- [惯导模块](../ins/README.md)（必选）
		- `/<ins_name>/euler-angle`：惯导提供欧拉角，`<ins_name>`代表本服务配置表中`ins-name`项的值
- 工具类模块
	- [电机模块](../../tools/motor/README.md)（必选）
	- [PID模块](../../tools/controller/README.md)（必选）

### 文件依赖

- 本模块文件
	- `gimbal.c`（必选）
- 底层库文件 
	- `cmsis_os.h`（必选）

### 其他依赖

- 系统广播（可选）
	- `/system/stop`：在监听到该广播后会设置该模块下所有电机进入急停模式

---

## 模块配置项

1. 模块配置项
    
    | 配置名 | 数值类型 | 默认值 | 说明 |
    | :---: | :---: | :---: | :---: |
    | `task-interval` | `uint16_t` | 2 | 任务执行间隔  |
	| `name`          | `char*`    | `"gimbal"`  | 本服务的运行时名称  |
	| `ins-name`      | `char*`    | `"ins"`     | 为本服务提供欧拉角的惯导模块的名称  |
	| `zero-yaw`      | `uint16_t` | 0 | yaw轴机械零点对应的电机编码器值  |
	| `zero-pitch`    | `uint16_t` | 0 | pitch轴机械零点对应的电机编码器值  |
	| `yaw-imu-pid`   | `CF_DICT`  | / | yaw轴角度外环pid配置[>>>](../../tools/controller/README.md/#模块配置项)  |
	| `pitch-imu-pid` | `CF_DICT`  | / | pitch轴角度外环pid配置[>>>](../../tools/controller/README.md/#模块配置项)  |
	| `motor-yaw`     | `CF_DICT`  | / | yaw电机配置信息[>>>](../../tools/motor/README.md/#模块配置项)|
	| `motor-pitch`   | `CF_DICT`  | / | pitch电机配置信息[>>>](../../tools/motor/README.md/#模块配置项)  |

### 配置示例：

```c
//云台服务配置
{"gimbal", CF_DICT{
	//yaw pitch 机械零点
	{"zero-yaw",IM_PTR(uint16_t,4010)},
	{"zero-pitch",IM_PTR(uint16_t,5300)},
	//任务循环周期
	{"task-interval", IM_PTR(uint16_t, 10)},
	//陀螺仪pid参数设置，作为角度外环
	{"yaw-imu-pid",CF_DICT{
		{"p", IM_PTR(float, -90)},
		{"i", IM_PTR(float, 0)},
		{"d", IM_PTR(float, 0)},
		{"max-i", IM_PTR(float, 500)},
		{"max-out", IM_PTR(float, 1000)},
		CF_DICT_END
	}},
	//云台电机配置
	{"motor-yaw", CF_DICT{
		{"type", "M6020"},
		{"id", IM_PTR(uint16_t, 1)},
		{"can-x", IM_PTR(uint8_t, 1)},
		//速度环参数设置
		{"speed-pid", CF_DICT{						
			{"p", IM_PTR(float, 15)},
			{"i", IM_PTR(float, 0)},
			{"d", IM_PTR(float, 0)},
			{"max-i", IM_PTR(float, 500)},
			{"max-out", IM_PTR(float, 20000)},
			CF_DICT_END
		}},
		CF_DICT_END
	}},	
	//陀螺仪pid参数设置，作为角度外环
	{"pitch-imu-pid",CF_DICT{
		{"p", IM_PTR(float, 63)},
		{"i", IM_PTR(float, 0)},
		{"d", IM_PTR(float, 0)},
		{"max-i", IM_PTR(float, 10000)},
		{"max-out", IM_PTR(float, 20000)},
		CF_DICT_END
	}},
	{"motor-pitch", CF_DICT{
		{"type", "M6020"},
		{"id", IM_PTR(uint16_t, 4)},
		{"can-x", IM_PTR(uint8_t, 2)},
		//速度环参数设置
		{"speed-pid", CF_DICT{
			{"p", IM_PTR(float, 15)},
			{"i", IM_PTR(float, 0)},
			{"d", IM_PTR(float, 0)},
			{"max-i", IM_PTR(float, 500)},
			{"max-out", IM_PTR(float, 20000)},
			CF_DICT_END
		}},
		CF_DICT_END
	}},	
	CF_DICT_END		
}},
```

## 软总线接口

> 注：下述广播和远程函数名称中`<gimbal_name>`代表服务配置表中`name`配置项的值

### 广播接口
  
- **广播云台yaw轴电机相对于机械零点的旋转角**：`/<gimbal_name>/yaw/relative-angle`

	- **广播类型**：普通方式（映射表数据帧）
    
    - **数据帧格式**

    | 数据字段名 | 数据类型 | 说明 |
    | :---: | :---: | :---: |
    | `angle` | `float` | 云台距离零点的分离角(单位：deg，范围：0~360) |
		

### 远程函数接口
  
- **设置云台目标转角**：`/<gimbal_name>/setting`

	参数格式如下

    | 数据字段名 | 数据类型 | 是否为返回值 | 是否必须传输 | 说明 |
    | :---: | :---: | :---: | :---: | :---: |
    | `yaw`   | `float` | × | 可选 | 云台的目标yaw角(单位deg) |
    | `pitch` | `float` | × | 可选 | 云台的目标pitch角(单位deg) |
    
	> 注：此接口设置的是累计转角，即范围可超过360°
