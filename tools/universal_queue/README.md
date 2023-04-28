# 通用队列算法

本项目实现了一个通用类型的队列算法

## 包含文件

项目仅包含一对`my_queue.c/h`文件，使用前将这两个文件添加至项目中即可

## 依赖项

项目仅对标准库文件`stdint.h`、`stdlib.h`、`string.h`有依赖，使用了其中的`malloc`、`free`、`memcpy`以及`uint32_t`等基础类型

## 原理介绍

队列结构体如下
```c
typedef struct
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
```
队列算法为常见的循环队列，其队列数据区为`Queue`结构体中的`data`成员，分配内存后是一个`void*`型数组，用于指向任意类型的变量，实现类型的通用性

若需要入队的数据不是全局变量，在其对应的指针出队时可能已经被销毁，则要将一个数据保存区附加到`buffer`，入队时将指定数据拷贝一份至保存区内，出队时即可读取保存区内的值

## API

```c
//队列的初始化，为data字段分配内存
void Queue_Init(Queue *queue,uint16_t maxSize);
//附加数据保存区到buffer，参数3为每个所用数据类型的字节数
void Queue_AttachBuffer(Queue *queue,void *buffer,uint8_t elemSize);
//销毁队列
void Queue_Destroy(Queue *queue);
//计算队列内元素个数
uint16_t Queue_Size(Queue *queue);
//判断队列是否为满
uint8_t Queue_IsFull(Queue *queue);
//判断队列是否为空
uint8_t Queue_IsEmpty(Queue *queue);
//获取队列中第position个元素
void* Queue_GetElement(Queue *queue,uint16_t position);
//获取队头元素（但不出队）
void* Queue_Top(Queue *queue);
//出队
void* Queue_Dequeue(Queue *queue);
//入队
void Queue_Enqueue(Queue *queue,void* data);
```

## 使用方法简介

在需要使用队列的文件头部引用`Queue.h`文件
```c
#include "Queue.h"
```
创建队列结构体变量，并初始化
```c
//创建队列对象
Queue queue;
//进行初始化（动态分配队列内存）
Queue_Init(&queue,10);//第二个参数是队列最大长度
```
此后即可进行队列操作
```c
int data[10];//全局变量
//...
//入队
Queue_Enqueue(&queue,&data[0]);
Queue_Enqueue(&queue,&data[1]);
//出队
int *num1 = Queue_Dequeue(&queue);
int *num2 = Queue_Dequeue(&queue);

printf("%d,%d", *num1, *num2);
```
若需要入队局部变量，则需附加一个数据保存区，该保存区大小必须不小于（队列最大长度*元素大小）
```c
int queueBuffer[10];//保存区为全局变量
//char queueBuffer[10*sizeof(int)];//大小足够即可

//初始化后执行
Queue_AttachBuffer(&queue,queueBuffer,sizeof(int));

//某函数内部
for(int i=0;i<10;i++)//入队
{
	Queue_Enqueue(&queue,&i);//i为局部变量，会被拷贝至queueBuffer
}
for(int i=0;i<10;i++)//出队
{
	int num=*(int*)Queue_Dequeue(&queue);//取出的数据来自queueBuffer
	printf("%d ",num);
}
```
由于队列的通用性，可以存储任意类型的变量，以下为一个完整演示，完整Dev-Cpp工程在example目录下
```c
#include "Queue.h"

#define MAX_QUEUE_SIZE 10

typedef struct
{
	char *name;
	int score;
}Student;

Queue stuQueue;
Student queueBuffer[MAX_QUEUE_SIZE];

//创建一个Student并加入队列
void AddStudent(char *name,int score)
{
	Student student;
	student.name=name;
	student.score=score;
	Queue_Enqueue(&stuQueue,&student);//入队
}//退出函数后student已被销毁，但仍在queueBuffer内有备份

int main()
{
	//队列初始化
	Queue_Init(&stuQueue,MAX_QUEUE_SIZE);
	Queue_AttachBuffer(&stuQueue,queueBuffer,sizeof(Student));
	//载入数据
	AddStudent("zhangsan",90);
	AddStudent("lisi",60);
	//读出数据
	Student stu1=*(Student*)Queue_Dequeue(&stuQueue);
	Student stu2=*(Student*)Queue_Dequeue(&stuQueue);

	printf("stu1:%s,%d;stu2:%s,%d",
		stu1.name,stu1.score,
		stu2.name,stu2.score);
}
```

## 注意事项

由于队列内存采用动态内存分配，在存储空间较小的单片机上使用时需要控制队列长度，过长会导致队列初始化失败；由于动态内存分配使用堆空间，用户需要保证其大小足够
