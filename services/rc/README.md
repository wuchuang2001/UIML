# 鼠标键盘事件模块

---

## 项目简介

本项目实现了对机甲大师官方遥控器数据的解析和二次抽象封装，使用软总线机制，采用消息发布方式。目前本项目支持键盘鼠标的单击、长按、抬起、按住四种事件，并支持`Ctrl`或`Shift`组合键触发，可以大幅简化对键盘数据的处理流程

---

## 包含文件
`RC.c`：源文件，需要被包含在Keil工程中，包含全局变量定义和函数的实现

---

## 项目依赖

1.  系统配置文件`config.h`
2.	软总线文件 `softbus.h`
3.	freertos支持文件 `cmsis_os.h`
---

## 概念阐述

**按键及鼠标左右键**
一个按键事件包含**主键**、**事件类型**和**组合键**三个部分，当操作手的操作符合以上三项时，对应消息就会被发布
* 主键：官方遥控器所支持的键盘按键以及鼠标左右键
* 事件类型：有单击、长按、按下、抬起、按住五种
  * 单击：按键按下后在`长按判定时间`之前抬起，抬起时触发
  * 长按：按键按下超过`长按判定时间`，在超时时触发（而非抬起时）
  * 按下：在按键按下时触发
  * 抬起：在按键抬起时触发
  * 按住：在按键被按下时会连续定时触发，触发频率与`RC_UpdateKeys()`调用频率一致
* 组合键：无组合键、`Ctrl`、`Shift`三种，注意主键选择`Ctrl`或`Shift`时组合键不可选择相同键

**消息发布**
发布的topic为
1. 单击	`"rc/key/on-click"` 
2. 长按 `"rc/key/on-long-press"` 
3. 按下 `"rc/key/on-down"` 
4. 抬起 `"rc/key/on-up"` 
5. 按住 `"rc/key/on-pressing"` 
   
均有iterm
1. `"key"` 为按下的主按键（包括鼠标左右键）
2. `"combine-key"`为组合按键 `"none"`、 `"ctrl"`、 `"shift"`

**遥控器及鼠标移动**
将遥控器分为`左摇杆`、`右摇杆`、`拨杆`、`滚轮`、`鼠标移动`五个部分,其中左、右摇杆均包括`通道x`、`通道y`；拨杆包括`左拨杆`、`右拨杆`；鼠标包括`通道x`、`通道y`。只有当上述四个部分中的数据有更新时则向外发布对应部分的消息。

**消息发布**
发布的topic为
1. 右摇杆`"rc/right-stick"` 有iterm `"x"`、`"y"`
2. 左摇杆`"rc/left-stick"` 有iterm	`"x"`、`"y"`
3. 拨杆`"rc/switch"` 有iterm `"right"`、`"left"`
4. 滚轮`"rc/wheel"` 有iterm `"value"`
5. 鼠标移动`"rc/mouse-move"`有iterm `"x"`、`"y"`
---

## 模块配置项

* 需要在`sys_conf.h`为rc添加服务`SERVICE(rc, RC_TaskCallback, osPriorityNormal,256)`
* 需要在`sys_conf.h`添加rc配置

---


## 使用示例(HAL)

```c
void test_TaskCallback(void const * argument)
{
	SoftBus_MultiSubscribe(&recrc,callback,{"rc/right-stick","rc/left-stick","rc/switch","rc/wheel"});
	SoftBus_MultiSubscribe(NULL,Chassis_MoveCallback,{"rc/key/on-pressing"});
	SoftBus_MultiSubscribe(NULL,Chassis_StopCallback,{"rc/key/on-up"});
	SoftBus_MultiSubscribe(NULL,Chassis_RotateCallback,{"rc/mouse-move"});	
	SoftBus_MultiSubscribe(NULL,Shooter_ShooterCallback,{"rc/key/on-click"});	
	while(1)
	{
		osDelay(10);
	}

}

void Chassis_MoveCallback(const char* topic, SoftBusFrame* frame, void* bindData)
{
	float speedRatio=0,vx=0,vy=0;;
	if(!strcmp(SoftBus_GetMapValue(frame,"combine-key"),"none"))
		speedRatio=1;
	else if(!strcmp(SoftBus_GetMapValue(frame,"combine-key"),"shift"))
		speedRatio=5;
	else if(!strcmp(SoftBus_GetMapValue(frame,"combine-key"),"ctrl"))
		speedRatio=0.2;

	if(!strcmp(SoftBus_GetMapValue(frame,"key"),"A"))
		vx = 200*speedRatio;
	if(!strcmp(SoftBus_GetMapValue(frame,"key"),"D"))
		vx = -200*speedRatio;
	if(!strcmp(SoftBus_GetMapValue(frame,"key"),"W"))
		vy = 200*speedRatio;
	if(!strcmp(SoftBus_GetMapValue(frame,"key"),"S"))
		vy = -200*speedRatio;
	SoftBus_Publish("chassis",{{"vx",&vx},{"vy",&vy}});
}
```

---

## 注意事项

* `RC_InitKeyJudgeTime(rc,0x3ffff,50,500)`默认全部按键50~500ms之间为短按，大于500ms为长按，可自行修改
* 如果效率不够可将不同的话题订阅的不同的回调函数，而不是一个回调函数集中处理。此时无需考虑话题名是否正确。
* 若要移植到非HAL库平台，需要将`RC_UpdateKeys()`中的`HAL_GetTick()`修改为目标平台中用于获取时间戳（既系统运行时间，单位ms）的API函数
