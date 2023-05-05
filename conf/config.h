#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

//配置项类型
typedef struct {
	char *name; //配置名
	void *value; //配置值
} ConfItem;

//给用户配置文件用的宏封装
#define CF_DICT (ConfItem[])
#define CF_DICT_END {NULL,NULL}

#ifndef IM_PTR
#define IM_PTR(type,...) (&(type){__VA_ARGS__}) //取立即数的地址
#endif

//获取配置值，不应直接调用，应使用下方的封装宏
void* _Conf_GetValue(ConfItem* dict, const char* name);

/*
	@brief 判断字典中配置名是否存在
	@param dict:目标字典
	@param name:要查找的配置名，可通过'/'分隔以查找内层字典
	@retval 0:不存在 1:存在
*/
#define Conf_ItemExist(dict,name) (_Conf_GetValue((dict),(name))!=NULL)

/*
	@brief 获取配置值指针
	@param dict:目标字典
	@param name:要查找的配置名，可通过'/'分隔以查找内层字典
	@param type:配置值的类型
	@retval 指向配置值的(type*)型指针，若配置名不存在则返回NULL
*/
#define Conf_GetPtr(dict,name,type) ((type*)_Conf_GetValue((dict),(name)))

/*
	@brief 获取配置值
	@param dict:目标字典
	@param name:要查找的配置名，可通过'/'分隔以查找内层字典
	@param type:配置值的类型
	@param def:默认值
	@retval type型配置值数据，若配置名不存在则返回def
*/
#define Conf_GetValue(dict,name,type,def) (_Conf_GetValue((dict),(name))?(*(type*)(_Conf_GetValue((dict),(name)))):(def))

#endif
