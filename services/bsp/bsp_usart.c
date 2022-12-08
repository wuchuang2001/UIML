#include "config.h"
#include "softbus.h"
#include "cmsis_os.h"

#include "usart.h"

#define UART_IRQ \
	IRQ_FUN(USART1_IRQHandler, 1) \
	IRQ_FUN(USART2_IRQHandler, 2) \
	IRQ_FUN(USART3_IRQHandler, 3) \
	IRQ_FUN(UART4_IRQHandler, 4) \
	IRQ_FUN(UART5_IRQHandler, 5) \
	IRQ_FUN(USART6_IRQHandler, 6) \
	IRQ_FUN(UART7_IRQHandler, 7) \
	IRQ_FUN(UART8_IRQHandler, 8)
	
#define UART_TOTAL_NUM 8

//UART句柄信息
typedef struct {
	UART_HandleTypeDef* huart;
	uint8_t number; //uartX中的X
	struct 
	{
		uint8_t *data;
		uint16_t maxBufSize;
		uint16_t pos;
	}recvBuffer;
	SoftBusFastTopicHandle fastHandle;
}UARTInfo;

//UART服务数据
typedef struct {
	UARTInfo uartList[UART_TOTAL_NUM];
	uint8_t uartNum;
	uint8_t initFinished;
}UARTService;

UARTService uartService = {0};
//函数声明
void BSP_UART_Init(ConfItem* dict);
void BSP_UART_InitInfo(UARTInfo* info, ConfItem* dict);
void BSP_UART_Start_IT(UARTInfo* info);
void BSP_UART_SoftBusCallback(const char* topic, SoftBusFrame* frame, void* bindData);
void BSP_UART_InitRecvBuffer(UARTInfo* info);

//uart接收中断回调函数
void BSP_UART_IRQCallback(uint8_t huartX)
{
	UARTInfo* uartInfo = &uartService.uartList[huartX - 1];
	
	if(!uartService.initFinished)//如果初始化未完成则清除标志位
	{              
		(void)uartInfo->huart->Instance->SR; 
		(void)uartInfo->huart->Instance->DR;
		return;
	}
	
	if (__HAL_UART_GET_FLAG(uartInfo->huart, UART_FLAG_RXNE))
	{
		//防止数组越界
		if(uartInfo->recvBuffer.pos < uartInfo->recvBuffer.maxBufSize)
			uartInfo->recvBuffer.data[uartInfo->recvBuffer.pos++] = uartInfo->huart->Instance->DR;
	}
		
	if (__HAL_UART_GET_FLAG(uartInfo->huart, UART_FLAG_IDLE))
	{
		/* clear idle it flag avoid idle interrupt all the time */
		__HAL_UART_CLEAR_IDLEFLAG(uartInfo->huart);
		uint16_t recSize=uartInfo->recvBuffer.pos;
		SoftBus_FastPublish(uartInfo->fastHandle, {uartInfo->recvBuffer.data, &recSize});
		uartInfo->recvBuffer.pos = 0;
	}
}

//uart任务回调函数
void BSP_UART_TaskCallback(void const * argument)
{
	//进入临界区
	portENTER_CRITICAL();
	BSP_UART_Init((ConfItem*)argument);
	portEXIT_CRITICAL();
	
	vTaskDelete(NULL);
}

void BSP_UART_Init(ConfItem* dict)
{
	//计算用户配置的uart数量
	uartService.uartNum = 0;
	for(uint8_t num = 0; ; num++)
	{
		char confName[] = "uarts/_";
		confName[6] = num + '0';
		if(Conf_ItemExist(dict, confName))
			uartService.uartNum++;
		else
			break;
	}
	//初始化各uart信息
	for(uint8_t num = 0; num < uartService.uartNum; num++)
	{
		char confName[] = "uarts/_";
		confName[6] = num + '0';
		BSP_UART_InitInfo(uartService.uartList, Conf_GetPtr(dict, confName, ConfItem));
	}

	//订阅话题
	SoftBus_MultiSubscribe(NULL, BSP_UART_SoftBusCallback, {"/uart/trans/it","/uart/trans/dma","/uart/trans/block"});
	uartService.initFinished = 1;
}

//初始化uart信息
void BSP_UART_InitInfo(UARTInfo* info, ConfItem* dict)
{
	uint8_t number = Conf_GetValue(dict, "uart-x", uint8_t, 0);
	info[number-1].huart = Conf_GetPtr(dict, "huart", UART_HandleTypeDef);
	info[number-1].number = number;
	info[number-1].recvBuffer.maxBufSize = Conf_GetValue(dict, "maxRecvSize", uint16_t, 1);
	char topic[] = "/uart_/recv";
	topic[5] = info[number-1].number + '0';
	info[number-1].fastHandle = SoftBus_CreateFastTopicHandle(topic);
	//初始化接收缓冲区
	BSP_UART_InitRecvBuffer(&info[number-1]);
	//开启uart中断
	BSP_UART_Start_IT(&info[number-1]);
}

//开启uart中断
void BSP_UART_Start_IT(UARTInfo* info)
{
	__HAL_UART_ENABLE_IT(info->huart, UART_IT_RXNE);
	__HAL_UART_ENABLE_IT(info->huart, UART_IT_IDLE);
}
//初始化接收缓冲区
void BSP_UART_InitRecvBuffer(UARTInfo* info)
{
   	info->recvBuffer.pos=0;
	info->recvBuffer.data = pvPortMalloc(info->recvBuffer.maxBufSize);
    memset(info->recvBuffer.data,0,info->recvBuffer.maxBufSize);
}

//软总线回调
void BSP_UART_SoftBusCallback(const char* topic, SoftBusFrame* frame, void* bindData)
{
	if(!SoftBus_CheckMapKeys(frame, {"uart-x","data","transSize"}))
		return;

	uint8_t uartX = *(uint8_t*)SoftBus_GetMapValue(frame, "uart-x");
	uint8_t* data = (uint8_t*)SoftBus_GetMapValue(frame, "data");
	uint16_t transSize = *(uint16_t*)SoftBus_GetMapValue(frame, "transSize");
	for(uint8_t i = 0; i < uartService.uartNum; i++)
	{
		UARTInfo* info = &uartService.uartList[i];
		if(info->number == uartX)
		{
			if(!strcmp(topic, "/uart/trans/it")) //中断发送
				HAL_UART_Transmit_IT(info->huart,data,transSize);
			else if(!strcmp(topic, "/uart/trans/dma")) //dma发送
				HAL_UART_Transmit_DMA(info->huart,data,transSize);
			else if(!strcmp(topic, "/uart/trans/block") && SoftBus_IsMapKeyExist(frame, "timeout"))//阻塞式发送
			{
				uint32_t timeout = *(uint32_t*)SoftBus_GetMapValue(frame, "timeout");
				HAL_UART_Transmit(info->huart,data,transSize,timeout);
			}
		}
	}
}

//生成中断服务函数
#define IRQ_FUN(irq, number) \
void irq(void) \
{ \
	BSP_UART_IRQCallback(number); \
	HAL_UART_IRQHandler(uartService.uartList[number-1].huart); \
}

UART_IRQ
#undef IRQ_FUN
