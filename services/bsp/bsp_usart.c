#include "config.h"
#include "softbus.h"
#include "cmsis_os.h"

#include "usart.h"

//UART�����Ϣ
typedef struct {
	UART_HandleTypeDef* huart;
	uint8_t number; //uartX�е�X
	struct 
	{
		uint8_t *data;
		uint16_t maxBufSize;
		uint16_t pos;
	}recvBuffer;
	SoftBusFastHandle fastHandle;
}UARTInfo;

//UART��������
typedef struct {
	UARTInfo* uartList;
	uint8_t uartNum;
	uint8_t initFinished;
}UARTService;

UARTService uartService = {0};
//��������
void BSP_UART_Init(ConfItem* dict);
void BSP_UART_InitInfo(UARTInfo* info, ConfItem* dict);
void BSP_UART_Start_IT(UARTInfo* info);
void BSP_UART_SoftBusCallback(const char* topic, SoftBusFrame* frame, void* bindData);
void BSP_UART_InitRecvBuffer(UARTInfo* info);

//uart���ս����ж�
void BSP_UART_IdleCallback(UART_HandleTypeDef *huart)
{
	if(!uartService.initFinished)//�����ʼ��δ����������־λ
	{              
		(void)huart->Instance->SR; 
		(void)huart->Instance->DR;
		return;
	}
	
	for(uint8_t i = 0; i < uartService.uartNum; ++i)
	{
		UARTInfo* uartInfo = &uartService.uartList[i];
		if(huart == uartInfo->huart)
		{
		   if (__HAL_UART_GET_FLAG(huart, UART_FLAG_RXNE))
			{
				//��ֹ����Խ��
				if(uartInfo->recvBuffer.pos < uartInfo->recvBuffer.maxBufSize)
					uartInfo->recvBuffer.data[uartInfo->recvBuffer.pos++] = uartInfo->huart->Instance->DR;
			}
				
			if (__HAL_UART_GET_FLAG(huart, UART_FLAG_IDLE))
			{
				/* clear idle it flag avoid idle interrupt all the time */
				__HAL_UART_CLEAR_IDLEFLAG(uartInfo->huart);
				uint16_t recSize=uartInfo->recvBuffer.pos;
				SoftBus_FastPublish(uartInfo->fastHandle, {uartInfo->recvBuffer.data, &recSize});
				uartInfo->recvBuffer.pos = 0;
			}
		}
		break;
	}
}

//uart����ص�����
void BSP_UART_TaskCallback(void const * argument)
{
	//�����ٽ���
	portENTER_CRITICAL();
	BSP_UART_Init((ConfItem*)argument);
	portEXIT_CRITICAL();
	
	vTaskDelete(NULL);
}

void BSP_UART_Init(ConfItem* dict)
{
	//�����û����õ�uart����
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
	//��ʼ����uart��Ϣ
	uartService.uartList = pvPortMalloc(uartService.uartNum * sizeof(UARTInfo));
	for(uint8_t num = 0; num < uartService.uartNum; num++)
	{
		char confName[] = "uarts/_";
		confName[6] = num + '0';
		BSP_UART_InitInfo(&uartService.uartList[num], Conf_GetPtr(dict, confName, ConfItem));
	}

	for(uint8_t num = 0; num < uartService.uartNum; num++)
	{
		//��ʼ�����ջ�����
		BSP_UART_InitRecvBuffer(&uartService.uartList[num]);
		//����uart�ж�
		BSP_UART_Start_IT(&uartService.uartList[num]);

	}

	//���Ļ���
	SoftBus_MultiSubscribe(NULL, BSP_UART_SoftBusCallback, {"/uart/trans/it","/uart/trans/dma","/uart/trans/block"});
	uartService.initFinished = 1;
}

//��ʼ��uart��Ϣ
void BSP_UART_InitInfo(UARTInfo* info, ConfItem* dict)
{
	info->huart = Conf_GetPtr(dict, "huart", UART_HandleTypeDef);
	info->number = Conf_GetValue(dict, "uart-x", uint8_t, 0);
	info->recvBuffer.maxBufSize = Conf_GetValue(dict, "maxRecvSize", uint16_t, 1);
	char topic[] = "/uart_/recv";
	topic[5] = info->number + '0';
	info->fastHandle = SoftBus_CreateFastHandle(topic);
}

//����uart�ж�
void BSP_UART_Start_IT(UARTInfo* info)
{
	__HAL_UART_ENABLE_IT(info->huart, UART_IT_RXNE);
	__HAL_UART_ENABLE_IT(info->huart, UART_IT_IDLE);
}
//��ʼ�����ջ�����
void BSP_UART_InitRecvBuffer(UARTInfo* info)
{
   	info->recvBuffer.pos=0;
	info->recvBuffer.data = pvPortMalloc(info->recvBuffer.maxBufSize);
    memset(info->recvBuffer.data,0,info->recvBuffer.maxBufSize);
}

//�����߻ص�
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
			if(!strcmp(topic, "/uart/trans/it")) //�жϷ���
				HAL_UART_Transmit_IT(info->huart,data,transSize);
			else if(!strcmp(topic, "/uart/trans/dma")) //dma����
				HAL_UART_Transmit_DMA(info->huart,data,transSize);
			else if(!strcmp(topic, "/uart/trans/block") && SoftBus_IsMapKeyExist(frame, "timeout"))//����ʽ����
			{
				uint32_t timeout = *(uint32_t*)SoftBus_GetMapValue(frame, "timeout");
				HAL_UART_Transmit(info->huart,data,transSize,timeout);
			}
		}
	}
}