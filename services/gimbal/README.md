# 云台模块

---

## 模块介绍

1. 这是整个代码库的云台模块，根据用户设置的yaw、pitch的角度控制云台进行转动
2. 在本云台模块中需要在`sys_conf.h`配置云台零点的编码器值，在开机上电后会自动根据当前角度和零点的偏差归中


## 模块依赖项

1. 文件依赖

    - 本项目文件
      	- `softbus.c/h`、`config.c/h`、`sys_conf.h`、`pid.c/h`、`motor.c/h`(及其使用到的电机子类)
    - hal库文件 
        - `cmsis_os.h`

2. 模块依赖
    - 系统广播
        - `/system/stop`：在监听到该广播后会设置该模块下所有电机进入急停模式
    - 其他模块
        - `ins`模块：在惯导模式下需要提供`/<ins_name>/euler-angle`惯导计算出来的欧拉角，`<ins_name>`为可以替换部分，例如：在配置文件中添加`{"ins-name", "up-ins"},`就可以将默认的`/ins/euler-angle`，替换成`/up-ins/speed`

---

> 注：下面远程函数所写的数据类型为指针的项仅强调该项传递的应该是数组，实际传递的参数只需数组名即可，不需要传递数组名的地址。广播也是如此，所写的数据类型若为指针的仅强调该项传递的应该是数组，获取该项的值是仅需要强制类型转换成相应的指针即可，无需额外解引用

---

## 模块配置项

1. 模块配置项
    
    | 配置名 | 数值类型 | 默认值 | 说明 |
    | :---: | :---: | :---: | :---: |
    | `task-interval` | `uint16_t` | 2 | 任务执行间隔  |
	| `name`          | `char*`    | `"gimbal"`  | 如果需要重命名模块则配置该项  |
	| `ins-name`      | `char*`    | `"ins"`     | 如果需要重命名惯导接收广播则配置该项  |
	| `zero-yaw`      | `uint16_t` | 0 | yaw轴零点(编码器值)  |
	| `zero-pitch`    | `uint16_t` | 0 | pitch轴零点(编码器值)  |
	| `motor-yaw`     | `CF_DICT`  | / | yaw电机配置信息[>>>](../../tools/motor/README.md/#模块配置项)|
	| `motor-pitch`   | `CF_DICT`  | / | pitch电机配置信息[>>>](../../tools/motor/README.md/#模块配置项)  |

> 注：由于电机角度外环返回数据由惯导提供因此外环pid计算由云台模块完成，电机类只负责速度闭环

> `motor-yaw/imu`，`motor-pitch/imu`两者配置项详见[>>>](../../tools/controller/README.md/#模块配置项)

#### 示例：

```c
//云台服务配置
{"gimbal", CF_DICT{
	//yaw pitch 机械零点
	{"zero-yaw",IM_PTR(uint16_t,4010)},
	{"zero-pitch",IM_PTR(uint16_t,5300)},
	//任务循环周期
	{"task-interval", IM_PTR(uint8_t, 10)},
	//云台电机配置
	{"motor-yaw", CF_DICT{
		{"type", "M6020"},
		{"id", IM_PTR(uint16_t, 1)},
		{"can-x", IM_PTR(uint8_t, 1)},
		{"imu",CF_DICT{								//陀螺仪pid参数设置
			{"p", IM_PTR(float, -90)},
			{"i", IM_PTR(float, 0)},
			{"d", IM_PTR(float, 0)},
			{"max-i", IM_PTR(float, 500)},
			{"max-out", IM_PTR(float, 1000)},
			CF_DICT_END
		}},
		{"speed-pid", CF_DICT{						//陀螺仪做角度外环电机执行速度内环
			{"p", IM_PTR(float, 15)},
			{"i", IM_PTR(float, 0)},
			{"d", IM_PTR(float, 0)},
			{"max-i", IM_PTR(float, 500)},
			{"max-out", IM_PTR(float, 20000)},
			CF_DICT_END
		}},
		CF_DICT_END
	}},			
	{"motor-pitch", CF_DICT{
		{"type", "M6020"},
		{"id", IM_PTR(uint16_t, 4)},
		{"can-x", IM_PTR(uint8_t, 2)},
		{"imu",CF_DICT{								//陀螺仪pid参数设置
			{"p", IM_PTR(float, 63)},
			{"i", IM_PTR(float, 0)},
			{"d", IM_PTR(float, 0)},
			{"max-i", IM_PTR(float, 10000)},
			{"max-out", IM_PTR(float, 20000)},
			CF_DICT_END
			}},
		{"speed-pid", CF_DICT{						//陀螺仪做角度外环电机执行速度内环
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

- 广播：
  
    - 快速方式：无
  
    - 普通方式
  
  	1. `/<gimbal_name>/yaw/relative-angle`

		说明：广播云台yaw轴相对于机械零点的偏离角(单位：°，范围：0°-360°)，`<gimbal_name>`为可以替换部分，例如：在配置文件中添加`{"name", "up-gimbal"},`就可以将默认的`/gimbal/yaw/relative-angle`，替换成`/up-gimbal/yaw/relative-angle`

        广播数据：

        | 数据字段名 | 数据类型 | 说明 |
        | :---: | :---: | :---: |
        | `angle` | `float` | 云台距离零点的分离角 |
		

- 远程函数
  
    1. `/<gimbal_name>/setting`

        说明：设置云台yaw、pitch的角度，`<gimbal_name>`为可以替换部分，例如：在配置文件中添加`{"name", "up-gimbal"},`就可以将默认的`/gimbal/setting`，替换成`/up-gimbal/setting`

        传入参数数据：

        | 数据字段名 | 数据类型 | 是否为返回值 | 是否必须传输 | 说明 |
        | :---: | :---: | :---: | :---: | :---: |
        | `yaw`   | `float` | × | 可选 | 云台的目标yaw角(单位°) |
        | `pitch` | `float` | × | 可选 | 云台的目标pitch角(单位°) |
    