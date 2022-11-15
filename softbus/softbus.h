#ifndef _SOFTBUS_H_
#define _SOFTBUS_H_

#include <stdint.h>
#include <stdarg.h>

typedef struct{
	void* data;
	uint16_t length;
}SoftBusFrame;//数据帧

typedef struct{
    char* key;
	void* data;
	uint16_t length;
}SoftBusItem;//数据字段

typedef void (*SoftBusCallback)(const char* topic, SoftBusFrame* frame, void* userData);//回调函数指针

//操作函数声明(不直接调用，应使用下方define定义的接口)
int8_t _SoftBus_MultiSubscribe(void* userData, SoftBusCallback callback, uint16_t topicsNum, char** topics);
void _SoftBus_Publish(const char* topic, SoftBusFrame* frame);
void _SoftBus_PublishMap(const char* topic, uint16_t itemNum, SoftBusItem* items);

/*
	@brief 订阅软总线上的一个话题
	@param callback:话题发布时的回调函数
	@param topic:话题名
	@retval 0:成功 -1:堆空间不足 -2:参数为空
	@note 回调函数的形式应为void callback(const char* topic, SoftBusFrame* frame)
*/
int8_t SoftBus_Subscribe(void* userData, SoftBusCallback callback, const char* topic);

/*
	@brief 订阅软总线上的多个话题
	@param callback:话题发布时的回调函数
	@param ...:话题名数组
	@retval 0:成功 -1:堆空间不足 -2:参数为空
	@example SoftBus_MultiSubscribe(callback,{"topic1","topic2"});
*/
#define SoftBus_MultiSubscribe(userData, callback,...) _SoftBus_MultiSubscribe((userData),(callback),(sizeof((char*[])__VA_ARGS__)/sizeof(char*)),((char*[])__VA_ARGS__))

/*
	@brief 在软总线上发布一个带有自定义格式数据的话题
	@param topic:话题名
	@param data:自定义格式数据
	@param len:自定义格式数据的长度(字节)
	@retval void
*/
#define SoftBus_Publish(topic,data,len) _SoftBus_Publish((topic),&(SoftBusFrame){(data),(len)})

/*
	@brief 在软总线上发布一个带有映射表的话题
	@param topic:话题名
	@param ...:映射表
	@retval void
	@example SoftBus_PublishMap("topic",{{"data1",data1,len},{"data2",data2,len}});
*/
#define SoftBus_PublishMap(topic,...) _SoftBus_PublishMap((topic),(sizeof((SoftBusItem[])__VA_ARGS__)/sizeof(SoftBusItem)),((SoftBusItem[])__VA_ARGS__))

/*
	@brief 获取数据帧里的数据字段
	@param frame:数据帧的指针
	@param key:数据字段的名字
	@retval 指向数据字段的const指针,不应该通过指针修改数据,若数据帧中查询不到key对应的数据字段则返回NULL
*/
const SoftBusItem* SoftBus_GetItem(SoftBusFrame* frame, char* key);

#endif
