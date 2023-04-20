#ifndef _QUEUE_H_
#define _QUEUE_H_

#include "stdint.h" 

#define EMPTY_QUEUE {NULL,0,0,0,NULL,0}

/*************数据结构**************/
//队列结构体
typedef struct _Queue
{
	//队列数据
	void **data;//只保存指针，若要同时保存指向的数据请附加保存区
	uint16_t maxSize;
	uint16_t front,rear;
	uint8_t initialized;
	//数据保存区(可选用)
	void *buffer;
	uint8_t bufElemSize;//每个元素的大小
}Queue;

/**************接口函数***************/
void Queue_Init(Queue *queue,uint16_t maxSize);
void Queue_AttachBuffer(Queue *queue,void *buffer,uint8_t elemSize);
void Queue_Destroy(Queue *queue);
uint16_t Queue_Size(Queue *queue);
uint8_t Queue_IsFull(Queue *queue);
uint8_t Queue_IsEmpty(Queue *queue);
void* Queue_GetElement(Queue *queue,uint16_t position);
void* Queue_Top(Queue *queue);
void* Queue_Dequeue(Queue *queue);
void Queue_Enqueue(Queue *queue,void* data);

#endif
