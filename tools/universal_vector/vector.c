#include "vector.h"

//引用标准库函数用于内存操作，可通过修改宏定义进行移植
#include <stddef.h>
#include <string.h>
#include "cmsis_os.h"
#define VECTOR_MALLOC_PORT(len) pvPortMalloc(len)
#define VECTOR_FREE_PORT(ptr) vPortFree(ptr)
#define VECTOR_MEMCPY_PORT(dst,src,len) memcpy(dst,src,len)

//初始化, 成功返回0失败返回-1
int _Vector_Init(Vector *vector, int _elementSize)
{
	vector->elementSize=_elementSize;
	vector->capacity=1;
	vector->size=0;
	vector->data=VECTOR_MALLOC_PORT(_elementSize);
	if(!vector->data)
		return -1;
	return 0;
}

//创建并初始化一个vector
Vector _Vector_Create(int _elementSize)
{
	Vector vector;
	_Vector_Init(&vector, _elementSize);
	return vector;
}

//销毁一个vector
void _Vector_Destroy(Vector *vector)
{
	if(vector->data)
		VECTOR_FREE_PORT(vector->data);
}

//插入元素, 成功返回0失败返回-1
int _Vector_Insert(Vector *vector, uint32_t index, void *element)
{
	if(!vector->data || index > vector->size)
		return -1;
	if(vector->size+1 > vector->capacity) //若剩余空间不足，重新分配两倍空间并拷贝数据
	{
		uint8_t *newBuf=VECTOR_MALLOC_PORT(vector->capacity * 2 * vector->elementSize);
		if(!newBuf)
			return -1;
		vector->capacity*=2;
		VECTOR_MEMCPY_PORT(newBuf, vector->data, vector->size * vector->elementSize);
		VECTOR_FREE_PORT(vector->data);
		vector->data=newBuf;
	}
	if(vector->size > 0 && index <= vector->size-1) //将插入点后的数据后移
	{
		for(int32_t pos=vector->size-1; pos>=(int32_t)index; pos--)
			VECTOR_MEMCPY_PORT(vector->data + (pos+1) * vector->elementSize, vector->data + pos * vector->elementSize, vector->elementSize);
	}
	VECTOR_MEMCPY_PORT(vector->data + index * vector->elementSize, element, vector->elementSize); //拷贝用户传入的值
	vector->size++;
	return 0;
}

//删除元素, 成功返回0失败返回-1
int _Vector_Remove(Vector *vector, uint32_t index)
{
	if(!vector->data || vector->size==0 || index >= vector->size)
		return -1;
	for(uint32_t pos=index; pos<(vector->size-1); pos++) //将删除点后的数据前移
		VECTOR_MEMCPY_PORT(vector->data + pos * vector->elementSize, vector->data + (pos+1) * vector->elementSize, vector->elementSize);
	vector->size--;
	return 0;
}

//获取指定位置上的元素指针, 若失败返回NULL
void *_Vector_GetByIndex(Vector *vector, uint32_t index)
{
	if(index >= vector->size)
		return NULL;
	return vector->data + index * vector->elementSize; //计算偏移，返回地址
}

//修改指定位置上的元素, 成功返回0失败返回-1
int _Vector_SetValue(Vector *vector, uint32_t index, void *element)
{
	if(index >= vector->size || !vector->data)
		return -1;
	VECTOR_MEMCPY_PORT(vector->data + index * vector->elementSize, element, vector->elementSize); //拷贝用户传入的值到目标地址
	return 0;
}

//删除多余的空间, 成功返回0失败返回-1
int _Vector_TrimCapacity(Vector *vector)
{
	if(!vector->data)
		return -1;
	if(vector->capacity <= vector->size)
		return 0;
	int newCapacity=(vector->size ? vector->size : 1); //计算新空间大小
	uint8_t *newBuf=VECTOR_MALLOC_PORT(newCapacity * vector->elementSize); //分配新空间
	if(!newBuf)
		return -1;
	if(vector->size)
		VECTOR_MEMCPY_PORT(newBuf, vector->data, vector->size * vector->elementSize); //拷贝原有数据
	VECTOR_FREE_PORT(vector->data);
	vector->data=newBuf;
	vector->capacity=newCapacity;
	return 0;
}
