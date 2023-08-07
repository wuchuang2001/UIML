#include "config.h"
#include "softbus.h"
#include "cmsis_os.h"

#include "gpio.h"
#include "spi.h"
#include <stdio.h>
#include <string.h>

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
	struct 
	{
		uint8_t *data;
		uint16_t maxBufSize;
		uint16_t dataLen;
	}recvBuffer;
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
bool BSP_SPI_DMACallback(const char* name, SoftBusFrame* frame, void* bindData);
bool BSP_SPI_BlockCallback(const char* name, SoftBusFrame* frame, void* bindData);

void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
	for(uint8_t num = 0; num < spiService.spiNum; num++)
	{
		SPIInfo* spiInfo = &spiService.spiList[num];
		if(hspi == spiInfo->hspi) //找到中断回调函数中对应spi列表的spi
		{
			Bus_FastBroadcastSend(spiInfo->fastHandle, {spiService.spiList->recvBuffer.data, &spiService.spiList->recvBuffer.dataLen}); //发送数据
			//将片选全部拉高
			for(uint8_t i = 0; i < spiService.spiList[num].csNum; i++)
			{
				HAL_GPIO_WritePin(spiService.spiList[num].csList[i].gpioX, spiService.spiList[num].csList[i].pin, GPIO_PIN_SET);
			}
			//解锁
			osSemaphoreRelease(spiService.spiList[num].lock);
			break;
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
	Bus_RegisterRemoteFunc(NULL, BSP_SPI_BlockCallback, "/spi/block");
	Bus_RegisterRemoteFunc(NULL,BSP_SPI_DMACallback, "/spi/trans/dma");
	spiService.initFinished = 1;
}
//初始化spi信息
void BSP_SPI_InitInfo(SPIInfo* info, ConfItem* dict)
{
	info->hspi = Conf_GetPtr(dict, "hspi", SPI_HandleTypeDef);
	info->number = Conf_GetValue(dict,"number",uint8_t,0);

	char name[17] = {0};
	sprintf(name, "/spi%d/recv", info->number);
	info->fastHandle=Bus_CreateReceiverHandle(name);
	osSemaphoreDef(lock);
	info->lock = osSemaphoreCreate(osSemaphore(lock), 1);
	//初始化片选引脚
	BSP_SPI_InitCS(info, Conf_GetPtr(dict, "cs", ConfItem));
	//初始化缓冲区
	info->recvBuffer.maxBufSize = Conf_GetValue(dict, "max-recv-size", uint16_t, 1);
	info->recvBuffer.data = pvPortMalloc(info->recvBuffer.maxBufSize);
    memset(info->recvBuffer.data,0,info->recvBuffer.maxBufSize);
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
    	//重新映射至GPIO_PIN=2^pin
		info->csList[num].pin =1<<Conf_GetValue(dict, confName, uint8_t, 0);
		sprintf(confName, "%d/name", num);
		info->csList[num].name = Conf_GetPtr(dict, confName, char);
		sprintf(confName, "%d/gpio-x", num);
		info->csList[num].gpioX = Conf_GetPtr(dict, confName, GPIO_TypeDef);
	}
}

