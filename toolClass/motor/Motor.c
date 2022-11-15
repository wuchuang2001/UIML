#include "Motor.h"

//子类头文件
#include "M3508.h"
#include "M2006.h"
#include "M6020.h"

//内部函数声明
void Motor_CtrlerCalc(Motor* motor, float reference);
void Motor_ChangeCtrler(Motor* motor, Ctrler ctrlerType);
void Motor_StartStatAngle(Motor* motor);
void Motor_StatAngle(Motor* motor);
void _Motor_Init(Motor* motor);

void Motor_Init(Motor* motor, ConfItem* dict)
{
	char* motorType = Conf_GetPtr(dict, "type", char);
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
	motor->changeCtrler = Motor_ChangeCtrler;
	motor->ctrlerCalc = Motor_CtrlerCalc;
	motor->startStatAngle = Motor_StartStatAngle;
	motor->statAngle = Motor_StatAngle;
}

void Motor_CtrlerCalc(Motor* motor, float reference)
{
	return ;
}

void Motor_ChangeCtrler(Motor* motor, Ctrler ctrlerType)
{
	return ;
}

void Motor_StartStatAngle(Motor* motor)
{
	return;
}

void Motor_StatAngle(Motor* motor)
{
	return;
}
