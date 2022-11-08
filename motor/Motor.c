#include "Motor.h"

//子类头文件
#include "M3508.h"
#include "M2006.h"
#include "M6020.h"

//引用标准库函数用于内存操作，可通过修改宏定义进行移植
#include <stdlib.h>
#include "cmsis_os.h"
#define MOTOR_MALLOC_PORT(len) pvPortMalloc(len)
#define MOTOR_FREE_PORT(ptr) vPortFree(ptr)

//内部函数声明
float Motor_GetReductionRatio(Motor* motor);
uint16_t Motor_GetID(Motor* motor);
float Motor_SpeedPIDCalc(Motor* motor, float reference);
float Motor_AnglePIDCalc(Motor* motor, float reference);
void Motor_StartStatAngle(Motor* motor);
void Motor_StatAngle(Motor* motor);
void Motor_Update(Motor* motor,void* data);
void _Motor_Init(Motor* motor);

void Motor_Init(Motor* motor, ConfItem* dict)
{
	char* motorType = Conf_GetPtr(dict, type, char);
	if(!strcmp(motorType, "M3508"))
	{
		motor = (Motor*)pvPortMalloc(sizeof(M3508));
		_Motor_Init(motor);
		M3508_Init(motor, dict);
	}
	else if(!strcmp(motorType, "M2006"))
	{
		motor = (Motor*)pvPortMalloc(sizeof(M2006));
		_Motor_Init(motor);
		M2006_Init(motor, dict);
	}
	else if(!strcmp(motorType, "M6020"))
	{
		motor = (Motor*)pvPortMalloc(sizeof(M6020));
		_Motor_Init(motor);
		M6020_Init(motor, dict);
	}
}

void _Motor_Init(Motor* motor)
{
	motor->getReductionRatio = Motor_GetReductionRatio;
	motor->getID = Motor_GetID;
	motor->speedPIDCalc = Motor_SpeedPIDCalc;
	motor->anglePIDCalc = Motor_AnglePIDCalc;
	motor->startStatAngle = Motor_StartStatAngle;
	motor->statAngle = Motor_StatAngle;
	motor->update = Motor_Update;
}

float Motor_GetReductionRatio(Motor* motor)
{
	return 0;
}

uint16_t Motor_GetID(Motor* motor)
{
	return 0;
}

float Motor_SpeedPIDCalc(Motor* motor, float reference)
{
	return 0;
}

float Motor_AnglePIDCalc(Motor* motor, float reference)
{
	return 0;
}

void Motor_StartStatAngle(Motor* motor)
{
	return;
}

void Motor_StatAngle(Motor* motor)
{
	return;
}

void Motor_Update(Motor* motor,void* data)
{
	return;
}
