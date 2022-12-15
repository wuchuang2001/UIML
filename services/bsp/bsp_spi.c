#include "config.h"
#include "softbus.h"
#include "cmsis_os.h"

#include "gpio.h"
#include "spi.h"
#include <stdio.h>

typedef struct 
{
	char* name;
	GPIO_TypeDef* gpioX;
	uint16_t pin;
}CSInfo;

//SPI句柄信息
typedef struct {
	SPI_HandleTypeDef* hspi;
	uint8_t number; //SPIX中的X

	osSemaphoreId lock;
	CSInfo* csList;
	uint8_t csNum;
	SoftBusReceiverHandle fastHandle;
}SPIInfo;

//SPI服务数据
typedef struct {
	SPIInfo* spiList;
	uint8_t spiNum;
	uint8_t initFinished;
}SPIService;

SPIService spiService = {0};
//函数声明

void BSP_SPI_Init(ConfItem* dict);
void BSP_SPI_InitInfo(SPIInfo* info, ConfItem* dict);
void BSP_SPI_InitCS(SPIInfo* info, ConfItem* dict);
bool BSP_SPI_DMA_RemoteCallback(const char* name, SoftBusFrame* frame, void* bindData);
bool BSP_SPI_Block_RemoteCallback(const char* name, SoftBusFrame* frame, void* bindData);

void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
	for(uint8_t num = 0; num < spiService.spiNum; num++)
	{
		SPIInfo* spiInfo = &spiService.spiList[num];
		if(hspi == spiInfo->hspi)
		{
			Bus_FastBroadcastSend(spiInfo->fastHandle, {""});
			for(uint8_t i = 0; i < spiService.spiList[num].csNum; i++)
			{
				HAL_GPIO_WritePin(spiService.spiList[num].csList[i].gpioX, spiService.spiList[num].csList[i].pin, GPIO_PIN_SET);
			}
			//解锁
			osSemaphoreRelease(spiService.spiList[num].lock);
		}
	}
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
		char confName[8] = {0};
		sprintf(confName, "spis/%d", num);
		if(Conf_ItemExist(dict, confName))
			spiService.spiNum++;
		else
			break;
	}
	
	//初始化各spi信息
	spiService.spiList = pvPortMalloc(spiService.spiNum * sizeof(SPIInfo));
	for(uint8_t num = 0; num < spiService.spiNum; num++)
	{
		char confName[8] = {0};
		sprintf(confName, "spis/%d", num);
		BSP_SPI_InitInfo(&spiService.spiList[num], Conf_GetPtr(dict, confName, ConfItem));
	}

	//注册远程服务
	Bus_RegisterRemoteFunc(NULL, BSP_SPI_Block_RemoteCallback, "/spi/block");
	Bus_RegisterRemoteFunc(NULL,BSP_SPI_DMA_RemoteCallback, "/spi/trans/dma");
	spiService.initFinished = 1;
}
//初始化spi信息
void BSP_SPI_InitInfo(SPIInfo* info, ConfItem* dict)
{
	info->hspi = Conf_GetPtr(dict, "hspi", SPI_HandleTypeDef);
	info->number = Conf_GetValue(dict,"number",uint8_t,0);

	char name[17] = {0};
	sprintf(name, "/spi%d/recvFinish", info->number);
	info->fastHandle=Bus_CreateReceiverHandle(name);
	osSemaphoreDef(lock);
	info->lock = osSemaphoreCreate(osSemaphore(lock), 1);
	//初始化片选引脚
	BSP_SPI_InitCS(info, Conf_GetPtr(dict, "cs", ConfItem));
}

//初始化片选引脚
void BSP_SPI_InitCS(SPIInfo* info, ConfItem* dict)
{
	for(uint8_t num = 0; ; num++)
	{
		char confName[4] = {0};
		sprintf(confName, "%d", num);
		if(Conf_ItemExist(dict, confName))
			info->csNum++;
		else
			break;
	}
	//初始化各spi信息
	info->csList = pvPortMalloc(info->csNum * sizeof(CSInfo));
	for(uint8_t num = 0; num < info->csNum; num++)
	{
		char confName[11] = {0};
		sprintf(confName, "%d/pin", num);
		info->csList[num].pin = Conf_GetValue(dict, confName, uint16_t, 0);
		sprintf(confName, "%d/name", num);
		info->csList[num].name = Conf_GetPtr(dict, confName, char);
		sprintf(confName, "%d/gpio-x", num);
		info->csList[num].gpioX = Conf_GetPtr(dict, confName, GPIO_TypeDef);
	}
}

