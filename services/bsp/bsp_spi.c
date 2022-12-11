#include "config.h"
#include "softbus.h"
#include "cmsis_os.h"

#include "spi.h"

//SPI句柄信息
typedef struct {
	SPI_HandleTypeDef* hspi;
	uint8_t number; //SPIX中的X
		struct 
	{
		uint8_t  *data;  	
		uint8_t MaxSize;
	}SPIBuffer;
	SoftBusReceiverHandle fastHandle;
}SPIInfo;


//SPI服务数据
typedef struct {
	SPIInfo* spiList;
	uint8_t spiNum;
	uint8_t initFinished;
}SPIService;

SPIService spiService={0};
//函数声明

void BSP_SPI_Init(ConfItem* dict);
void BSP_SPI_InitInfo(SPIInfo* info, ConfItem* dict);
void BSP_SPI_SoftBusCallback(const char* name, SoftBusFrame* frame, void* bindData);
void BSP_SPI_InitBuffer(SPIInfo* info, ConfItem* dict);


void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);
	
}

//SPI任务回调函数
void BSP_SPI_TaskCallback(void const * argument)
{
	//进入临界区
	portENTER_CRITICAL();
	BSP_SPI_Init((ConfItem*)argument);
	portEXIT_CRITICAL();
	
	vTaskDelete(NULL);
}
void BSP_SPI_Init(ConfItem* dict)
{
	//计算用户配置的spi数量
	spiService.spiNum = 0;
	for(uint8_t num = 0; ; num++)
	{
		char confName[] = "spis/_";
		confName[5] = num + '0';
		if(Conf_ItemExist(dict, confName))
			spiService.spiNum++;
		else
			break;
	}
	
	//初始化各spi信息
	spiService.spiList = pvPortMalloc(spiService.spiNum * sizeof(SPIInfo));
	for(uint8_t num = 0; num < spiService.spiNum; num++)
	{
		char confName[] = "spis/_";
		confName[5] = num + '0';
		BSP_SPI_InitInfo(&spiService.spiList[num], Conf_GetPtr(dict, confName, ConfItem));
	}
	
	
	for(uint8_t num = 0; num < spiService.spiNum; num++)
	{
		char confName[] = "spibuffer/_";
		confName[10] = num + '0';
		//初始化接收缓冲区
		BSP_SPI_InitBuffer(&spiService.spiList[num],Conf_GetPtr(dict, confName, ConfItem)); 
	}
	//订阅话题
	Bus_MultiRegisterReceiver(NULL,BSP_SPI_SoftBusCallback,{"/spi/trans/dam","/spi/trans/block","/exti/pin4","/exti/pin5"});
	spiService.initFinished = 1;
}
//初始化spi信息
void BSP_SPI_InitInfo(SPIInfo* info, ConfItem* dict)
{
  info->hspi=Conf_GetPtr(dict,"hspi",SPI_HandleTypeDef);
	info->number=Conf_GetValue(dict,"number",uint8_t,0);
	char name[] = "/spi_/exchange";
	name[4] = info->number + '0';
	info->fastHandle=Bus_CreateReceiverHandle(name);
}

//初始化spi缓冲区
void BSP_SPI_InitBuffer(SPIInfo* info, ConfItem* dict)
{

	info->SPIBuffer.data=pvPortMalloc(info->SPIBuffer.MaxSize);
	memset(info->SPIBuffer.data,0,info->SPIBuffer.MaxSize);

}

void BSP_SPI_SoftBusCallback(const char* name, SoftBusFrame* frame, void* bindData)
{
	
	  if(!Bus_CheckMapKeys(frame, {"chip","state"}))
		return;
		
		uint8_t *accel_tx = (uint8_t*)Bus_GetMapValue(frame, "accel_tx");
		uint16_t transSize_Acc = *(uint16_t*)Bus_GetMapValue(frame, "transSize_Acc");
    uint8_t *temp_tx = (uint8_t*)Bus_GetMapValue(frame, "temp_tx");
		uint16_t transSize_Tem = *(uint16_t*)Bus_GetMapValue(frame, "transSize_Tem");
		uint8_t *gyro_tx = (uint8_t*)Bus_GetMapValue(frame, "gyro_tx");
		uint16_t transSize_Gyr = *(uint16_t*)Bus_GetMapValue(frame, "transSize_Gyr");

		
		for(uint8_t i = 0; i < spiService.spiNum; i++)
		{
			SPIInfo* info = &spiService.spiList[i];

			  if(!strcmp(name, "/exti/pin4"))
				{
				char* chip = (char*)Bus_GetMapValue(frame, "chip");
				char*state = (char*)Bus_GetMapValue(frame, "state");
					if(!strcmp(state, "on"))
					{
						if(!strcmp(chip, "ACCEL"))
						{
						Bus_FastBroadcastSend(info->fastHandle,{"chip",chip});	
					  HAL_SPI_TransmitReceive_DMA(info->hspi,accel_tx,info->SPIBuffer.data,transSize_Acc);
						Bus_FastBroadcastSend(info->fastHandle,{"acc_buf",info->SPIBuffer.data});
						}
						else if(!strcmp(chip, "TEMP"))
						{
							Bus_FastBroadcastSend(info->fastHandle,{"chip",chip});	
						HAL_SPI_TransmitReceive_DMA(info->hspi,temp_tx,info->SPIBuffer.data,transSize_Tem);		
							Bus_FastBroadcastSend(info->fastHandle,{"tem_buf",info->SPIBuffer.data});
						}
					}
				}
				else if(!strcmp(name, "/exti/pin5"))
				{
				char* chip = (char*)Bus_GetMapValue(frame, "chip");
				char*state = (char*)Bus_GetMapValue(frame, "state");
					if(!strcmp(state, "on"))
					{
						if(!strcmp(chip, "GYRO"))
						{
							Bus_FastBroadcastSend(info->fastHandle,{"chip",chip});
				  		HAL_SPI_TransmitReceive_DMA(info->hspi,gyro_tx,info->SPIBuffer.data,transSize_Gyr);
							Bus_FastBroadcastSend(info->fastHandle,{"gyr_buf",info->SPIBuffer.data});
					
						}
					}
				}
			
		}
}
