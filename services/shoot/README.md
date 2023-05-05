# 发射模块

---

## 模块介绍

这是整个代码库的发射模块，根据用户设置发射单发、连射、停止，完成相应的动作，同时接收拨弹电机的堵转广播，若发生堵弹，会及时退弹。

## 模块依赖项

### 模块依赖

- 服务类模块
	- 无
- 工具类模块
	- [电机模块](../../tools/motor/README.md)（必选）

### 文件依赖

- 本模块文件
	- `shoot.c`（必选）
- 底层库文件 
	- `cmsis_os.h`（必选）

### 其他依赖

- 系统广播（可选）
	- `/system/stop`：在监听到该广播后会设置该模块下所有电机进入急停模式
- 电机堵转广播（必选）
	- `/trigger-motor/stall`：在监听到该广播后会对进行退弹操作，需要将[拨弹电机重命名](../../tools/motor/motor_can/README.md/#模块配置项)

---

## 模块配置项

1. 模块配置项
    
    | 配置名 | (数值类型)默认值 | 说明 |
    | :---: | :---: | :---: |
    | `task-interval`    | `uint16_t` | 20 | 任务执行间隔  |
	| `name`             | `char*`   | `"shooter"` | 如果需要重命名模块则配置该项  |
	| `trigger-angle`    | `float`   | 1/7.0*360 | 拨一发弹丸的角度 |
	| `fric-speed`       | `float`   | 5450      | 初始弹速 |
	| `fric-motor-left`  | `CF_DICT` | / | 左摩擦轮电机配置信息[>>>](../../tools/motor/README.md/#模块配置项)  |
	| `fric-motor-right` | `CF_DICT` | / | 右摩擦轮电机配置信息[>>>](../../tools/motor/README.md/#模块配置项)  |
	| `trigger-motor`    | `CF_DICT` | / | 拨弹电机配置信息[>>>](../../tools/motor/README.md/#模块配置项)  |

### 配置示例：

```c
{"shooter", CF_DICT{
	//任务循环周期
	{"task-interval", IM_PTR(uint16_t, 10)},
	//拨一发弹丸的角度
	{"trigger-angle",IM_PTR(float,45)},
	//发射机构电机配置
	{"fric-motor-left", CF_DICT{
		{"type", "M3508"},
		{"id", IM_PTR(uint16_t, 2)},
		{"can-x", IM_PTR(uint8_t, 2)},
		{"reduction-ratio", IM_PTR(float, 1)},
		{"speed-pid", CF_DICT{
			{"p", IM_PTR(float, 10)},
			{"i", IM_PTR(float, 1)},
			{"d", IM_PTR(float, 0)},
			{"max-i", IM_PTR(float, 10000)},
			{"max-out", IM_PTR(float, 20000)},
			CF_DICT_END
		}},
		CF_DICT_END
	}},		
	{"fric-motor-right", CF_DICT{
		{"type", "M3508"},
		{"id", IM_PTR(uint16_t, 1)},
		{"canX", IM_PTR(uint8_t, 2)},
		{"reduction-ratio", IM_PTR(float, 1)},
		{"speed-pid", CF_DICT{
			{"p", IM_PTR(float, 10)},
			{"i", IM_PTR(float, 1)},
			{"d", IM_PTR(float, 0)},
			{"max-i", IM_PTR(float, 10000)},
			{"max-out", IM_PTR(float, 20000)},
			CF_DICT_END
		}},
		CF_DICT_END
	}},	
	{"trigger-motor", CF_DICT{
		{"type", "M2006"},
		{"id", IM_PTR(uint16_t, 6)},
		{"name","trigger-motor"},
		{"can-x", IM_PTR(uint8_t, 1)},
		{"angle-pid", CF_DICT{								//串级pid
			{"inner", CF_DICT{								//内环pid参数设置
				{"p", IM_PTR(float, 10)},
				{"i", IM_PTR(float, 0)},
				{"d", IM_PTR(float, 0)},
				{"max-i", IM_PTR(float, 10000)},
				{"max-out", IM_PTR(float, 20000)},
				CF_DICT_END
			}},
			{"outer", CF_DICT{								//外环pid参数设置
				{"p", IM_PTR(float, 0.3)},
				{"i", IM_PTR(float, 0)},
				{"d", IM_PTR(float, 0)},
				{"max-i", IM_PTR(float, 2000)},
				{"max-out", IM_PTR(float, 200)},
				CF_DICT_END
			}},
			CF_DICT_END
		}},
		CF_DICT_END
	}},	
	CF_DICT_END		
}},
```

## 软总线接口

> 注：下述广播和远程函数名称中`<shooter_name>`代表服务配置表中`name`配置项的值

### 广播接口

本模块暂无广播接口

### 远程函数接口
  
- **设置发射机构属性**：`/<shooter_name>/setting`

	参数格式如下

    | 数据字段名 | 数据类型 | 是否为返回值 | 是否必须传输 | 说明 |
    | :---: | :---: | :---: | :---: | :---: |
    | `fric-speed`    | `float` | × | 可选 | 设置摩擦轮转速(单位：rpm) |
    | `trigger-angle` | `float` | × | 可选 | 设置拨一发弹丸旋转的角度(单位：°) |
    | `fric-enable`   | `bool`  | × | 可选 | 使能摩擦轮 |

- **设置发射机构运行模式**：`/<shooter_name>/mode`

	> `once`：单发弹丸 
	> `continue`：连发弹丸直到修改模式为idle才停止
	> `idle`：停止发射弹丸

	参数格式如下

    | 数据字段名 | 数据类型 | 是否为返回值 | 是否必须传输 | 说明 |
    | :---: | :---: | :---: | :---: | :---: |
    | `mode`         | `char*`    | × | 必须 | 设置拨弹模式(`"once","continue","idle"`) |
    | `interval-time` | `uint16_t` | × | 可选 | 仅在连发时需要设置，表示连发弹丸时两次发射的间隔时间 |
