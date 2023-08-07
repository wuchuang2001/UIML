#include "softbus.h"
#include "vector.h"
#include "cmsis_os.h"

#include <string.h>
#include <stdlib.h>

#define SOFTBUS_MALLOC_PORT(len) pvPortMalloc(len)
#define SOFTBUS_FREE_PORT(ptr) vPortFree(ptr)
#define SOFTBUS_MEMCPY_PORT(dst,src,len) memcpy(dst,src,len)
#define SOFTBUS_STRLEN_PORT(str) strlen(str)

#define SoftBus_Str2Hash(str) SoftBus_Str2Hash_8(str)

typedef struct{
    void* bindData;
    void* callback;
}CallbackNode;//回调函数节点

typedef struct{
    char* name;
    Vector callbackNodes;
}ReceiverNode;//receiver节点

typedef struct 
{
	char* name;
	CallbackNode callbackNode;
}RemoteNode;//remote节点

typedef struct{
    uint32_t hash;
    Vector receiverNodes;
	Vector remoteNodes;
}HashNode;//hash节点

int8_t Bus_Init(void);//初始化软总线,返回0:成功 -1:失败
uint32_t SoftBus_Str2Hash_8(const char* str);//8位处理的hash函数，在字符串长度小于20个字符时使用
uint32_t SoftBus_Str2Hash_32(const char* str);//32位处理的hash函数，在字符串长度小于20个字符时使用
void _Bus_BroadcastSend(const char* name, SoftBusFrame* frame);//发布消息
bool _Bus_RemoteCall(const char* name, SoftBusFrame* frame);//请求服务
void Bus_EmptyBroadcastReceiver(const char* name, SoftBusFrame* frame, void* bindData);//空回调函数
bool Bus_EmptyRemoteFunction(const char* name, SoftBusFrame* frame, void* bindData);//空回调函数

Vector hashList={0};
//初始化hash树
int8_t Bus_Init()
{
    return Vector_Init(hashList,HashNode);
}

int8_t Bus_RegisterReceiver(void* bindData, SoftBusBroadcastReceiver callback, const char* name)
{
	if(!name || !callback)
		return -2;
	if(hashList.data == NULL)//如果软总线未初始化则初始化软总线
	{
		if(Bus_Init())
			return -1;
	}
	uint32_t hash = SoftBus_Str2Hash(name);//计算字符串hash值
	Vector_ForEach(hashList, hashNode, HashNode)//遍历所有hash节点
	{
		if(hash == hashNode->hash)
		{
			Vector_ForEach(hashNode->receiverNodes, receiverNode, ReceiverNode)//遍历该hash节点下所有receiver
			{
				if(strcmp(name, receiverNode->name) == 0)//匹配到已有receiver注册回调函数
				{
					if(Vector_GetFront(receiverNode->callbackNodes, CallbackNode)->callback == Bus_EmptyBroadcastReceiver)//如果该receiver下有空回调函数
					{
						CallbackNode* callbackNode = Vector_GetFront(receiverNode->callbackNodes, CallbackNode);
						callbackNode->bindData = bindData;//更新绑定数据
						callbackNode->callback = callback;//更新回调函数
						return 0;
					}
					return Vector_PushBack(receiverNode->callbackNodes, ((CallbackNode){bindData, callback}));
				}
			}
			Vector callbackNodes = Vector_Create(CallbackNode);//未匹配到receiver产生hash冲突，在该hash节点处添加一个receiver节点解决hash冲突
			Vector_PushBack(callbackNodes, ((CallbackNode){bindData, callback}));
			char* nameCpy = SOFTBUS_MALLOC_PORT(SOFTBUS_STRLEN_PORT(name)+1);//防止name是局部变量，分配空间保存到hash树中
			SOFTBUS_MEMCPY_PORT(nameCpy, name, SOFTBUS_STRLEN_PORT(name)+1);
			return Vector_PushBack(hashNode->receiverNodes,((ReceiverNode){nameCpy, callbackNodes}));
		}
	}
	Vector callbackNodes = Vector_Create(CallbackNode);//新的hash节点
	Vector_PushBack(callbackNodes, ((CallbackNode){bindData, callback}));
	Vector receiverV = Vector_Create(ReceiverNode);
	Vector remoteV = Vector_Create(RemoteNode);
	char* nameCpy = SOFTBUS_MALLOC_PORT(SOFTBUS_STRLEN_PORT(name)+1);
	SOFTBUS_MEMCPY_PORT(nameCpy, name, SOFTBUS_STRLEN_PORT(name)+1);
	Vector_PushBack(receiverV, ((ReceiverNode){nameCpy, callbackNodes}));
	Vector_PushBack(remoteV, ((RemoteNode){nameCpy, ((CallbackNode){NULL, Bus_EmptyRemoteFunction})}));
	return Vector_PushBack(hashList, ((HashNode){hash, receiverV, remoteV}));
}

