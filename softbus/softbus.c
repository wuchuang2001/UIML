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
    uint32_t hash;
    Vector topicNodes;
}HashNode;//hash节点

typedef struct{
    char* topic;
    Vector callbackNodes;
}TopicNode;//topic节点

typedef struct{
    void* bindData;
    SoftBusCallback callback;
}CallbackNode;//hash节点

int8_t SoftBus_Init(void);//初始化软总线,返回0:成功 -1:失败
uint32_t SoftBus_Str2Hash_8(const char* str);//8位处理的hash函数，在字符串长度小于20个字符时使用
uint32_t SoftBus_Str2Hash_32(const char* str);//32位处理的hash函数，在字符串长度小于20个字符时使用
void SoftBus_EmptyCallback(const char* topic, SoftBusFrame* frame, void* bindData);//空回调函数

Vector hashList={0};

int8_t SoftBus_Init()
{
    return Vector_Init(hashList,HashNode);
}

int8_t SoftBus_Subscribe(void* bindData, SoftBusCallback callback, const char* topic)
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
					if(Vector_GetFront(topicNode->callbackNodes, CallbackNode)->callback == SoftBus_EmptyCallback)//如果该topic下有空回调函数
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
	char* topicCpy = SOFTBUS_MALLOC_PORT(SOFTBUS_STRLEN_PORT(topic)+1);
	SOFTBUS_MEMCPY_PORT(topicCpy, topic, SOFTBUS_STRLEN_PORT(topic)+1);
	Vector_PushBack(topicV, ((TopicNode){topicCpy, callbackNodes}));
	return Vector_PushBack(hashList, ((HashNode){hash, topicV}));
}

int8_t _SoftBus_MultiSubscribe(void* bindData, SoftBusCallback callback, uint16_t topicsNum, char** topics)
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
						(*(callbackNode->callback))(topic, frame, callbackNode->bindData);
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

const SoftBusItem* SoftBus_GetMapItem(SoftBusFrame* frame, char* key)
{
	for(uint16_t i = 0; i < frame->size; ++i)
	{
		SoftBusItem* item = (SoftBusItem*)frame->data;
		if(strcmp(key, item->key) == 0)//如果key值与数据帧中相应的字段匹配上则返回它
			return item;
	}
	return NULL;
}

SoftBusFastHandle SoftBus_CreateFastHandle(const char* topic)
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
	SoftBus_Subscribe(NULL, SoftBus_EmptyCallback, topic);//未匹配到topic,注册一个空回调函数
	return SoftBus_CreateFastHandle(topic);//递归调用
}

void _SoftBus_PublishList(SoftBusFastHandle topicHandle, uint16_t listNum, void** list)
{
	if(!hashList.data || !listNum || !list)
		return;
	TopicNode* topicNode = (TopicNode*)topicHandle;
	SoftBusFrame frame = {list, listNum};
	Vector_ForEach(topicNode->callbackNodes, callbackNode, CallbackNode)
	{
		(*(callbackNode->callback))(topicNode->topic, &frame, callbackNode->bindData);
	}
}

void* _SoftBus_GetListValue(SoftBusFrame* frame, uint16_t pos)
{
	if(!frame || pos >= frame->size)
		return NULL;
	return ((void**)frame->data)[pos];
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

void SoftBus_EmptyCallback(const char* topic, SoftBusFrame* frame, void* bindData) { }
