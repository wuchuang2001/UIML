#include "filter.h"

//X-MACRO
//子类列表，每一项格式为(类型名,初始化函数名)
#define FILTER_CHILD_LIST \
	FILTER_TYPE("kalman",Kalman_Init) \
	FILTER_TYPE("mean",Mean_Init) \
	FILTER_TYPE("low-pass",LowPass_Init)

//声明子类初始化函数
#define FILTER_TYPE(name,initFunc) extern Filter* initFunc(ConfItem*);
FILTER_CHILD_LIST
#undef FILTER_TYPE

//内部函数声明
void Filter_InitDefault(Filter* filter);
float Filter_Cala(Filter *filter, float data);

Filter* Filter_Init(ConfItem* dict)
{
	char* filterType = Conf_GetPtr(dict, "type", char);

	Filter *filter = NULL;
	//判断属于哪个子类
	#define FILTER_TYPE(name,initFunc) \
	if(!strcmp(filterType, name)) \
		filter = initFunc(dict);
	FILTER_CHILD_LIST
	#undef MOTOR_TYPE
	if(!filter)
		filter = FILTER_MALLOC_PORT(sizeof(Filter));
	
	//将子类未定义的方法设置为空函数
	Filter_InitDefault(filter);

	return filter;
}

void Filter_InitDefault(Filter* filter)
{
	if(!filter->cala)
		filter->cala = Filter_Cala;
}

//纯虚函数
float Filter_Cala(Filter *filter, float data) {return 0;}
