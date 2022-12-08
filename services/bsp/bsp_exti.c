#include "softbus.h"
#include  "cmsis_os.h"
#include "config.h"
#include "gpio.h"

//EXTI GPIO信息
typedef struct
{
	GPIO_TypeDef* GPIOX;
	uint16_t pin;
	SoftBusFastTopicHandle fastHandle;
}EXTIInfo;

//EXTI服务数据
typedef struct {
	EXTIInfo extiList[16];
	uint8_t extiNum;
	uint8_t initFinished;
}EXTIService;

//函数声明
void BSP_EXTI_Init(ConfItem* dict);
void BSP_EXIT_InitInfo(EXTIInfo* info, ConfItem* dict);

EXTIService extiService={0};

//外部中断服务函数
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	if(!extiService.initFinished)
		return;  
	uint8_t pin = 31 - __clz((uint32_t)GPIO_Pin);//使用内核函数__clz就算GPIO_Pin前导0的个数，从而得到中断线号
	EXTIInfo* extiInfo = &extiService.extiList[pin];
	GPIO_PinState state = HAL_GPIO_ReadPin(extiInfo->GPIOX, GPIO_Pin);
	SoftBus_FastPublish(extiInfo->fastHandle,{&state});
}
//EXTI任务回调函数
void BSP_EXTI_TaskCallback(void const * argument)
{
	//进入临界区
	portENTER_CRITICAL();
	BSP_EXTI_Init((ConfItem*)argument);
	portEXIT_CRITICAL();
	
	vTaskDelete(NULL);
}
//EXTI初始化
void BSP_EXTI_Init(ConfItem* dict)
{
	//计算用户配置的exit数量
	extiService.extiNum = 0;
	for(uint8_t num = 0; ; num++)
	{
		char confName[] = "extis/_";
		confName[6] = num + '0';
		if(Conf_ItemExist(dict, confName))
			extiService.extiNum++;
		else
			break;
	}

	for(uint8_t num = 0; num < extiService.extiNum; num++)
	{
		char confName[9] = "extis/_";
		if(num<10)
		{
			confName[6] = num + '0';
		}
		else
		{
			confName[6] = num/10 + '0';
			confName[7] = num%10 + '0';
		}
		BSP_EXIT_InitInfo(extiService.extiList, Conf_GetPtr(dict, confName, ConfItem));
	}
	extiService.initFinished=1;
}

//初始化EXTI信息
void BSP_EXIT_InitInfo(EXTIInfo* info, ConfItem* dict)
{
	uint8_t pin = Conf_GetValue(dict, "pin-x", uint8_t, 0);
	info[pin].GPIOX = Conf_GetPtr(dict, "gpio-x", GPIO_TypeDef);
	char topic[12] = "/exti/pin_";
	if(pin < 10)
	{
		topic[9] = pin + '0';
	}
	else
	{
		topic[9] = pin/10 + '0';
		topic[10] = pin%10 + '0';
	}
	//重新映射至GPIO_PIN=2^pin
	info[pin].pin = 1 << pin;
	info[pin].fastHandle = SoftBus_CreateFastTopicHandle(topic);
}


