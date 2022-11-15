#ifndef _MOTOR_H_
#define _MOTOR_H_
#include "config.h"
#include "cmsis_os.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define MOTOR_MALLOC_PORT(len) pvPortMalloc(len)
#define MOTOR_FREE_PORT(ptr) vPortFree(ptr)
#define MOTOR_STRLEN_PORT(str) strlen(str)
#define MOTOR_STRCAT_PORT(str1,str2) strcat((str1),(str2))

typedef enum
{
	torque,
	speed,
	angle
}Ctrler;

typedef struct _Motor
{
	void (*changeCtrler)(struct _Motor* motor, Ctrler ctrlerType);
	void (*ctrlerCalc)(struct _Motor* motor,float reference);
	
	void (*startStatAngle)(struct _Motor* motor);
	void (*statAngle)(struct _Motor* motor);
}Motor;

void Motor_Init(Motor* motor, ConfItem* dict);

#endif
