# 发射模块

---

## 模块介绍

1. 这是整个代码库的发射模块，根据用户设置发射单发、连射、停止，完成相应的动作，同时接收拨弹电机的堵转广播，若发生堵弹，会及时退弹。
2. 由于监听了拨弹电机堵转广播，因此需要在配置拨弹电机时，需要将拨弹电机重命名`{"name", "triggerMotor"},`


## 项目文件及依赖项

1. 文件依赖
   
    - 本项目文件
      	- `softbus.c/h`、`config.c/h`、`sys_conf.h`、`motor.c/h`(及其使用到的电机子类)
    - hal库文件 
        - `cmsis_os.h`

2. 模块依赖

    - 系统广播
        - `/system/stop`：在监听到该广播后会设置该模块下所有电机进入急停模式
    - 电机堵转广播
        - `"/trigger-motor/stall"`：在监听到拨弹电机堵转后会做出退弹操作

---

> 注：下面远程函数所写的数据类型为指针的项仅强调该项传递的应该是数组，实际传递的参数只需数组名即可，不需要传递数组名的地址。广播也是如此，所写的数据类型若为指针的仅强调该项传递的应该是数组，获取该项的值是仅需要强制类型转换成相应的指针即可，无需额外解引用

---

## 模块配置项

1. 模块配置项
    
    | 配置名 | (数值类型)默认值 | 说明 |
    | :---: | :---: | :---: |
    | `task-interval`    | (uint16_t)20                      | 任务执行间隔  |
	| `name`             | (char*)`"shooter"`                | 如果需要重命名模块则配置该项  |
	| `trigger-angle`    | (float)1/7.0*360                  | 拨一发弹丸的角度 |
	| `fric-speed`       | (float)5450                       | 初始弹速 |
	| `fric-motor-left`  | [>>](../../tools/motor/README.md/#模块配置项) | 左摩擦轮电机配置信息  |
	| `fric-motor-right` | [>>](../../tools/motor/README.md/#模块配置项) | 右摩擦轮电机配置信息  |
	| `trigger-motor`    | [>>](../../tools/motor/README.md/#模块配置项) | 拨弹电机配置信息  |

#### 示例：

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

- 广播：无

- 远程函数
  
    1. `/<shooter_name>/setting`

        说明：设置拨弹电机的一些属性，`<shooter_name>`为可以替换部分，例如：在配置文件中添加`{"name", "up-shooter"},`就可以将默认的`/shooter/setting`，替换成`/up-shooter/setting`

        传入参数数据：

        | 数据字段名 | 数据类型 | 是否为返回值 | 是否必须传输 | 说明 |
        | :---: | :---: | :---: | :---: | :---: |
        | `fric-speed`    | `float` | × | 可选 | 设置摩擦轮转速(单位：rpm) |
        | `trigger-angle` | `float` | × | 可选 | 设置拨一发弹丸旋转的角度(单位：°) |
        | `fric-enable`   | `bool`  | × | 可选 | 使能摩擦轮 |
    
    2. `/<shooter_name>/mode`

        说明：修改发射机构运行模式，`<shooter_name>`为可以替换部分，例如：在配置文件中添加`{"shooter", "up-shooter"},`就可以将默认的`/shooter/mode`，替换成`/up-shooter/mode`

		`once`：单发弹丸 

		`continue`：连发弹丸直到修改模式为idle才停止
		
		`idle`：停止发射弹丸

        传入参数数据：

        | 数据字段名 | 数据类型 | 是否为返回值 | 是否必须传输 | 说明 |
        | :---: | :---: | :---: | :---: | :---: |
        | `mode`         | `char*`    | × | 必须 | 设置拨弹模式(`"once","continue","idle"`) |
        | `interval-time` | `uint16_t` | × | 可选 | 仅在连发时需要设置，表示连发弹丸时两次发射的间隔时间 |
