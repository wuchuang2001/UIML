/****************通用循环队列*****************/

#include "my_queue.h"
#include <string.h>
#include <stdlib.h>

//初始化队列
void Queue_Init(Queue *queue,uint16_t maxSize)
{
	queue->data=malloc(maxSize*sizeof(void*));
	if(queue->data)
		queue->initialized=1;
	queue->maxSize=maxSize;
}

//附加上一个数据保存区(入队时会将数据复制到保存区对应位置)
//注意保存区大小不得小于队列长度，否则会越界
void Queue_AttachBuffer(Queue *queue,void *buffer,uint8_t elemSize)
{
	if(!queue->initialized) return;
	queue->buffer=buffer;
	queue->bufElemSize=elemSize;
}

//销毁一个队列
void Queue_Destroy(Queue *queue)
{
	if(!queue->initialized) return;
	free(queue->data);
	memset(queue,0,sizeof(Queue));
}

//获取队列长度
uint16_t Queue_Size(Queue *queue)
{
	if(!queue->initialized) return 0;
	return (queue->rear+queue->maxSize-queue->front)%queue->maxSize;
}

//队列是否为满
uint8_t Queue_IsFull(Queue *queue)
{
	if(!queue->initialized) return 1;
	return (queue->front==(queue->rear+1)%queue->maxSize)?1:0;
}

//队列是否为空
uint8_t Queue_IsEmpty(Queue *queue)
{
	if(!queue->initialized) return 1;
	return (queue->front==queue->rear)?1:0;
}

//获取队列指定位置的元素(队头元素位置为0)
void* Queue_GetElement(Queue *queue,uint16_t position)
{
	if(!queue->initialized) return NULL;

	if(position>=Queue_Size(queue)) return NULL;
	
	return queue->data[(queue->front+position)%queue->maxSize];
}

//获取队头元素(但不出队)
void* Queue_Top(Queue *queue)
{
	if(!queue->initialized) return NULL;

	if(Queue_IsEmpty(queue)) return NULL;
	
	return queue->data[queue->front];
}

//出队
void* Queue_Dequeue(Queue *queue)
{
	if(!queue->initialized) return NULL;

	if(Queue_IsEmpty(queue)) return NULL;
	
	void* res=queue->data[queue->front];
	queue->front=(queue->front+1)%queue->maxSize;
	return res;
}

//入队
void Queue_Enqueue(Queue *queue,void* data)
{
	if(!queue->initialized) return;

	if(Queue_IsFull(queue)) return;
	
	//如果缓冲区指针为空说明未附加保存区，入队原始指针
	if(queue->buffer == NULL)
	{
		queue->data[queue->rear]=data;
	}
	else//若加了保存区则将数据复制过来并入队保存区的指针
	{
		memcpy((uint8_t*)(queue->buffer) + (queue->rear)*(queue->bufElemSize),
						data,
						queue->bufElemSize);
		queue->data[queue->rear]=(uint8_t*)(queue->buffer) + (queue->rear)*(queue->bufElemSize);
	}
	
	queue->rear=(queue->rear+1)%queue->maxSize;
}