int8_t _Bus_MultiRegisterReceiver(void* bindData, SoftBusBroadcastReceiver callback, uint16_t namesNum, char** names)
{
	if(!names || !namesNum || !callback)
		return -2;
	for (uint16_t i = 0; i < namesNum; i++)
	{
		uint8_t retval = Bus_RegisterReceiver(bindData, callback, names[i]); //逐个订阅话题
		if(retval)
			return retval;
	}
	return 0;
}

uint32_t SoftBus_Str2Hash_8(const char* str)  
{
    uint32_t h = 0;  
	for(uint16_t i = 0; str[i] != '\0'; ++i)  
		h = (h << 5) - h + str[i];  
    return h;  
}

uint32_t SoftBus_Str2Hash_32(const char* str)  
{
	uint32_t h = 0;  
	uint16_t strLength = strlen(str),alignedLen = strLength/sizeof(uint32_t);
	for(uint16_t i = 0; i < alignedLen; ++i)  
		h = (h << 5) - h + ((uint32_t*)str)[i]; 
	for(uint16_t i = alignedLen << 2; i < strLength; ++i)
		h = (h << 5) - h + str[i]; 
    return h; 
}

void _Bus_BroadcastSend(const char* name, SoftBusFrame* frame)
{
	if(!hashList.data ||!name || !frame)
		return;
	uint32_t hash = SoftBus_Str2Hash(name);
	Vector_ForEach(hashList, hashNode, HashNode)//遍历所有hash节点
	{
		if(hash == hashNode->hash)//匹配到hash值
		{
			Vector_ForEach(hashNode->receiverNodes, receiverNode, ReceiverNode)//遍历改hash节点的所有receiver
			{
				if(strcmp(name, receiverNode->name) == 0)//匹配到receiver，抛出该receiver所有回调函数
				{
					Vector_ForEach(receiverNode->callbackNodes, callbackNode, CallbackNode)
					{
						(*((SoftBusBroadcastReceiver)callbackNode->callback))(name, frame, callbackNode->bindData);
					}
					break;
				}
			}
			break;
		}
	}
}

void _Bus_BroadcastSendMap(const char* name, uint16_t itemNum, SoftBusItem* items)
{
	if(!hashList.data ||!name || !itemNum || !items)
		return;
	SoftBusFrame frame = {items, itemNum};
	_Bus_BroadcastSend(name, &frame);
}

void _Bus_BroadcastSendList(SoftBusReceiverHandle receiverHandle, uint16_t listNum, void** list)
{
	if(!hashList.data || !listNum || !list)
		return;
	ReceiverNode* receiverNode = (ReceiverNode*)receiverHandle;
	SoftBusFrame frame = {list, listNum};
	Vector_ForEach(receiverNode->callbackNodes, callbackNode, CallbackNode)//抛出该快速句柄下所有回调函数
	{
		(*((SoftBusBroadcastReceiver)callbackNode->callback))(receiverNode->name, &frame, callbackNode->bindData);
	}
}

SoftBusReceiverHandle Bus_CreateReceiverHandle(const char* name)
{
	if(!name)
		return NULL;
	uint32_t hash = SoftBus_Str2Hash(name);//计算字符串hash值
	Vector_ForEach(hashList, hashNode, HashNode)//遍历所有hash节点
	{
		if(hash == hashNode->hash)
		{
			Vector_ForEach(hashNode->receiverNodes, receiverNode, ReceiverNode)//遍历该hash节点下所有receiver
			{
				if(strcmp(name, receiverNode->name) == 0)//匹配到已有receiver注册回调函数
				{
					return receiverNode;
				}
			}
		}
	}
	Bus_RegisterReceiver(NULL, Bus_EmptyBroadcastReceiver, name);//未匹配到receiver,注册一个空回调函数
	return Bus_CreateReceiverHandle(name);//递归调用
}