bool BSP_SPI_DMACallback(const char* name, SoftBusFrame* frame, void* bindData)
{
	if(!Bus_CheckMapKeys(frame,{"spi-x", "tx-data", "len", "cs-name", "is-block"}))
		return false;
	uint8_t spiX = *(uint8_t *)Bus_GetMapValue(frame, "spi-x");
	uint8_t* txData = (uint8_t*)Bus_GetMapValue(frame, "tx-data");
	uint8_t* rxData = NULL;
	if(Bus_IsMapKeyExist(frame, "rx-data"))
		rxData = (uint8_t*)Bus_GetMapValue(frame, "rx-data"); //不检查该项是因为若为null则指向spi缓冲区
	uint16_t len = *(uint16_t*)Bus_GetMapValue(frame, "len");
	char* csName = (char*)Bus_GetMapValue(frame, "cs-name");
	uint32_t waitTime = (*(bool*)Bus_GetMapValue(frame, "is-block"))? osWaitForever: 0;
	for(uint8_t num = 0; num < spiService.spiNum; num++)
	{
		if(spiX == spiService.spiList[num].number) //找到对应的spi
		{
			if(rxData == NULL) //若未指定接收缓冲区则指向spi缓冲区
				rxData = spiService.spiList[num].recvBuffer.data;
			if(len > spiService.spiList[num].recvBuffer.maxBufSize) //若接收长度大于缓冲区长度，则返回错误
				return false;
			for (uint8_t i = 0; i < spiService.spiList[num].csNum; i++)
			{
				if(!strcmp(csName, spiService.spiList[num].csList[i].name)) //找到对应的片选引脚
				{
					//上锁
					if(osSemaphoreWait(spiService.spiList[num].lock, waitTime) != osOK)
						return false;
					HAL_GPIO_WritePin(spiService.spiList[num].csList[i].gpioX, spiService.spiList[num].csList[i].pin, GPIO_PIN_RESET); //拉低片选,开始通信
					HAL_SPI_TransmitReceive_DMA(spiService.spiList[num].hspi, txData, rxData, len); //开始传输
					return true;
				}
			}
			break;
		}
	}
	return false;
}

bool BSP_SPI_BlockCallback(const char* name, SoftBusFrame* frame, void* bindData)
{
	if(!Bus_CheckMapKeys(frame,{"spi-x", "tx-data", "len", "timeout", "cs-name", "is-block"}))
		return false;
	uint8_t spiX = *(uint8_t *)Bus_GetMapValue(frame, "spi-x");
	uint8_t* txData = (uint8_t*)Bus_GetMapValue(frame, "tx-data");
	uint8_t* rxData = NULL;
	if(Bus_IsMapKeyExist(frame, "rx-data"))
		rxData = (uint8_t*)Bus_GetMapValue(frame, "rx-data"); //不检查该项是因为若为null则指向spi缓冲区
	uint16_t len = *(uint16_t*)Bus_GetMapValue(frame, "len");
	uint32_t timeout = *(uint32_t*)Bus_GetMapValue(frame, "timeout");
	char* csName = (char*)Bus_GetMapValue(frame, "cs-name");
	uint32_t waitTime = (*(bool*)Bus_GetMapValue(frame, "is-block"))? osWaitForever: 0;
	for(uint8_t num = 0; num < spiService.spiNum; num++)
	{
		if(spiX == spiService.spiList[num].number)	//找到对应的spi
		{
			if(rxData == NULL) //若未指定接收缓冲区则指向spi缓冲区
				rxData = spiService.spiList[num].recvBuffer.data;
			if(len > spiService.spiList[num].recvBuffer.maxBufSize) //若接收长度大于缓冲区长度，则返回错误
				return false;
			spiService.spiList[num].recvBuffer.dataLen = len;
			for (uint8_t i = 0; i < spiService.spiList[num].csNum; i++) //找到对应的片选引脚
			{
				if(!strcmp(csName, spiService.spiList[num].csList[i].name))
				{
					//上锁
					if(osSemaphoreWait(spiService.spiList[num].lock, waitTime) != osOK)
						return false;
					HAL_GPIO_WritePin(spiService.spiList[num].csList[i].gpioX, spiService.spiList[num].csList[i].pin, GPIO_PIN_RESET); //拉低片选,开始通信
					HAL_SPI_TransmitReceive(spiService.spiList[num].hspi, txData, rxData, len, timeout); //开始传输
					HAL_GPIO_WritePin(spiService.spiList[num].csList[i].gpioX, spiService.spiList[num].csList[i].pin, GPIO_PIN_SET); //拉高片选,结束通信
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
