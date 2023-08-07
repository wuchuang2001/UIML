#ifndef __FILER_H
#define __FILER_H

#include "config.h"
#include "cmsis_os.h"
#include <string.h>

#define FILTER_MALLOC_PORT(len) pvPortMalloc(len)
#define FILTER_FREE_PORT(ptr) vPortFree(ptr)

typedef struct _Filter
{
	float(*cala)(struct _Filter *filter, float data);
}Filter;

Filter* Filter_Init(ConfItem* dict);

#endif /* __FILER_H */
