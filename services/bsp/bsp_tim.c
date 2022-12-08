#include "config.h"
#include "softbus.h"
#include "cmsis_os.h"

#include "tim.h"
typedef struct 
{
	TIM_HandleTypeDef *htim;
	uint8_t number;
	SoftBusFastTopicHandle fastHandle;
}TIMInfo;

typedef struct 
{
	TIMInfo *timList;
	uint8_t timNum;
}TIMService;

void BSP_TIM_Init(ConfItem* dict);
void BSP_TIM_InitInfo(TIMInfo* info,ConfItem* dict);
void BSP_TIM_StartHardware(TIMInfo* info,ConfItem* dict);
void BSP_TIM_SoftBusCallback(const char* topic, SoftBusFrame* frame, void* bindData);

TIMService timService={0};

void BASE_TIM_TaskCallback(void const * argument)
{
	//进入临界区
	portENTER_CRITICAL();
	BSP_TIM_Init((ConfItem*)argument);
	portEXIT_CRITICAL();

	vTaskDelete(NULL);
}

void BSP_TIM_Init(ConfItem* dict)
{
	//计算用户配置的can数量
	timService.timNum = 0;
	for(uint8_t num = 0; ; num++)
	{
		char confName[] = "tims/_";
		confName[5] = num + '0';
		if(Conf_ItemExist(dict, confName))
			timService.timNum++;
		else
			break;
	}
	timService.timList=pvPortMalloc(timService.timNum * sizeof(TIMInfo));
	for(uint8_t num = 0; num < timService.timNum; num++)
	{
		char confName[] = "tims/_";
		confName[5] = num + '0';
		BSP_TIM_InitInfo(&timService.timList[num], Conf_GetPtr(dict, confName, ConfItem));
	}
}

void BSP_TIM_InitInfo(TIMInfo* info,ConfItem* dict)
{
	info->htim=Conf_GetPtr(dict,"htim",TIM_HandleTypeDef);
	info->number=Conf_GetValue(dict,"number",uint8_t,0);
	if(!strcmp(Conf_GetPtr(dict,"mode",char),"encode"))
	{
		char topic[14] = "/encode/tim_";
		if(info->number < 10)
		{
			topic[11] = info->number + '0';
		}
		else
		{
			topic[11] = info->number/10 + '0';
			topic[12] = info->number%10 + '0';
		}
		info->fastHandle = SoftBus_CreateFastTopicHandle(topic);  
	}
	else if(!strcmp(Conf_GetPtr(dict,"mode",char),"pwm"))
	{
		char topic[11] = "/pwm/tim_";
		if(info->number < 10)
		{
			topic[8] = info->number + '0';
		}
		else
		{
			topic[8] = info->number/10 + '0';
			topic[9] = info->number%10 + '0';
		}
		SoftBus_Subscribe(NULL,BSP_TIM_SoftBusCallback,topic);
	}
}

void BSP_TIM_StartHardware(TIMInfo* info,ConfItem* dict)
{
	if(!strcmp(Conf_GetPtr(dict,"mode",char),"encode"))
	{
		HAL_TIM_Encoder_Start(info->htim,TIM_CHANNEL_ALL);
	}
	else if(!strcmp(Conf_GetPtr(dict,"mode",char),"pwm"))
	{
		HAL_TIM_PWM_Start(info->htim, TIM_CHANNEL_1);
		HAL_TIM_PWM_Start(info->htim, TIM_CHANNEL_2);
		HAL_TIM_PWM_Start(info->htim, TIM_CHANNEL_3);
		HAL_TIM_PWM_Start(info->htim, TIM_CHANNEL_4);
	}
}
void BSP_TIM_SoftBusCallback(const char* topic, SoftBusFrame* frame, void* bindData)
{
	
}

