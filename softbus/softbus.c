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
    char* topic;
    Vector callbackNodes;
}TopicNode;//topic节点

typedef struct 
{
	char* service;
	CallbackNode callbackNode;
}ServiceNode;//service节点

typedef struct{
    uint32_t hash;
    Vector topicNodes;
	Vector serviceNodes;
}HashNode;//hash节点

int8_t SoftBus_Init(void);//初始化软总线,返回0:成功 -1:失败
uint32_t SoftBus_Str2Hash_8(const char* str);//8位处理的hash函数，在字符串长度小于20个字符时使用
uint32_t SoftBus_Str2Hash_32(const char* str);//32位处理的hash函数，在字符串长度小于20个字符时使用
void _SoftBus_Publish(const char* topic, SoftBusFrame* frame);//发布消息
bool _SoftBus_Request(const char* service, SoftBusFrame* request, void* response);//请求服务
void SoftBus_EmptyTopicCallback(const char* topic, SoftBusFrame* frame, void* bindData);//空回调函数
bool SoftBus_EmptyServiceCallback(const char* topic, SoftBusFrame* request, void* bindData, void* response);//空回调函数

Vector hashList={0};

int8_t SoftBus_Init()
{
    return Vector_Init(hashList,HashNode);
}

int8_t SoftBus_Subscribe(void* bindData, SoftBusTopicCallback callback, const char* topic)
{
	if(!topic || !callback)
		return -2;
	if(hashList.data == NULL)//如果软总线未初始化则初始化软总线
	{
		if(SoftBus_Init())
			return -1;
	}
	uint32_t hash = SoftBus_Str2Hash(topic);//计算字符串hash值
	Vector_ForEach(hashList, hashNode, HashNode)//遍历所有hash节点
	{
		if(hash == hashNode->hash)
		{
			Vector_ForEach(hashNode->topicNodes, topicNode, TopicNode)//遍历该hash节点下所有topic
			{
				if(strcmp(topic, topicNode->topic) == 0)//匹配到已有topic注册回调函数
				{
					if(Vector_GetFront(topicNode->callbackNodes, CallbackNode)->callback == SoftBus_EmptyTopicCallback)//如果该topic下有空回调函数
					{
						CallbackNode* callbackNode = Vector_GetFront(topicNode->callbackNodes, CallbackNode);
						callbackNode->bindData = bindData;//更新绑定数据
						callbackNode->callback = callback;//更新回调函数
						return 0;
					}
					return Vector_PushBack(topicNode->callbackNodes, ((CallbackNode){bindData, callback}));
				}
			}
			Vector callbackNodes = Vector_Create(CallbackNode);//未匹配到topic产生hash冲突，在该hash节点处添加一个topic节点解决hash冲突
			Vector_PushBack(callbackNodes, ((CallbackNode){bindData, callback}));
			char* topicCpy = SOFTBUS_MALLOC_PORT(SOFTBUS_STRLEN_PORT(topic)+1);
			SOFTBUS_MEMCPY_PORT(topicCpy, topic, SOFTBUS_STRLEN_PORT(topic)+1);
			return Vector_PushBack(hashNode->topicNodes,((TopicNode){topicCpy, callbackNodes}));
		}
	}
	Vector callbackNodes = Vector_Create(CallbackNode);//新的hash节点
	Vector_PushBack(callbackNodes, ((CallbackNode){bindData, callback}));
	Vector topicV = Vector_Create(TopicNode);
	Vector serviceV = Vector_Create(ServiceNode);
	char* topicCpy = SOFTBUS_MALLOC_PORT(SOFTBUS_STRLEN_PORT(topic)+1);
	SOFTBUS_MEMCPY_PORT(topicCpy, topic, SOFTBUS_STRLEN_PORT(topic)+1);
	char* serviceCpy = topicCpy;
	Vector_PushBack(topicV, ((TopicNode){topicCpy, callbackNodes}));
	Vector_PushBack(serviceV, ((ServiceNode){serviceCpy, ((CallbackNode){NULL, SoftBus_EmptyServiceCallback})}));
	return Vector_PushBack(hashList, ((HashNode){hash, topicV, serviceV}));
}

