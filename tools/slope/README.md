# 斜坡函数类

## 模块简介

本模块可以用来模拟斜坡函数，用户在设置步长后通过本模块可以使阶跃函数变成斜坡函数

## 模块依赖项

无

## 模块配置项

无

## 接口说明

1. `void Slope_Init(Slope *slope,float step,float deadzone)`
   
    斜坡函数初始化，`step`为步长，即每次更新时增加的值；`deadzone`为死区，即当前值与最终值小于死区时不再更新。使用示例：

	```c
	Slope slope;//单级pid
    Slope_Init(&slope, 2, 0); 
	```

2. `void Slope_SetTarget(Slope *slope,float target)`

	通过此函数可以设置斜坡函数最终值。使用示例：

	```c
	Slope_SetTarget(&slope, 10);
	```

3. `void Slope_SetStep(Slope *slope,float step)`

	通过此函数可以斜坡函数步长。使用示例：

	```c
	Slope_SetStep(&slope, 2);
	```

4. `float Slope_NextVal(Slope *slope)`

	此函数用于更新斜坡值，并返回更新后的值。使用示例：

	```c
	float value = Slope_NextVal(&slope);
	```
5. `float Slope_GetVal(Slope *slope)`

	通过此函数可以获取当前斜坡值。使用示例：

	```c
	float value = Slope_GetVal(&slope);
	```
