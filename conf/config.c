#include "config.h"
#include "sys_conf.h"
#include "cmsis_os.h"

//声明服务模块回调函数
#define SERVICE(service,callback,priority,stackSize) extern void callback(void const *);
SERVICE_LIST
#undef SERVICE

//服务列表枚举
typedef enum
{
	#define SERVICE(service,callback,priority,stackSize) service,
	SERVICE_LIST
	#undef SERVICE
	serviceNum
}Module;

//服务任务句柄
osThreadId serviceTaskHandle[serviceNum];

//取出给定配置表中给定配置项的值，没找到则返回NULL
void* _Conf_GetValue(ConfItem* dict, const char* name)
{
	if(!dict)
		return NULL;

	char *sep=NULL;

	do{
		sep=strchr(name,'/');
		if(sep==NULL)
			sep=(char*)name+strlen(name);

		while(dict->name)
		{
			if(strlen(dict->name)==sep-name && strncmp(dict->name,name,sep-name)==0)
			{
				if(*sep=='\0')
					return dict->value;
				dict=dict->value;
				break;
			}
			dict++;
		}

		name=sep+1;

	}while(*sep!='\0');
	return NULL;
}

//FreeRTOS默认任务
void StartDefaultTask(void const * argument)
{
	//创建所有服务任务，将配置表分别作为参数传入
	#define SERVICE(service,callback,priority,stackSize) \
		osThreadDef(service, callback, priority, 0, stackSize); \
		serviceTaskHandle[service] = osThreadCreate(osThread(service), Conf_GetPtr(systemConfig,#service,void));
	SERVICE_LIST
	#undef SERVICE
	//销毁自己
	vTaskDelete(NULL);
}
