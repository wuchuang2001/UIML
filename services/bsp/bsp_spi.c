#include "config.h"
#include "softbus.h"
#include "cmsis_os.h"

#include "spi.h"

//SPI句柄信息
typedef struct {
	SPI_HandleTypeDef* hspi;
	uint8_t number; //SPIX中的X
	SoftBusFastHandle fastHandle;
}SPIInfo;
typedef	struct 
{
	SPIInfo* SPIinfo;//数据缓冲区指的SPI
	uint8_t  *data;  	
	uint8_t *BufSize;
	uint8_t buffNum;
	uint8_t buffsize;
}SPIBuffer;


//SPI服务数据
typedef struct {
	SPIInfo* spiList;
	SPIBuffer* bufs;
	uint8_t spiNum;
	uint8_t initFinished;
}SPIService;

SPIService spiService={0};
//函数声明

void BSP_SPI_Init(ConfItem* dict);
void BSP_SPI_InitInfo(SPIInfo* info, ConfItem* dict);
void BSP_SPI_SoftBusCallback(const char* topic, SoftBusFrame* frame, void* bindData);
void BSP_SPI_InitBuffer(SPIBuffer* buffer, ConfItem* dict);



void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{

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
		BSP_SPI_InitBuffer(&spiService.bufs[num],Conf_GetPtr(dict, confName, ConfItem));

	}
	//订阅话题
	spiService.initFinished = 1;
}
//初始化spi信息
void BSP_SPI_InitInfo(SPIInfo* info, ConfItem* dict)
{
  info->hspi=Conf_GetPtr(dict,"hspi",SPI_HandleTypeDef);
	info->number=Conf_GetValue(dict,"number",uint8_t,0);
	char topic[] = "/spi_/exchange";
	topic[4] = info->number + '0';
	info->fastHandle=SoftBus_CreateFastHandle(topic);
}

//初始化spi缓冲区
void BSP_SPI_InitBuffer(SPIBuffer* buffer, ConfItem* dict)
{
	uint8_t spiX = Conf_GetValue(dict, "spi_num", uint8_t, 0);
	for(uint8_t i = 0; i < spiService.spiNum; ++i)
		if(spiService.spiList[i].number == spiX)
			buffer->SPIinfo = &spiService.spiList[i];
  //计算buff缓冲区数量		
	buffer->buffNum=0;	
	buffer->buffsize=0;
	for(uint8_t num = 0; ; num++)
	{
		char confName[] = "buff/size_";
		confName[9] = num + '0';
		if(Conf_ItemExist(dict, confName))
		{
			buffer->BufSize[num]=Conf_GetValue(dict,confName,uint8_t,0);
			buffer->buffsize+=buffer->BufSize[num];
			buffer->buffNum++;
		}
		else
			break;
	}
	buffer->data=pvPortMalloc(buffer->buffsize);
	memset(buffer->data,0,buffer->buffsize);

}

void BSP_SPI_SoftBusCallback(const char* topic, SoftBusFrame* frame, void* bindData)
{

}