int8_t _SoftBus_MultiSubscribe(void* bindData, SoftBusTopicCallback callback, uint16_t topicsNum, char** topics)
{
	if(!topics || !topicsNum || !callback)
		return -2;
	for (uint16_t i = 0; i < topicsNum; i++)
	{
		uint8_t retval = SoftBus_Subscribe(bindData, callback, topics[i]); //逐个订阅话题
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

void _SoftBus_Publish(const char* topic, SoftBusFrame* frame)
{
	if(!hashList.data ||!topic || !frame)
		return;
	uint32_t hash = SoftBus_Str2Hash(topic);
	Vector_ForEach(hashList, hashNode, HashNode)//遍历所有hash节点
	{
		if(hash == hashNode->hash)//匹配到hash值
		{
			Vector_ForEach(hashNode->topicNodes, topicNode, TopicNode)//遍历改hash节点的所有topic
			{
				if(strcmp(topic, topicNode->topic) == 0)//匹配到topic，抛出该topic所有回调函数
				{
					Vector_ForEach(topicNode->callbackNodes, callbackNode, CallbackNode)
					{
						(*((SoftBusTopicCallback)callbackNode->callback))(topic, frame, callbackNode->bindData);
					}
					break;
				}
			}
			break;
		}
	}
}

void _SoftBus_PublishMap(const char* topic, uint16_t itemNum, SoftBusItem* items)
{
	if(!hashList.data ||!topic || !itemNum || !items)
		return;
	SoftBusFrame frame = {items, itemNum};
	_SoftBus_Publish(topic, &frame);
}

void _SoftBus_PublishList(SoftBusFastTopicHandle topicHandle, uint16_t listNum, void** list)
{
	if(!hashList.data || !listNum || !list)
		return;
	TopicNode* topicNode = (TopicNode*)topicHandle;
	SoftBusFrame frame = {list, listNum};
	Vector_ForEach(topicNode->callbackNodes, callbackNode, CallbackNode)
	{
		(*((SoftBusTopicCallback)callbackNode->callback))(topicNode->topic, &frame, callbackNode->bindData);
	}
}

SoftBusFastTopicHandle SoftBus_CreateFastTopicHandle(const char* topic)
{
	if(!topic)
		return NULL;
	uint32_t hash = SoftBus_Str2Hash(topic);//计算字符串hash值
	Vector_ForEach(hashList, hashNode, HashNode)//遍历所有hash节点
	{
		if(hash == hashNode->hash)
		{
			Vector_ForEach(hashNode->topicNodes, topicNode, TopicNode)//遍历该hash节点下所有topic
			{
				if(strcmp(topic, topicNode->topic) == 0)//匹配到已有topic注册回调函数
				{
					return topicNode;
				}
			}
		}
	}
	SoftBus_Subscribe(NULL, SoftBus_EmptyTopicCallback, topic);//未匹配到topic,注册一个空回调函数
	return SoftBus_CreateFastTopicHandle(topic);//递归调用
}

int8_t SoftBus_CreateServer(void* bindData, SoftBusServiceCallback callback, const char* service)
{
	if(!service || !callback)
		return -2;
	if(hashList.data == NULL)//如果软总线未初始化则初始化软总线
	{
		if(SoftBus_Init())
			return -1;
	}
	uint32_t hash = SoftBus_Str2Hash(service);//计算字符串hash值
	Vector_ForEach(hashList, hashNode, HashNode)//遍历所有hash节点
	{
		if(hash == hashNode->hash)
		{
			Vector_ForEach(hashNode->serviceNodes, serviceNode, ServiceNode)//遍历该hash节点下所有service
			{
				if(strcmp(service, serviceNode->service) == 0)//匹配到已有service注册回调函数
				{
					if(serviceNode->callbackNode.callback == SoftBus_EmptyServiceCallback)//如果该service下为空回调函数
					{
						serviceNode->callbackNode = ((CallbackNode){bindData, callback});//更新回调函数
						return 0;
					}
					return -3; //该服务已注册过服务器，不允许一个服务有多个服务器
				}
			}
			CallbackNode callbackNode = {bindData, callback};//未匹配到service产生hash冲突，在该hash节点处添加一个service节点解决hash冲突
			char* serviceCpy = SOFTBUS_MALLOC_PORT(SOFTBUS_STRLEN_PORT(service)+1);
			SOFTBUS_MEMCPY_PORT(serviceCpy, service, SOFTBUS_STRLEN_PORT(service)+1);
			return Vector_PushBack(hashNode->serviceNodes,((ServiceNode){serviceCpy, callbackNode}));
		}
	}

	Vector serviceV = Vector_Create(ServiceNode);//新的hash节点
	char* serviceCpy = SOFTBUS_MALLOC_PORT(SOFTBUS_STRLEN_PORT(service)+1);
	SOFTBUS_MEMCPY_PORT(serviceCpy, service, SOFTBUS_STRLEN_PORT(service)+1);
	Vector callbackNodes = Vector_Create(CallbackNode);
	Vector_PushBack(callbackNodes, ((CallbackNode){NULL, SoftBus_EmptyTopicCallback}));
	Vector topicV = Vector_Create(TopicNode);
	char* topicCpy = serviceCpy;
	Vector_PushBack(topicV, ((TopicNode){topicCpy, callbackNodes}));
	Vector_PushBack(serviceV, ((ServiceNode){serviceCpy, ((CallbackNode){bindData, callback})}));
	return Vector_PushBack(hashList, ((HashNode){hash, topicV, serviceV}));
}

bool _SoftBus_Request(const char* service, SoftBusFrame* request, void* response)
{
	if(!hashList.data ||!service || !request)
		return false;
	uint32_t hash = SoftBus_Str2Hash(service);
	Vector_ForEach(hashList, hashNode, HashNode)//遍历所有hash节点
	{
		if(hash == hashNode->hash)//匹配到hash值
		{
			Vector_ForEach(hashNode->serviceNodes, serviceNode, ServiceNode)//遍历改hash节点的所有service
			{
				if(strcmp(service, serviceNode->service) == 0)//匹配到service，抛出该service的回调函数
				{
					CallbackNode callbackNode = serviceNode->callbackNode;
					return (*((SoftBusServiceCallback)callbackNode.callback))(service, request, callbackNode.bindData, response);
				}
			}
			return false;
		}
	}
	return false;
}

bool _SoftBus_RequestMap(const char* service, void* response, uint16_t itemNum, SoftBusItem* items)
{
	if(!hashList.data ||!service || !response || !itemNum || !items)
		return false;
	SoftBusFrame frame = {items, itemNum};
	return _SoftBus_Request(service, &frame, response);
}

uint8_t _SoftBus_CheckMapKeys(SoftBusFrame* frame, uint16_t keysNum, char** keys)
{
	if(!frame || !keys || !keysNum)
		return 0;
	for(uint16_t i = 0; i < keysNum; ++i)
	{
		if(!SoftBus_GetMapItem(frame, keys[i]))
			return 0;
	}
	return 1;
}

const SoftBusItem* SoftBus_GetMapItem(SoftBusFrame* frame, char* key)
{
	for(uint16_t i = 0; i < frame->size; ++i)
	{
		SoftBusItem* item = (SoftBusItem*)frame->data + i;
		if(strcmp(key, item->key) == 0)//如果key值与数据帧中相应的字段匹配上则返回它
			return item;
	}
	return NULL;
}

void SoftBus_EmptyTopicCallback(const char* topic, SoftBusFrame* frame, void* bindData) { }
bool SoftBus_EmptyServiceCallback(const char* topic, SoftBusFrame* request, void* bindData, void* response) {return false;}
