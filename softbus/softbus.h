#ifndef _SOFTBUS_H_
#define _SOFTBUS_H_

#include <stdint.h>
#include <stdbool.h>

typedef struct{
	void* data;
	uint16_t size;
}SoftBusFrame;//数据帧

typedef struct{
    char* key;
	void* data;
}SoftBusItem;//数据字段

#ifndef IM_PTR
#define IM_PTR(type,...) (&(type){__VA_ARGS__}) //取立即数的地址
#endif

typedef void* SoftBusReceiverHandle;//软总线快速句柄
typedef void (*SoftBusBroadcastReceiver)(const char* name, SoftBusFrame* frame, void* bindData);//话题回调函数指针
typedef bool (*SoftBusRemoteFunction)(const char* name, SoftBusFrame* request, void* bindData);//服务回调函数指针

//操作函数声明(不直接调用，应使用下方define定义的接口)
int8_t _Bus_MultiRegisterReceiver(void* bindData, SoftBusBroadcastReceiver callback, uint16_t namesNum, char** names);
void _Bus_BroadcastSendMap(const char* name, uint16_t itemNum, SoftBusItem* items);
void _Bus_BroadcastSendList(SoftBusReceiverHandle receiverHandle, uint16_t listNum, void** list);
bool _Bus_RemoteCallMap(const char* name, uint16_t itemNum, SoftBusItem* items);
uint8_t _Bus_CheckMapKeys(SoftBusFrame* frame, uint16_t keysNum, char** keys);

/*
	@brief 订阅软总线上的一个话题
	@param callback:话题发布时的回调函数
	@param name:话题名
	@retval 0:成功 -1:堆空间不足 -2:参数为空
	@note 回调函数的形式应为void callback(const char* name, SoftBusFrame* frame, void* bindData)
*/
int8_t Bus_RegisterReceiver(void* bindData, SoftBusBroadcastReceiver callback, const char* name);

/*
	@brief 订阅软总线上的多个话题
	@param bindData:绑定数据
	@param callback:话题发布时的回调函数
	@param ...:话题字符串列表
	@retval 0:成功 -1:堆空间不足 -2:参数为空
	@example Bus_MultiRegisterReceiver(NULL, callback, {"name1", "name2"});
*/
#define Bus_MultiRegisterReceiver(bindData, callback,...) _Bus_MultiRegisterReceiver((bindData),(callback),(sizeof((char*[])__VA_ARGS__)/sizeof(char*)),((char*[])__VA_ARGS__))

/*
	@brief 以普通方式发布映射表数据帧
	@param name:话题字符串
	@param ...:映射表
	@retval void
	@example Bus_BroadcastSend("name", {{"key1", data1}, {"key2", data2}});
*/
#define Bus_BroadcastSend(name,...) _Bus_BroadcastSendMap((name),(sizeof((SoftBusItem[])__VA_ARGS__)/sizeof(SoftBusItem)),((SoftBusItem[])__VA_ARGS__))

/*
	@brief 以快速方式发送列表数据帧
	@param handle:快速句柄
	@param ...:数据指针列表
	@retval void
	@example float value1,value2; Bus_FastBroadcastSend(handle, {&value1, &value2});
*/
#define Bus_FastBroadcastSend(handle,...) _Bus_BroadcastSendList((handle),(sizeof((void*[])__VA_ARGS__)/sizeof(void*)),((void*[])__VA_ARGS__))

/*
	@brief 创建软总线上的一个服务
	@param callback:响应服务时的回调函数
	@param name:服务名
	@retval 0:成功 -1:堆空间不足 -2:参数为空 -3:服务已存在
	@note 回调函数的形式应为bool callback(const char* name, SoftBusFrame* request, void* bindData)
*/
int8_t Bus_RegisterRemoteFunc(void* bindData, SoftBusRemoteFunction callback, const char* name);

/*
	@brief 通过映射表数据帧请求服务
	@param name:服务名
	@param ...:请求数据帧
	@retval true:成功 false:失败
	@example Bus_RemoteCall("name", {{"key1", data1}, {"key2", data2}});
*/
#define Bus_RemoteCall(name,...) _Bus_RemoteCallMap((name),(sizeof((SoftBusItem[])__VA_ARGS__)/sizeof(SoftBusItem)),((SoftBusItem[])__VA_ARGS__))

/*
	@brief 查找映射表数据帧中的数据字段
	@param frame:数据帧的指针
	@param key:数据字段的名字
	@retval 指向指定数据字段的const指针,若查询不到key则返回NULL
	@note 不应对返回的数据帧进行修改
*/
const SoftBusItem* Bus_GetMapItem(SoftBusFrame* frame, char* key);

/*
	@brief 判断映射表数据帧中是否存在指定字段
	@param frame:数据帧的指针
	@param key:数据字段的名字
	@retval 0:不存在 1:存在
*/
#define Bus_IsMapKeyExist(frame,key) (Bus_GetMapItem((frame),(key)) != NULL)

/*
	@brief 判断给定key列表是否全部存在于映射表数据帧中
	@param frame:数据帧的指针
	@param ...:要判断的key列表
	@retval 0:任意一个key不存在 1:所有key都存在
	@example if(Bus_CheckMapKeys(frame, {"key1", "key2", "key3"})) { ... }
*/
#define Bus_CheckMapKeys(frame,...) _Bus_CheckMapKeys((frame),(sizeof((char*[])__VA_ARGS__)/sizeof(char*)),((char*[])__VA_ARGS__))

/*
	@brief 获取映射表数据帧中指定字段的值
	@param frame:数据帧的指针
	@param key:数据字段的名字
	@retval 指向值的(void*)型指针
	@note 必须确保传入的key存在于数据帧中，应先用相关接口进行检查
	@note 不应通过返回的指针修改指向的数据
	@example float value = *(float*)Bus_GetMapValue(frame, "key");
*/
#define Bus_GetMapValue(frame,key) (Bus_GetMapItem((frame),(key))->data)

/*
	@brief 通过话题字符串创建快速句柄
	@param name:话题字符串
	@retval 创建出的快速句柄
	@example SoftBusReceiverHandle handle = Bus_CreateReceiverHandle("name");
	@note 应仅在程序初始化时创建一次，而不是每次发布前创建
*/
SoftBusReceiverHandle Bus_CreateReceiverHandle(const char* name);

/*
	@brief 获取列表数据帧中指定索引的数据
	@param frame:数据帧的指针
	@param pos:数据在列表中的位置
	@retval 指向数据的(void*)型指针，若不存在则返回NULL
	@note 不应通过返回的指针修改指向的数据
	@example float value = *(float*)Bus_GetListValue(frame, 0); //获取列表中第一个值
*/
#define Bus_GetListValue(frame,pos) (((pos) < (frame)->size)?((void**)(frame)->data)[(pos)]:NULL)

#endif
