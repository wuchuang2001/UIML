#include "config.h"
#include "softbus.h"
#include "cmsis_os.h"

#include "can.h"

#define BSP_MALLOC_PORT(len) pvPortMalloc(len)
#define BSP_FREE_PORT(ptr) vPortFree(ptr)

typedef struct
{
	CAN_HandleTypeDef* hcan;
	char* name;
	uint8_t isSlave;
	uint8_t** txBuffer;
	uint8_t txIdNum;
}CANInfo;

typedef struct
{
	CANInfo* canInfo;
	uint8_t canNUm;
	uint8_t taskInterval;
}CAN;

void BSP_CAN_SoftBusCallback(const char* topic, SoftBusFrame* frame, void* bindData);
uint8_t BSP_CAN_SendData(CAN_HandleTypeDef* hcan,uint32_t StdId,uint8_t data[8]);

CAN can = {0};

//can接收结束中断
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
	CAN_RxHeaderTypeDef header;
  uint8_t rx_data[10];
	
	HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &header, rx_data+2);
	*(uint16_t*)rx_data = header.StdId;
	
	for(uint8_t i = 0; i < can.canNUm; i++)
	{
		if(hcan->Instance == can.canInfo[i].hcan->Instance)
		{
			SoftBus_PublishMap("canReceive",{
				{can.canInfo[i].name, rx_data, 10}
			});
		}
	}
}

//can初始化，在while(1)前调用
void BSP_CAN_Init(ConfItem* dict)
{
	can.taskInterval = Conf_GetValue(dict, "taskInterval", uint8_t, 2);
	uint8_t num = Conf_GetValue(dict, "canNum", uint8_t, 0);
	can.canInfo = (CANInfo*)BSP_MALLOC_PORT(num*sizeof(CANInfo));
	for(uint8_t i = 0; i < num; ++i)
	{
		can.canInfo[i].hcan = Conf_GetPtr(dict, "hcan", CAN_HandleTypeDef*)[i];
		can.canInfo[i].name = Conf_GetPtr(dict, "name", char*)[i];
		can.canInfo[i].isSlave = Conf_GetPtr(dict, "isSlave", uint8_t)[i];
		can.canInfo[i].txIdNum = Conf_GetPtr(dict, "txIdNum", uint8_t)[i];
		can.canInfo[i].txBuffer = (uint8_t**)BSP_MALLOC_PORT(can.canInfo[i].txIdNum*sizeof(uint8_t*));
		for(uint8_t j = 0; j < can.canInfo[i].txIdNum; ++j)
		{
			can.canInfo[i].txBuffer[j] = (uint8_t*)BSP_MALLOC_PORT(10*sizeof(uint8_t));
			*(uint16_t*)can.canInfo[i].txBuffer[j] = Conf_GetPtr(dict, "txId", uint16_t)[j];
			memset(can.canInfo[i].txBuffer[j]+2,0,8);
		}
		
		//CAN过滤器初始化
		CAN_FilterTypeDef canFilter = {0};
		canFilter.FilterActivation = ENABLE;
		canFilter.FilterMode = CAN_FILTERMODE_IDMASK;
		canFilter.FilterScale = CAN_FILTERSCALE_32BIT;
		canFilter.FilterIdHigh = 0x0000;
		canFilter.FilterIdLow = 0x0000;
		canFilter.FilterMaskIdHigh = 0x0000;
		canFilter.FilterMaskIdLow = 0x0000;
		canFilter.FilterFIFOAssignment = CAN_RX_FIFO0;
		if(can.canInfo[i].isSlave)
		{
			canFilter.SlaveStartFilterBank=14;
			canFilter.FilterBank = 14;
		}
		else
		{
			canFilter.FilterBank = 0;
		}
		HAL_CAN_ConfigFilter(can.canInfo[i].hcan, &canFilter);
		//开启CAN
		HAL_CAN_Start(can.canInfo[i].hcan);
		HAL_CAN_ActivateNotification(can.canInfo[i].hcan, CAN_IT_RX_FIFO0_MSG_PENDING);
	}
	can.canNUm = num;//放最后，确保初始化结束后can中断才能读数据
	SoftBus_Subscribe(NULL, BSP_CAN_SoftBusCallback, "canSend");
}

//CAN发送数据
uint8_t BSP_CAN_SendData(CAN_HandleTypeDef* hcan,uint32_t StdId,uint8_t data[8])
{
	CAN_TxHeaderTypeDef tx_header;
	
	tx_header.StdId = StdId;
  tx_header.IDE   = CAN_ID_STD;
  tx_header.RTR   = CAN_RTR_DATA;
  tx_header.DLC   = 8;
	
	uint8_t retVal=HAL_CAN_AddTxMessage(hcan, &tx_header, data, (uint32_t*)CAN_TX_MAILBOX0);
	
	return retVal;
}

//can发送任务回调函数
void BSP_CAN_TaskCallback(void const * argument)
{
	BSP_CAN_Init((ConfItem*)argument);
	TickType_t tick = xTaskGetTickCount();
	while(1)
	{
		for(uint8_t i = 0; i < can.canNUm; ++i)
		{
			for(uint8_t j = 0; j < can.canInfo[i].txIdNum; ++j)
			{
				BSP_CAN_SendData(can.canInfo[i].hcan, *(uint16_t*)can.canInfo[i].txBuffer[j], can.canInfo[i].txBuffer[j]+2);
			}
		}
		osDelayUntil(&tick,can.taskInterval);
	}
}

void BSP_CAN_SoftBusCallback(const char* topic, SoftBusFrame* frame, void* bindData)
{
//	if(!strcmp(topic, "canSend"))
//	{
		const SoftBusItem* item = SoftBus_GetItem(frame, "canX");
		if(item)
		{
			char* canName = (char*)item->data;
			for(uint8_t i = 0; i < can.canNUm; ++i)
			{
				if(!strcmp(can.canInfo[i].name, canName))
				{
					item = SoftBus_GetItem(frame, "id");
					if(item)
					{
						uint16_t canId = *(uint16_t*)item->data;
						for(uint8_t j = 0; j < can.canInfo[i].txIdNum; ++j)
						{
							if(canId == *(uint16_t*)can.canInfo[i].txBuffer[j])
							{
								item = SoftBus_GetItem(frame, "bits");
								if(item)
								{
									uint8_t setBits = *(uint8_t*)item->data;
									item = SoftBus_GetItem(frame, "data");
									if(item)
									{
										uint8_t* datas = (uint8_t*)item->data;
										uint8_t num = 0;
										for(uint8_t k = 0; k < 8; ++k)
										{
											if(setBits & 1)
											{
												can.canInfo[i].txBuffer[j][2+k] = datas[num];
												++num;
											}
											else if(!setBits)
											{
												break;
											}
											setBits >>= 1;
										}
									}
								}
							}
							break;
						}
					}
				}
				break;
			}
		}
		
//	}
}
