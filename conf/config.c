#include "config.h"
#include "sys_conf.h"
#include "cmsis_os.h"

//声明服务模块回调函数
#define SERVICE(service,callback,priority) extern void callback(void const *);
SERVICE_LIST
#undef SERVICE

typedef enum
{
	#define SERVICE(service,callback,priority) service,
	SERVICE_LIST
	#undef SERVICE
	serviceNum
}Module;

osThreadId serviceTaskHandle[serviceNum];

void* _Conf_GetValue(ConfItem* dict, const char* name)
{
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

void StartDefaultTask(void const * argument)
{
	#define SERVICE(service,callback,priority) \
		osThreadDef(service, callback, priority, 0, 128); \
		serviceTaskHandle[service] = osThreadCreate(osThread(service), Conf_GetPtr(systemConfig,#service,void));
	SERVICE_LIST
	#undef SERVICE
	vTaskDelete(NULL);
}
