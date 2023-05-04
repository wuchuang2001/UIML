# PID类

## 模块简介

1. 本项目实现了一个PID类，能够进行单级、串级PID计算、清空PID数据、设置死区等功能

## 模块依赖项

1. 文件依赖

    - 本项目文件
      	- `config.c/h`、`sys_conf.h`
  	- 标准库文件
    	- `stdint.h`

## 模块配置项

1. 模块配置项
    
    | 配置名 | (数值类型)默认值 | 说明 |
    | :---: | :---: | :---: |
    | `p`       | (float)0  | kp |
	| `i`       | (float)0  | ki |
	| `d`       | (float)0  | kd |
	| `max-i`   | (float)0  | 积分限幅 |
	| `max-out` | (float)0  | 输出限幅 |

#### 示例：

```c
{"pid", CF_DICT{
    {"p", IM_PTR(float, 0.15)},
    {"i", IM_PTR(float, 0.01)},
    {"d", IM_PTR(float, 0)},
    {"max-i", IM_PTR(float, 0.15)},
    {"max-out", IM_PTR(float, 1)},
    CF_DICT_END
}},
```

## 接口说明

1. `void PID_Init(PID *pid, ConfItem* conf)`
   
    会根据传入的配置初始化pid参数。使用示例：

	```c
	PID pid;//单级pid
    PID_Init(&pid, dict); 
    CascadePID pidC;//串级pid
    PID_Init(&pidC.inner, dict); //初始化内环
    PID_Init(&pidC.outer, dict); //初始化外环
	```

2. `void PID_SingleCalc(PID *pid,float reference,float feedback)`

	通过此函数可以计算单级pid，其中`reference`为目标值，`feedback`为反馈值。使用示例：

	```c
	PID_SingleCalc(&pid, ref, fd);
	```

3. `void PID_CascadeCalc(CascadePID *pid,float angleRef,float angleFdb,float speedFdb)`

	通过此函数可以计算单级pid，其中`angleRef`为角度目标值，`angleFdb`为角度反馈值，`speedFdb`为速度反馈值。使用示例：

	```c
	PID_SingleCalc(&pidC, angleRef, angleFdb, speedFdb);
	```

4. `void PID_Clear(PID *pid)`

	通过此函数可以清除pid的历史数据。使用示例：

	```c
	PID_Clear(&pid);
	```
5. `void PID_SetMaxOutput(PID *pid,float maxOut)`

	通过此函数可以设置pid的输出限幅。使用示例：

	```c
	PID_SetMaxOutput(&pid);
	```

6. `void PID_SetDeadzone(PID *pid,float deadzone)`

	通过此函数可以设置pid的死区。使用示例：

	```c
	PID_SetDeadzone(&pid, 5);
	```

