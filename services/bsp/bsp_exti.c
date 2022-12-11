#include "softbus.h"
#include  "cmsis_os.h"
#include "config.h"
#include "gpio.h"
#include "stdio.h"

//EXTI GPIO信息
typedef struct
{
	GPIO_TypeDef* GPIOX;
	uint16_t pin;
	SoftBusReceiverHandle fastHandle;
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
void BSP_EXIT_BroadcastCallback(const char* name, SoftBusFrame* frame, void* bindData);
EXTIService extiService={0};

//外部中断服务函数
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	if(!extiService.initFinished)
		return;  
	uint8_t pin = 31 - __clz((uint32_t)GPIO_Pin);//使用内核函数__clz就算GPIO_Pin前导0的个数，从而得到中断线号
	EXTIInfo* extiInfo = &extiService.extiList[pin];
	GPIO_PinState state = HAL_GPIO_ReadPin(extiInfo->GPIOX, GPIO_Pin);
	Bus_FastBroadcastSend(extiInfo->fastHandle,{&state});
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
		char confName[9];
		sprintf(confName,"extis/%d",num);
		if(Conf_ItemExist(dict, confName))
			extiService.extiNum++;
		else
			break;
	}

	for(uint8_t num = 0; num < extiService.extiNum; num++)
	{
		char confName[9] = "extis/_";
		sprintf(confName,"extis/%d",num);
		BSP_EXIT_InitInfo(extiService.extiList, Conf_GetPtr(dict, confName, ConfItem));
	}
	extiService.initFinished=1;
}

//初始化EXTI信息
void BSP_EXIT_InitInfo(EXTIInfo* info, ConfItem* dict)
{
	uint8_t pin = Conf_GetValue(dict, "pin-x", uint8_t, 0);
	info[pin].GPIOX = Conf_GetPtr(dict, "gpio-x", GPIO_TypeDef);
	char name[12];
	sprintf(name,"/exti/pin%d",pin);
	//重新映射至GPIO_PIN=2^pin
	info[pin].pin = 1 << pin;
	info[pin].fastHandle = Bus_CreateReceiverHandle(name);
	Bus_MultiRegisterReceiver(NULL,BSP_EXIT_BroadcastCallback,{"/exti/pin4","/exti/pin5"});

}


void BSP_EXIT_BroadcastCallback(const char* name, SoftBusFrame* frame, void* bindData)
{
	if(!Bus_CheckMapKeys(frame, {"gpio-x","pin-x"}))
		return;
  
	static int acc_op=0;
	GPIO_TypeDef *gpiox = (GPIO_TypeDef*)Bus_GetMapValue(frame, "gpio-x");
	uint8_t pinx = *(uint8_t*)Bus_GetMapValue(frame, "pin-x");
	for(uint8_t i = 0; i < extiService.extiNum; i++)
	{
		EXTIInfo* info = &extiService.extiList[i];
		if(!memcmp((info->GPIOX),gpiox,sizeof(GPIO_TypeDef)))
		{
				if(info->pin==pinx)
				{
					GPIO_PinState state = HAL_GPIO_ReadPin(info->GPIOX, info->pin);
					char*chip={0};
					if(state==SET)
					{
						char*state={"on"};
						
						if(!strcmp(name, "/exti/pin4"))
						{
						 HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
							if(acc_op==0)
							{
								chip="ACCEL";
								Bus_FastBroadcastSend(info->fastHandle,{"chip",chip});	
								Bus_FastBroadcastSend(info->fastHandle,{"state",state});
							}
							else if(acc_op==1)
							{
								chip="TEMP";
								Bus_FastBroadcastSend(info->fastHandle,{"chip",chip});	
								Bus_FastBroadcastSend(info->fastHandle,{"state",state});
							}
							acc_op++;
							acc_op%=2;
						}
					 else if(!strcmp(name, "/exti/pin5"))
					{
						
						HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);
						chip="GYRO";
								Bus_FastBroadcastSend(info->fastHandle,{"chip",chip});	
								Bus_FastBroadcastSend(info->fastHandle,{"state",state});

					}
				 }
					else 
					{
						char* state={"off"};
						Bus_FastBroadcastSend(info->fastHandle,{"state",state});
						chip="ACCEL";
								Bus_FastBroadcastSend(info->fastHandle,{"chip",chip});	
						chip="TEMP";
								Bus_FastBroadcastSend(info->fastHandle,{"chip",chip});	
						chip="GYRO";
								Bus_FastBroadcastSend(info->fastHandle,{"chip",chip});	
			
					}
		 		}
	
		}

	}
}
