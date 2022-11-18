# 鼠标键盘事件模块

---

## 项目简介

本项目实现了对机甲大师官方遥控器数据的解析和二次抽象封装，按照常见键鼠开发方式，使用了事件分发机制。目前本项目支持键盘鼠标的单击、长按、抬起、按住四种事件，并支持`Ctrl`或`Shift`组合键触发，可以大幅简化对键盘数据的处理流程

---

## 包含文件

`RC.h`：头文件，需要放在引用目录中，包含枚举、结构体的定义和外部接口函数的声明

`RC.c`：源文件，需要被包含在Keil工程中，包含全局变量定义和函数的实现

---

## 项目依赖

1. 需要为本模块提供一个获取系统时间戳的接口（如`HAL库`中的`HAL_GetTick()`），置于`RC_UpdateKeys()`函数中
2. 依赖C语言标准头文件`stdint.h`

---

## 概念阐述

**按键事件**
一个按键事件包含**主键**、**事件类型**和**组合键**三个部分，当操作手的操作符合以上三项时，对应的一个按键事件就会被触发
* 主键：官方遥控器所支持的键盘按键以及鼠标左右键
* 事件类型：有单击、长按、按下、抬起、按住五种
  * 单击：按键按下后在`长按判定时间`之前抬起，抬起时触发
  * 长按：按键按下超过`长按判定时间`，在超时时触发（而非抬起时）
  * 按下：在按键按下时触发
  * 抬起：在按键抬起时触发
  * 按住：在按键被按下时会连续定时触发，触发频率与`RC_UpdateKeys()`调用频率一致
* 组合键：无组合键、`Ctrl`、`Shift`三种，注意主键选择`Ctrl`或`Shift`时组合键不可选择相同键

**回调函数**
在一个按键事件被触发时，本模块会自动调用其所有回调函数，并向回调函数的参数传入被触发的事件信息

**注册**
将回调函数与事件进行绑定的过程，将函数B注册在事件A上，则事件A发生后B会被调用。本模块中的注册是多对多的，即一个事件可对应多个函数，一个函数可对应多个事件

---

## 模块配置项

* 宏定义`MAX_KEY_CALLBACK_NUM`为每个主键所能分配的最多回调函数个数，过小可能导致回调函数过多无法分配，过多会占用较大内存，默认值为5

---

## API

* `void RC_ParseData(uint8_t* buff);`：解析串口数据，用户需要在接收到一帧来自遥控器串口的完整数据帧后，将数据缓冲区首指针传入该函数，该函数会解析数据并更新全局变量`rcInfo`

* `void RC_Register(uint32_t key,KeyCombineType combine,KeyEventType event,KeyCallbackFunc func);`：为一个回调函数注册按键事件，需要传入事件对应的主键`key`、组合键`combine`、事件类型`event`以及要注册的回调函数名`func`。各枚举量可自行根据代码注释查看，其中key参数可以使用按位或的方式传入多个主键以同时注册多个事件。注意回调函数的类型须与`KeyCallbackFunc`一致，既`void(*)(KeyType,KeyCombineType,KeyEventType)`类型

* `void RC_InitKeys(void);`：初始化所有按键的判定时间，该函数需要在系统上电初始化时调用。该函数会调用内部函数`RC_InitKeyJudgeTime()`进行每个按键单击和长按判定时间的的设置，用户可根据需求自行修改

* `void RC_UpdateKeys(void);`：更新按键信息，该函数需要定时被调用（建议调用间隔为14ms左右），在该函数内会进行按键事件的判定和回调函数的触发

---

## 使用流程

1. 在`RC_InitKeys()`中配置好各按键的判定时间，调整`MAX_KEY_CALLBACK_NUM`设定好每个主键对应的最多回调函数数量

2. 在系统上电初始化过程中调用`RC_InitKeys()`进行初始化设置

3. 在其他各个模块的初始化过程中调用`RC_Register()`进行回调函数的事件注册

4. 在遥控器一帧完整数据接收结束时（如空闲中断中）调用`RC_ParseData()`进行数据解析

## 使用示例(HAL)

```c
/**********grabber.c（取弹模块）**********/
//键盘回调 根据按键选择弹药箱
void Grabber_KeyCallback_ChooseBox(KeyType key,KeyCombineType combine,KeyEventType event)
{
	switch(key)
	{
		case Key_Q:
		//...
		break;
		case Key_W:
		//...
		break;
		case Key_E:
		//...
		break;
	}
}

//模块初始化
void Grabber_Init()
{
	//...
	//为Grabber_KeyCallback_ChooseBox函数注册按键QWE+Ctrl
	RC_Register(Key_Q|Key_W|Key_E, CombineKey_Ctrl, KeyEvent_OnClick, Grabber_KeyCallback_ChooseBox);
	//...
}

/**********main.c**********/
int main()
{
	//...
	RC_Init();
	//...
	Grabber_Init();
	//...
	while(1)
	{

	}
}

/**********stm32f4xx_it.c（包含串口中断）**********/
//...
uint8_t rcBuf[20];//串口缓冲区
//...
void USART1_IRQHandler(void)
{
	//...
	if(__HAL_UART_GET_FLAG(&huart1, UART_FLAG_IDLE))//空闲中断,注意要事先开启
	{
		RC_ParseData(rcBuf);
		//...
	}
	//...
}
```

---

## 注意事项

* 回调函数不应占用过长时间，由于回调函数是在`RC_UpdateKeys()`函数中被调用，回调函数过长的执行时间会导致该函数执行时间较长

* 若要移植到非HAL库平台，需要将`RC_UpdateKeys()`中的`HAL_GetTick()`修改为目标平台中用于获取时间戳（既系统运行时间，单位ms）的API函数
