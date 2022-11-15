# 通用类型动态数组

## 简介

本项目实现了一个通用类型的动态数组，作用与C++中的vector模板库类似，是一个可动态插入、删除任意类型元素的线性结构

## 包含文件

项目仅包含一对`vector.c/h`文件，使用前将这两个文件添加至项目中即可

## 依赖项

项目仅对标准库文件`stdint.h`、`stdlib.h`、`string.h`有依赖，使用了其中的`malloc`、`free`、`memcpy`以及`uint32_t`等基础类型，用户可以通过修改`vector.c`中的宏定义进行移植替换

## 接口使用示例

假设我们需要在Vector中存储如下类型

```c
typedef struct{
	int num1,num2;
}Pair;
```
1. 首先需要创建一个Vector对象

	```c
	//方式1：先定义再初始化
	Vector vector;
	Vector_Init(vector, Pair);

	//方式2：直接返回初始化好的Vector对象
	Vector vector = Vector_Create(Pair);
	```

2. 向Vector中插入元素

	```c
	Pair pair = {0,0};
	//在头部插入
	Vector_PushFront(vector, pair);
	//在尾部插入
	Vector_PushBack(vector, pair);
	//在下标为1处插入
	Vector_Insert(vector, 1, pair);
	```

	可以使用复合字面量，如：

	```c
	Vector_PushFront(vector, (Pair){0,0});
	```

3. 读取Vector中的元素

	```c
	//读取首元素
	Pair pair = *Vector_GetFront(vector, Pair); //注意取出的是指针
	//读取尾元素
	Pair pair = *Vector_GetBack(vector, Pair);
	//读取下标为1的元素
	Pair pair = *Vector_GetByIndex(vector, 1, Pair);
	```

4. 修改Vector中的元素

	```c
	//将下标为1的元素修改为{1,1}
	Pair newPair = {1,1};
	Vector_SetVal(vector, 1, newPair);
	```

5. 可以使用数组方式操作

	```c
	//转为数组后可以直接读写
	Pair *arr = Vector_ToArray(vector, Pair);
	arr[1].num1 = 1;

	//也可转换后直接用下标操作
	Vector_ToArray(vector, Pair)[1].num1 = 1;
	```

6. 可以使用简化版`for`循环进行遍历

	```c
	//普通方式进行遍历
	for(int i=0; i<Vector_Size(vector); i++)
	{
		Pair *pair = Vector_GetByIndex(vector, i, Pair);
		printf("%d,%d\n", pair->num1, pair->num2);
	}

	//简化方式进行遍历
	Vector_ForEach(vector, pair, Pair) //(Pair*)型的pair依次指向每个元素
	{
		printf("%d,%d\n", pair->num1, pair->num2);
	}
	```

7. 获取Vector的元素个数和容量

	> 注：元素个数超过容量时容量会自动扩大1倍

	```c
	int size = Vector_Size(vector);
	int capacity = Vector_Capacity(vector);
	```

8. 从Vector中删除元素

	```c
	//删除首元素
	Vector_PopFront(vector);
	//删除尾元素
	Vector_PopBack(vector);
	//删除下标为1的元素
	Vector_Remove(vector, 1);
	//删除所有元素
	Vector_Clear(vector);
	```

9. 释放空余空间

	> 注：每次自动扩容时容量会增加一倍，且删除元素时不会释放空间，如有需要可使用这个函数手动释放未使用的空间

	```c
	Vector_TrimCapacity(vector);
	```

10. 销毁Vector对象

	```c
	Vector_Destroy(vector);
	```

## 注意事项

1. 初始化时传入的类型须与后续对该vector操作时传入的类型一致

2. 用于存放元素的空间全部分配自堆区，应保证堆区大小足够

3. 若一个vector对象已被初始化，在它被系统销毁前须调用`Vector_Destroy`进行释放，否则会造成内存泄漏

