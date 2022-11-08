#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <string.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct {
	char *name;
	void *value;
} ConfItem;

#define CF_DICT (ConfItem[])
#define CF_DICT_END {NULL,NULL}
#define IM_PTR(type,...) (&(type){__VA_ARGS__})

void* _Conf_GetValue(ConfItem* dict, const char* name);

#define Conf_ItemExist(dict,name) (_Conf_GetValue((dict),(#name))!=NULL)
#define Conf_GetPtr(dict,name,type) ((type*)_Conf_GetValue((dict),(#name)))
#define Conf_GetValue(dict,name,type,def) (_Conf_GetValue((dict),(#name))?(*(type*)(_Conf_GetValue((dict),(#name)))):(def))

#endif
