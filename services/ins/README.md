# INS模块

---

## 简介

这是整个代码库的惯导系统模块，根据imu的驱动获取imu的角速度和加速度，进而进行姿态解算出当前imu坐标系的姿态角同时广播出去。


## 模块依赖项

- 本项目文件
	- `softbus.c/h`、`config.c/h`、`sys_conf.h`、`bmi088_driver.c/h`、`AHRS_MiddleWare.c/h`、`AHRS.lib/h`、`pid.c/h`、`filter.c/h`
- 标准库文件
	- `stdint.h`、`stdbool.h`、`stdlib.h`
- hal库文件 
    - `cmsis_os.h`

---

> 注：下面远程函数所写的数据类型为指针的项仅强调该项传递的应该是数组，实际传递的参数只需数组名即可，不需要传递数组名的地址。广播也是如此，所写的数据类型若为指针的仅强调该项传递的应该是数组，获取该项的值是仅需要强制类型转换成相应的指针即可，无需额外解引用

---

## 说明
目前bmi088配置未开放到`sys_conf.h`，如需要修改加速度计或陀螺仪量程等请到`bmi088_driver.c`中修改命令表的值，目前使用的是mahony算法作为姿态解算算法

## 在STM32CubeMX中需要的bmi088的spi配置

spi配置如下
  
   ![spi配置](README-IMG/bmi088的spi配置.png)
	


## 在`sys_conf.h`中的配置

```c
{"ins",CF_DICT{
	{"spi-x",IM_PTR(uint8_t,1)},
	{"tim-x",IM_PTR(uint8_t,10)},
	{"channel-x",IM_PTR(uint8_t,1)},
	{"tmpPID", CF_DICT{
		{"p", IM_PTR(float, 0.15)},
		{"i", IM_PTR(float, 0.01)},
		{"d", IM_PTR(float, 0)},
		{"maxI", IM_PTR(float, 0.15)},
		{"maxOut", IM_PTR(float, 1)},
		CF_DICT_END
	}},
	CF_DICT_END
}},
```

## 模块接口

- 广播：

    - 快速方式：无
  
    - 普通方式
  
  	1. `/<ins_name>/euler-angle`

		说明：广播imu坐标系下3轴姿态角(单位：°)，`<ins_name>`为可以替换部分，例如：在配置文件中添加`{"ins", "up-ins"},`就可以将默认的`/ins/euler-angle`，替换成`/up-ins/euler-angle`

        广播数据：

        | 数据字段名 | 数据类型 | 说明 |
        | :---: | :---: | :---: |
        | `yaw`   | `float` | imu坐标系的yaw旋转角度 |
		| `pitch` | `float` | imu坐标系的pitch旋转角度 |
		| `roll`  | `float` | imu坐标系的roll旋转角度 |

- 远程函数：无
  