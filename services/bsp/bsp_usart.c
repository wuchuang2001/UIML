#include "config.h"
#include "softbus.h"
#include "cmsis_os.h"
#include <string.h>
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
	SoftBusReceiverHandle fastHandle;
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
bool BSP_UART_BlockCallback(const char* name, SoftBusFrame* frame, void* bindData);
bool BSP_UART_ItCallback(const char* name, SoftBusFrame* frame, void* bindData);
bool BSP_UART_DMACallback(const char* name, SoftBusFrame* frame, void* bindData);
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
		uint16_t recSize=uartInfo->recvBuffer.pos; //此时pos值为一帧数据的长度
		Bus_FastBroadcastSend(uartInfo->fastHandle, {uartInfo->recvBuffer.data, &recSize}); //空闲中断为一帧，发送一帧数据
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

	//注册远程函数
	Bus_RegisterRemoteFunc(NULL, BSP_UART_BlockCallback, "/uart/trans/block");
	Bus_RegisterRemoteFunc(NULL, BSP_UART_ItCallback, "/uart/trans/it");
	Bus_RegisterRemoteFunc(NULL, BSP_UART_DMACallback, "/uart/trans/dma");
	uartService.initFinished = 1;
}

//初始化uart信息
void BSP_UART_InitInfo(UARTInfo* info, ConfItem* dict)
{
	uint8_t number = Conf_GetValue(dict, "number", uint8_t, 0);
	info[number-1].huart = Conf_GetPtr(dict, "huart", UART_HandleTypeDef);
	info[number-1].number = number;
	info[number-1].recvBuffer.maxBufSize = Conf_GetValue(dict, "max-recv-size", uint16_t, 1);
	char name[] = "/uart_/recv";
	name[5] = info[number-1].number + '0';
	info[number-1].fastHandle = Bus_CreateReceiverHandle(name);
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

//阻塞回调
bool BSP_UART_BlockCallback(const char* name, SoftBusFrame* frame, void* bindData)
{
	if(!Bus_CheckMapKeys(frame, {"uart-x", "data", "trans-size", "timeout"}))
		return false;

	uint8_t uartX = *(uint8_t*)Bus_GetMapValue(frame, "uart-x");
	uint8_t* data = (uint8_t*)Bus_GetMapValue(frame, "data");
	uint16_t transSize = *(uint16_t*)Bus_GetMapValue(frame, "trans-size");
	uint32_t timeout = *(uint32_t*)Bus_GetMapValue(frame, "timeout");
	HAL_UART_Transmit(uartService.uartList[uartX - 1].huart,data,transSize,timeout);
	return true;
}

//中断发送回调
bool BSP_UART_ItCallback(const char* name, SoftBusFrame* frame, void* bindData)
{
	if(!Bus_CheckMapKeys(frame, {"uart-x","data","trans-size"}))
		return false;

	uint8_t uartX = *(uint8_t*)Bus_GetMapValue(frame, "uart-x");
	uint8_t* data = (uint8_t*)Bus_GetMapValue(frame, "data");
	uint16_t transSize = *(uint16_t*)Bus_GetMapValue(frame, "trans-size");
	HAL_UART_Transmit_IT(uartService.uartList[uartX - 1].huart,data,transSize);
	return true;
}

//DMA发送回调
bool BSP_UART_DMACallback(const char* name, SoftBusFrame* frame, void* bindData)
{
	if(!Bus_CheckMapKeys(frame, {"uart-x","data","trans-size"}))
		return false;

	uint8_t uartX = *(uint8_t*)Bus_GetMapValue(frame, "uart-x");
	uint8_t* data = (uint8_t*)Bus_GetMapValue(frame, "data");
	uint16_t transSize = *(uint16_t*)Bus_GetMapValue(frame, "trans-size");
	HAL_UART_Transmit_DMA(uartService.uartList[uartX - 1].huart,data,transSize);
	return true;
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
