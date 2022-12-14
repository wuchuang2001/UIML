#ifndef _USER_PID_H_
#define _USER_PID_H_

#include "config.h"

#include <stdint.h>

#ifndef LIMIT
#define LIMIT(x,min,max) (x)=(((x)<=(min))?(min):(((x)>=(max))?(max):(x)))
#endif

#ifndef ABS
#define ABS(x) ((x)>=0?(x):-(x))
#endif

typedef struct _PID
{
	float kp,ki,kd;
	float error,lastError;//误差、上次误差
	float integral,maxIntegral;//积分、积分限幅
	float output,maxOutput;//输出、输出限幅
	float deadzone;//死区
}PID;

typedef struct _CascadePID
{
	PID inner;//内环
	PID outer;//外环
	float output;//串级输出，等于inner.output
}CascadePID;

void PID_Init(PID *pid, ConfItem* conf);
void PID_SingleCalc(PID *pid,float reference,float feedback);
void PID_CascadeCalc(CascadePID *pid,float angleRef,float angleFdb,float speedFdb);
void PID_Clear(PID *pid);
void PID_SetMaxOutput(PID *pid,float maxOut);
void PID_SetDeadzone(PID *pid,float deadzone);

#endif
