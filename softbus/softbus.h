#ifndef _SOFTBUS_H_
#define _SOFTBUS_H_

#include <stdint.h>

typedef struct{
	void* data;
	uint16_t size;
}SoftBusFrame;//数据帧

typedef struct{
    char* key;
	void* data;
}SoftBusItem;//数据字段

typedef void* SoftBusFastHandle;//软总线快速句柄
typedef void (*SoftBusCallback)(const char* topic, SoftBusFrame* frame, void* bindData);//回调函数指针

//操作函数声明(不直接调用，应使用下方define定义的接口)
int8_t _SoftBus_MultiSubscribe(void* bindData, SoftBusCallback callback, uint16_t topicsNum, char** topics);
void _SoftBus_PublishMap(const char* topic, uint16_t itemNum, SoftBusItem* items);
void _SoftBus_PublishList(SoftBusFastHandle topicHandle, uint16_t listNum, void** list);
void* _SoftBus_GetListValue(SoftBusFrame* frame, uint16_t pos);
uint8_t _SoftBus_CheckMapKeys(SoftBusFrame* frame, uint16_t keysNum, char** keys);

/*
	@brief 订阅软总线上的一个话题
	@param callback:话题发布时的回调函数
	@param topic:话题名
	@retval 0:成功 -1:堆空间不足 -2:参数为空
	@note 回调函数的形式应为void callback(const char* topic, SoftBusFrame* frame)
*/
int8_t SoftBus_Subscribe(void* bindData, SoftBusCallback callback, const char* topic);

/*
	@brief 订阅软总线上的多个话题
	@param callback:话题发布时的回调函数
	@param ...:话题名数组
	@retval 0:成功 -1:堆空间不足 -2:参数为空
	@example SoftBus_MultiSubscribe(callback,{"topic1","topic2"});
*/
#define SoftBus_MultiSubscribe(bindData, callback,...) _SoftBus_MultiSubscribe((bindData),(callback),(sizeof((char*[])__VA_ARGS__)/sizeof(char*)),((char*[])__VA_ARGS__))

/*
	@brief 在软总线上发布一个带有映射表的话题
	@param topic:话题名
	@param ...:映射表
	@retval void
	@example SoftBus_PublishMap("topic",{{"data1",data1,len},{"data2",data2,len}});
*/
#define SoftBus_Publish(topic,...) _SoftBus_PublishMap((topic),(sizeof((SoftBusItem[])__VA_ARGS__)/sizeof(SoftBusItem)),((SoftBusItem[])__VA_ARGS__))

/*
	@brief 获取数据帧里的映射表的数据字段
	@param frame:数据帧的指针
	@param key:数据字段的名字
	@retval 指向数据字段的const指针,不应该通过指针修改数据,若数据帧中查询不到key对应的数据字段则返回NULL
*/
const SoftBusItem* SoftBus_GetMapItem(SoftBusFrame* frame, char* key);

/*
	@brief 判断数据帧中数据字段是否存在
	@param frame:数据帧的指针
	@param key:数据字段的名字
	@retval 0:不存在 1:存在
*/
#define SoftBus_IsMapKeyExist(frame,key) (SoftBus_GetMapItem((frame),(key)) != NULL)

/*
	@brief 判断数据帧中某些数据字段是否存在
	@param frame:数据帧的指针
	@param ...:要判断的数据字段的名字
	@retval 0:不存在 1:存在
*/
#define SoftBus_CheckMapKeys(frame,...) _SoftBus_CheckMapKeys((frame),(sizeof((char*[])__VA_ARGS__)/sizeof(char*)),((char*[])__VA_ARGS__))

/*
	@brief 获取数据帧里的映射表的值的指针
	@param frame:数据帧的指针
	@param key:数据字段的名字
	@param type:值的类型
	@retval 指向值的指针,不应该通过指针修改数据,若数据帧中查询不到key对应的值则返回NULL
*/
#define SoftBus_GetMapValue(frame,key) (SoftBus_GetMapItem((frame),(key))->data)

/*
	@brief 获取软总线上一个话题的快速句柄
	@param topic:话题名
	@retval 快速句柄
*/
SoftBusFastHandle SoftBus_CreateFastHandle(const char* topic);

/*
	@brief 在软总线上通过快速句柄发布一个带有列表数据的话题
	@param handle:快速句柄
	@param ...:列表数据
	@retval void
*/
#define SoftBus_FastPublish(handle,...) _SoftBus_PublishList((handle),(sizeof((void*[])__VA_ARGS__)/sizeof(void*)),((void*[])__VA_ARGS__))

/*
	@brief 获取数据帧里列表中的数据指针
	@param frame:数据帧的指针
	@param pos:数据在列表中的位置
	@retval 指向数据的(type*)型指针，若不存在则返回NULL
*/
#define SoftBus_GetListValue(frame,pos) (_SoftBus_GetListValue((frame),(pos)))

#endif