4. 由于接口内部使用地址传值，在插入和修改元素时不能直接使用立即数，应使用复合字面量传入

	```c
	Vector vector = Vector_Create(int);
	Vector_PushBack(vector, 100);//错误的插入方法
	Vector_PushBack(vector, (int){100});//正确的插入方法
	```

## 完整示例程序

> 注：该程序与仓库中`main.c`一致

```c
#include <stdio.h>
#include "vector.h"

typedef struct{
	int num1;
	int num2;
}Pair;

int main()
{
	Vector vector = Vector_Create(Pair); //创建用于存放Pair的Vector

	printf("从尾插入0-3, 从头插入0-3, 中间插入两个100\n");
	for(int i=0;i<4;i++)
		Vector_PushBack(vector, ((Pair){i,i})); //在后方插入
	for(int i=0;i<4;i++)
		Vector_PushFront(vector, ((Pair){i,i})); //在前方插入
	for(int i=0;i<2;i++)
		Vector_Insert(vector, 4, ((Pair){100,100})); //指定下标处插入
	for(int i=0;i<10;i++)
		printf("{%d,%d},", Vector_GetByIndex(vector,i,Pair)->num1, Vector_GetByIndex(vector,i,Pair)->num2);

	printf("\n\n修改为{0,9}-{9,0}\n");
	for(int i=0;i<5;i++)
		Vector_SetValue(vector, i, ((Pair){i,9-i})); //指定下标改写
	for(int i=5;i<10;i++)
		Vector_ToArray(vector,Pair)[i] = (Pair){i,9-i}; //转为数组后可直接读写
	for(int i=0;i<10;i++)
		printf("{%d,%d},", Vector_ToArray(vector,Pair)[i].num1, Vector_ToArray(vector,Pair)[i].num2);
		
	printf("\n\n删除前5个\n");
	for(int i=0;i<5;i++)
		Vector_PopFront(vector); //弹出首元素
	Vector_ForEach(vector, pair, Pair) //简化版遍历方式
		printf("{%d,%d},", pair->num1, pair->num2);
	
	printf("\n\n全部删除\n");
	Vector_Clear(vector); //清空数据
	printf("size=%d", Vector_Size(vector));

	printf("\n\n模拟栈操作, 入栈0-9后出栈\n");
	for(int i=0;i<10;i++)
		Vector_PushBack(vector,((Pair){i,i}));
	for(int i=0;i<10;i++)
	{
		printf("{%d,%d},", Vector_GetBack(vector,Pair)->num1, Vector_GetBack(vector,Pair)->num2);
		Vector_PopBack(vector);
	}

	Vector_Clear(vector);

	printf("\n\n模拟队列操作, 入队0-9后出队\n");
	for(int i=0;i<10;i++)
		Vector_PushBack(vector,((Pair){i,i}));
	for(int i=0;i<10;i++)
	{
		printf("{%d,%d},", Vector_GetFront(vector,Pair)->num1, Vector_GetFront(vector,Pair)->num2);
		Vector_PopFront(vector);
	}

	Vector_Destroy(vector); //销毁vector

	printf("\n\n回车退出");
	getchar();
}

```

**程序输出：**

```
从尾插入0-3, 从头插入0-3, 中间插入两个100
{3,3},{2,2},{1,1},{0,0},{100,100},{100,100},{0,0},{1,1},{2,2},{3,3},

修改为{0,9}-{9,0}
{0,9},{1,8},{2,7},{3,6},{4,5},{5,4},{6,3},{7,2},{8,1},{9,0},

删除前5个
{5,4},{6,3},{7,2},{8,1},{9,0},

全部删除
size=0

模拟栈操作, 入栈0-9后出栈
{9,9},{8,8},{7,7},{6,6},{5,5},{4,4},{3,3},{2,2},{1,1},{0,0},

模拟队列操作, 入队0-9后出队
{0,0},{1,1},{2,2},{3,3},{4,4},{5,5},{6,6},{7,7},{8,8},{9,9},

回车退出
```
