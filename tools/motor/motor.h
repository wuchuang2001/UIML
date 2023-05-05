#ifndef _MOTOR_H_
#define _MOTOR_H_
#include "config.h"
#include "cmsis_os.h"

#define MOTOR_MALLOC_PORT(len) pvPortMalloc(len)
#define MOTOR_FREE_PORT(ptr) vPortFree(ptr)
//模式
typedef enum
{
	MOTOR_TORQUE_MODE,
	MOTOR_SPEED_MODE,
	MOTOR_ANGLE_MODE,
	MOTOR_STOP_MODE
}MotorCtrlMode;
//父类，包含所有子类的方法
typedef struct _Motor
{
	void (*changeMode)(struct _Motor* motor, MotorCtrlMode mode);
	void (*setTarget)(struct _Motor* motor,float targetValue);
	
	void (*setStartAngle)(struct _Motor* motor, float angle);
	float (*getData)(struct _Motor* motor,  const char* data);
	void (*stop)(struct _Motor* motor);
}Motor;

Motor* Motor_Init(ConfItem* dict);

#endif