int8_t Bus_RegisterRemoteFunc(void* bindData, SoftBusRemoteFunction callback, const char* name)
{
	if(!name || !callback)
		return -2;
	if(hashList.data == NULL)//如果软总线未初始化则初始化软总线
	{
		if(Bus_Init())
			return -1;
	}
	uint32_t hash = SoftBus_Str2Hash(name);//计算字符串hash值
	Vector_ForEach(hashList, hashNode, HashNode)//遍历所有hash节点
	{
		if(hash == hashNode->hash)
		{
			Vector_ForEach(hashNode->remoteNodes, remoteNode, RemoteNode)//遍历该hash节点下所有remote
			{
				if(strcmp(name, remoteNode->name) == 0)//匹配到已有remote注册回调函数
				{
					if(remoteNode->callbackNode.callback == Bus_EmptyRemoteFunction)//如果该remote下为空回调函数
					{
						remoteNode->callbackNode = ((CallbackNode){bindData, callback});//更新回调函数
						return 0;
					}
					return -3; //该服务已注册过服务器，不允许一个服务有多个服务器
				}
			}
			CallbackNode callbackNode = {bindData, callback};//未匹配到remote产生hash冲突，在该hash节点处添加一个remote节点解决hash冲突
			char* remoteCpy = SOFTBUS_MALLOC_PORT(SOFTBUS_STRLEN_PORT(name)+1);
			SOFTBUS_MEMCPY_PORT(remoteCpy, name, SOFTBUS_STRLEN_PORT(name)+1);
			return Vector_PushBack(hashNode->remoteNodes,((RemoteNode){remoteCpy, callbackNode}));
		}
	}

	Vector remoteV = Vector_Create(RemoteNode);//新的hash节点
	char* nameCpy = SOFTBUS_MALLOC_PORT(SOFTBUS_STRLEN_PORT(name)+1);
	SOFTBUS_MEMCPY_PORT(nameCpy, name, SOFTBUS_STRLEN_PORT(name)+1);
	Vector callbackNodes = Vector_Create(CallbackNode);
	Vector_PushBack(callbackNodes, ((CallbackNode){NULL, Bus_EmptyBroadcastReceiver}));
	Vector receiverV = Vector_Create(ReceiverNode);
	Vector_PushBack(receiverV, ((ReceiverNode){nameCpy, callbackNodes}));
	Vector_PushBack(remoteV, ((RemoteNode){nameCpy, ((CallbackNode){bindData, callback})}));
	return Vector_PushBack(hashList, ((HashNode){hash, receiverV, remoteV}));
}

bool _Bus_RemoteCall(const char* name, SoftBusFrame* frame)
{
	if(!hashList.data ||!name || !frame)
		return false;
	uint32_t hash = SoftBus_Str2Hash(name);
	Vector_ForEach(hashList, hashNode, HashNode)//遍历所有hash节点
	{
		if(hash == hashNode->hash)//匹配到hash值
		{
			Vector_ForEach(hashNode->remoteNodes, remoteNode, RemoteNode)//遍历改hash节点的所有remote
			{
				if(strcmp(name, remoteNode->name) == 0)//匹配到remote，抛出该remote的回调函数
				{
					CallbackNode callbackNode = remoteNode->callbackNode;
					return (*((SoftBusRemoteFunction)callbackNode.callback))(name, frame, callbackNode.bindData);
				}
			}
			return false;
		}
	}
	return false;
}

bool _Bus_RemoteCallMap(const char* name, uint16_t itemNum, SoftBusItem* items)
{
	if(!hashList.data ||!name || !itemNum || !items)
		return false;
	SoftBusFrame frame = {items, itemNum};
	return _Bus_RemoteCall(name, &frame);
}

uint8_t _Bus_CheckMapKeys(SoftBusFrame* frame, uint16_t keysNum, char** keys)
{
	if(!frame || !keys || !keysNum)
		return 0;
	for(uint16_t i = 0; i < keysNum; ++i)
	{
		if(!Bus_GetMapItem(frame, keys[i]))
			return 0;
	}
	return 1;
}

const SoftBusItem* Bus_GetMapItem(SoftBusFrame* frame, char* key)
{
	for(uint16_t i = 0; i < frame->size; ++i)
	{
		SoftBusItem* item = (SoftBusItem*)frame->data + i;
		if(strcmp(key, item->key) == 0)//如果key值与数据帧中相应的字段匹配上则返回它
			return item;
	}
	return NULL;
}

void Bus_EmptyBroadcastReceiver(const char* name, SoftBusFrame* frame, void* bindData) { }
bool Bus_EmptyRemoteFunction(const char* name, SoftBusFrame* frame, void* bindData) {return false;}
