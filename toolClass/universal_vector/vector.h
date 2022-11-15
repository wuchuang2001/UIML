/* 用于C语言的通用类型动态数组模块，类似于C++中的STL模板库vector */

#ifndef _VECTOR_H_
#define _VECTOR_H_

#include <stdint.h>

//Vector数据结构体
typedef struct{
	uint8_t *data;
	uint8_t elementSize;
	uint32_t size,capacity;
}Vector;

//操作函数声明(不直接调用，应使用下方define定义的接口)
int _Vector_Init(Vector *vector,int _elementSize);
Vector _Vector_Create(int _elementSize);
void _Vector_Destroy(Vector *vector);
int _Vector_Insert(Vector *vector, uint32_t index, void *element);
int _Vector_Remove(Vector *vector, uint32_t index);
void *_Vector_GetByIndex(Vector *vector, uint32_t index);
int _Vector_SetValue(Vector *vector, uint32_t index, void *element);
int _Vector_TrimCapacity(Vector *vector);

/*
	@brief 初始化一个指定类型的Vector
	@param vector:需要初始化的Vector对象
	       type:该Vector中将会存放的元素类型
	@retval 0:成功 -1:失败
*/
#define Vector_Init(vector,type) (_Vector_Init(&(vector),sizeof(type)))

/*
	@brief 创建一个指定类型的Vector
	@param type:该Vector中将会存放的元素类型
	@retval 已初始化好的Vector对象 (若内存分配失败则其中data=NULL)
*/
#define Vector_Create(type) (_Vector_Create(sizeof(type)))

/*
	@brief 销毁一个Vector
	@param vector:需要销毁的Vector
	@retval void
*/
#define Vector_Destroy(vector) (_Vector_Destroy(&(vector)))

/*
	@brief 获取已插入的元素数量
	@param vector:目标Vector对象
	@retval 该vector中的元素数量
*/
#define Vector_Size(vector) ((vector).size)

/*
	@brief 获取当前已分配的容量
	@param vector:目标Vector对象
	@retval 已为该vector分配的空间所能存储的元素数量
*/
#define Vector_Capacity(vector) ((vector).capacity)

/*
	@brief 删除空余容量
	@param vector:目标Vector对象
	@retval 0:成功 -1:失败
*/
#define Vector_TrimCapacity(vector) (_Vector_TrimCapacity(&(vector)))

/*
	@brief 将Vector转为元素数组
	@param vector:目标vector对象
	       type:元素类型
	@retval type*类型的指针，指向数据首地址，可直接作为数组名使用
	@example int num = Vector_ToArray(vector,int)[0];
*/
#define Vector_ToArray(vector,type) ((type*)((vector).data))

/*
	@brief 获取指定下标处元素的指针
	@param vector:目标vector对象
	       index:指定的下标
	       type:元素类型
	@retval 指向指定下标处元素的(type*)型指针，若下标不存在则返回NULL
*/
#define Vector_GetByIndex(vector,index,type) ((type*)_Vector_GetByIndex(&(vector),(index)))

/*
	@brief 获取首元素指针
	@param vector:目标vector对象
	       type:元素类型
	@retval 指向首元素的(type*)型指针，若不存在则返回NULL
*/
#define Vector_GetFront(vector,type) Vector_GetByIndex(vector,0,type)

/*
	@brief 获取尾元素指针
	@param vector:目标vector对象
	       type:元素类型
	@retval 指向尾元素的(type*)型指针，若不存在则返回NULL
*/
#define Vector_GetBack(vector,type) Vector_GetByIndex(vector,(vector).size-1,type)

/*
	@brief 修改指定下标的值
	@param vector:目标vector对象
	       index:指定的下标
	       val:需要修改的新值
	@retval 0:成功 -1:失败
	@notice 若希望在val处传入立即数则应使用字面量，如传入100应写"(int){100}"
*/
#define Vector_SetValue(vector,index,val) (_Vector_SetValue(&(vector),(index),&(val)))

/*
	@brief 删除指定下标的元素
	@param vector:要操作的vector对象
	       index:指定的下标
	@retval 0:成功 -1:失败
*/
#define Vector_Remove(vector,index) (_Vector_Remove(&(vector),(index)))

/*
	@brief 删除尾元素
	@param vector:要操作的vector对象
	@retval 0:成功 -1:失败
*/
#define Vector_PopBack(vector) Vector_Remove(vector,(vector).size-1)

/*
	@brief 删除首元素
	@param vector:要操作的vector对象
	@retval 0:成功 -1:失败
*/
#define Vector_PopFront(vector) Vector_Remove(vector,0)

/*
	@brief 删除所有元素
	@param vector:要操作的vector对象
	@retval 0
*/
#define Vector_Clear(vector) ((vector).size=0)

/*
	@brief 在指定下标处插入元素，该下标处原有元素及其后续元素向后移动
	@param vector:目标vector对象
	       index:指定的下标
	       val:需要插入的值
	@retval 0:成功 -1:失败
	@notice 若希望在val处传入立即数则应使用字面量，如传入100应写"(int){100}"
*/
#define Vector_Insert(vector,index,val) (_Vector_Insert(&(vector),(index),&(val)))

/*
	@brief 在尾部插入元素
	@param vector:目标vector对象
	       val:需要插入的值
	@retval 0:成功 -1:失败
	@notice 若希望在val处传入立即数则应使用字面量，如传入100应写"(int){100}"
*/
#define Vector_PushBack(vector,val) Vector_Insert(vector,(vector).size,(val))

/*
	@brief 在头部插入元素
	@param vector:目标vector对象
	       val:需要插入的值
	@retval 0:成功 -1:失败
	@notice 若希望在val处传入立即数则应使用字面量，如传入100应写"(int){100}"
*/
#define Vector_PushFront(vector,val) Vector_Insert(vector,0,(val))

/*
	@brief 遍历迭代语句，替代遍历过程中的for语句
	@param vector:目标vector对象
	       iter:遍历时迭代器的变量名，是指向所遍历到的元素的(type*)型指针
	       type:元素的类型
	@example Vector_ForEach(vector,ptr,int) { printf("%d",*ptr); }
*/
#define Vector_ForEach(vector,iter,type) for(type *iter=(type*)(vector).data; iter<((type*)(vector).data+(vector).size); iter++)

#endif