bool BSP_SPI_DMA_RemoteCallback(const char* name, SoftBusFrame* frame, void* bindData)
{
	if(!Bus_CheckMapKeys(frame,{"spi-x", "txData", "rxData", "len", "csName", "isBlock"}))
		return false;
	uint8_t spiX = *(uint8_t *)Bus_GetMapValue(frame, "spi-x");
	uint8_t* txData = (uint8_t*)Bus_GetMapValue(frame, "txData");
	uint8_t* rxData = (uint8_t*)Bus_GetMapValue(frame, "rxData");
	uint16_t len = *(uint16_t*)Bus_GetMapValue(frame, "len");
	char* csName = (char*)Bus_GetMapValue(frame, "csName");
	uint32_t waitTime = (*(bool*)Bus_GetMapValue(frame, "isBlock"))? osWaitForever: 0;
	for(uint8_t num = 0; num < spiService.spiNum; num++)
	{
		if(spiX == spiService.spiList[num].number)
		{
			for (uint8_t i = 0; i < spiService.spiList[num].csNum; i++)
			{
				if(!strcmp(csName, spiService.spiList[num].csList[i].name))
				{
					//上锁
					if(osSemaphoreWait(spiService.spiList[num].lock, waitTime) != osOK)
						return false;
					HAL_GPIO_WritePin(spiService.spiList[num].csList[i].gpioX, spiService.spiList[num].csList[i].pin, GPIO_PIN_RESET);
					HAL_SPI_TransmitReceive_DMA(spiService.spiList[num].hspi, txData, rxData, len);
					return true;
				}
			}
			break;
		}
	}
	return false;
}

bool BSP_SPI_Block_RemoteCallback(const char* name, SoftBusFrame* frame, void* bindData)
{
	if(!Bus_CheckMapKeys(frame,{"spi-x", "txData", "rxData", "len", "timeout", "csName", "isBlock"}))
		return false;
	uint8_t spiX = *(uint8_t *)Bus_GetMapValue(frame, "spi-x");
	uint8_t* txData = (uint8_t*)Bus_GetMapValue(frame, "txData");
	uint8_t* rxData = (uint8_t*)Bus_GetMapValue(frame, "rxData");
	uint16_t len = *(uint16_t*)Bus_GetMapValue(frame, "len");
	uint32_t timeout = *(uint32_t*)Bus_GetMapValue(frame, "timeout");
	char* csName = (char*)Bus_GetMapValue(frame, "csName");
	uint32_t waitTime = (*(bool*)Bus_GetMapValue(frame, "isBlock"))? osWaitForever: 0;
	for(uint8_t num = 0; num < spiService.spiNum; num++)
	{
		if(spiX == spiService.spiList[num].number)
		{
			for (uint8_t i = 0; i < spiService.spiList[num].csNum; i++)
			{
				if(!strcmp(csName, spiService.spiList[num].csList[i].name))
				{
					//上锁
					if(osSemaphoreWait(spiService.spiList[num].lock, waitTime) != osOK)
						return false;
					HAL_GPIO_WritePin(spiService.spiList[num].csList[i].gpioX, spiService.spiList[num].csList[i].pin, GPIO_PIN_RESET);
					HAL_SPI_TransmitReceive(spiService.spiList[num].hspi, txData, rxData, len, timeout);
					HAL_GPIO_WritePin(spiService.spiList[num].csList[i].gpioX, spiService.spiList[num].csList[i].pin, GPIO_PIN_SET);
					//解锁
					osSemaphoreRelease(spiService.spiList[num].lock);
					return true;
				}
			}
			break;
		}
	}
	return false;
}
