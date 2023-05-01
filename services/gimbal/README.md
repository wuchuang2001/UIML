# 云台模块

---

## 简介

这是整个代码库的云台模块，根据用户设置的yaw、pitch的角度控制云台进行转动。同时可以选择使用电机编码器作为角度闭环反馈还是使用惯导传来的角度作为角度闭环反馈


## 模块依赖项

- 本项目文件
	- `softbus.c/h`、`config.c/h`、`sys_conf.h`、`pid.c/h`、`motor.c/h`(及其使用到的电机子类)
- hal库文件 
    - `cmsis_os.h`
- 系统广播
    - `/system/stop`：在监听到该广播后会设置该模块下所有电机进入急停模式
- 其他模块
    - `ins`模块：在惯导模式下需要提供`/ins/euler-angle`惯导计算出来的欧拉角

---

> 注：下面远程函数所写的数据类型为指针的项仅强调该项传递的应该是数组，实际传递的参数只需数组名即可，不需要传递数组名的地址。广播也是如此，所写的数据类型若为指针的仅强调该项传递的应该是数组，获取该项的值是仅需要强制类型转换成相应的指针即可，无需额外解引用

---

## 说明

在本云台模块中需要在`sys_conf.h`配置云台零点的编码器值，在开机上电后会自动根据当前角度和零点的偏差归中

## 在`sys_conf.h`中的配置

```c
{"gimbal", CF_DICT{
	//yaw pitch 机械零点
	{"zero-yaw",IM_PTR(uint16_t,4010)},
	{"zero-pitch",IM_PTR(uint16_t,5300)},
	//任务循环周期
	{"taskInterval", IM_PTR(uint8_t, 10)},
	//云台电机配置
	{"motor-yaw", CF_DICT{
		{"type", "M6020"},
		{"id", IM_PTR(uint16_t, 1)},
		{"canX", IM_PTR(uint8_t, 1)},
		{"anglePID", CF_DICT{                  			//串级pid
			{"inner", CF_DICT{							//内环pid参数设置
				{"p", IM_PTR(float,15)},
				{"i", IM_PTR(float, 0)},
				{"d", IM_PTR(float, 0)},
				{"maxI", IM_PTR(float, 10000)},
				{"maxOut", IM_PTR(float, 20000)},
				CF_DICT_END
			}},
			{"outer", CF_DICT{							//外环pid参数设置
				{"p", IM_PTR(float, 15)},
				{"i", IM_PTR(float, 0)},
				{"d", IM_PTR(float, 0)},
				{"maxI", IM_PTR(float, 500)},
				{"maxOut", IM_PTR(float, 1000)},
				CF_DICT_END
			}},
			CF_DICT_END
		}},
		CF_DICT_END
	}},			
	{"motor-pitch", CF_DICT{
		{"type", "M6020"},
		{"id", IM_PTR(uint16_t, 4)},
		{"canX", IM_PTR(uint8_t, 2)},
		{"anglePID", CF_DICT{                 //串级pid
			{"inner", CF_DICT{								//内环pid参数设置
				{"p", IM_PTR(float, 15)},
				{"i", IM_PTR(float, 0)},
				{"d", IM_PTR(float, 0)},
				{"maxI", IM_PTR(float, 10000)},
				{"maxOut", IM_PTR(float, 20000)},
				CF_DICT_END
			}},
			{"outer", CF_DICT{								//外环pid参数设置
				{"p", IM_PTR(float, 20)},
				{"i", IM_PTR(float, 0)},
				{"d", IM_PTR(float, 0)},
				{"maxI", IM_PTR(float, 500)},
				{"maxOut", IM_PTR(float, 1000)},
				CF_DICT_END
			}},
			CF_DICT_END
		}},
		CF_DICT_END
	}},	
	CF_DICT_END		
}},
```

## 模块接口

> 注：name重映射只需要在配置表中配置名写入原本name字符串，在配置值处写入重映射后的name字符串，就完成了name的重映射。例如：`{"old-name", "new-name"},`

- 广播：
  
    - 快速方式：无
  
    - 普通方式
  
  	1. `/gimbal/yaw/relative-angle`

		说明：广播云台yaw轴相对于机械零点的偏离角(单位：°，范围：0°-360°)

        **是否允许name重映射：允许**

        广播数据：

        | 数据字段名 | 数据类型 | 说明 |
        | :---: | :---: | :---: |
        | `angle` | `float` | 云台距离零点的分离角 |
		

- 远程函数
  
    1. `/gimbal/setting`

        说明：设置云台yaw、pitch的角度

        **是否允许name重映射：允许**

        传入参数数据：

        | 数据字段名 | 数据类型 | 是否为返回值 | 是否必须传输 | 说明 |
        | :---: | :---: | :---: | :---: | :---: |
        | `yaw`   | `float` | × | 可选 | 云台的目标yaw角(单位°) |
        | `pitch` | `float` | × | 可选 | 云台的目标pitch角(单位°) |
    