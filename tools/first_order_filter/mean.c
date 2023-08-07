#include "filter.h"

//均值滤波结构体
typedef struct
{
	Filter filter;

	uint8_t index;
	float* array;
	uint8_t size;
	float sum;
}MeanFilter;

float Mean_Cala(Filter* filter, float data);

/**
  * @brief  初始化一个均值滤波器
  * @param  滤波器结构体
  * @param  需要关联到滤波器结构体的数组
  * @param  滤波器缓存区大小（滤波器内部用于存储数据的数组的大小）
  * @retval None
  */
Filter* Mean_Init(ConfItem* dict)
{
	MeanFilter* mean = (MeanFilter*)FILTER_MALLOC_PORT(sizeof(MeanFilter));
	memset(mean, 0, sizeof(MeanFilter));

	mean->filter.cala = Mean_Cala;
	mean->size = Conf_GetValue(dict, "size", uint8_t, 1);;
	mean->index = 0;
	mean->sum = 0;
	mean->array = (float*)FILTER_MALLOC_PORT(mean->size * sizeof(float));
	memset(mean->array, 0, mean->size * sizeof(float));

	return (Filter*)mean;
}

/**
  * @brief  均值滤波函数
  * @param  滤波器结构体，需要滤波的数据
  * @retval 滤波输出量
  * @attention None
  */
float Mean_Cala(Filter* filter, float data)
{
	MeanFilter *mean = (MeanFilter*)filter;
	
	mean->sum -= mean->array[mean->index];
	mean->sum += data;
	float retval = mean->sum / (float)mean->size;
	
	mean->array[mean->index++] = data;
	if(mean->index >= mean->size)
		mean->index = 0;

	return retval;
}
