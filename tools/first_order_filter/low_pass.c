#include "filter.h"
//低通滤波结构体
typedef struct
{
	Filter filter;

	float rate;
	float lastData;
} LowPassFilter;

float LowPass_Cala(Filter* filter, float data);

Filter* LowPass_Init(ConfItem* dict)
{
	LowPassFilter* lowPass = (LowPassFilter*)FILTER_MALLOC_PORT(sizeof(LowPassFilter));
	memset(lowPass, 0, sizeof(LowPassFilter));

	lowPass->filter.cala = LowPass_Cala;
	lowPass->rate = Conf_GetValue(dict, "rate", float, 1);
	lowPass->lastData = 0;

	return (Filter*)lowPass;
}

float LowPass_Cala(Filter* filter, float data)
{
	LowPassFilter *lowPass = (LowPassFilter*)filter;

	lowPass->lastData = lowPass->lastData * (1 - lowPass->rate) + data * lowPass->rate;
	return lowPass->lastData;
}
