#include "motor.h"

//X-MACRO
//子类列表，每一项格式为(类型名,初始化函数名)
#define MOTOR_CHILD_LIST \
	MOTOR_TYPE("M3508",M3508_Init) \
	MOTOR_TYPE("M2006",M2006_Init) \
	MOTOR_TYPE("M6020",M6020_Init)

//内部函数声明
void Motor_SetTarget(Motor* motor, float targetValue);
void Motor_ChangeCtrler(Motor* motor, MotorCtrlMode ctrlerType);
void Motor_StartStatAngle(Motor* motor);
void Motor_StatAngle(Motor* motor);
void Motor_InitDefault(Motor* motor);

//声明子类初始化函数
#define MOTOR_TYPE(name,initFunc) extern Motor* initFunc(ConfItem*);
MOTOR_CHILD_LIST
#undef MOTOR_TYPE

Motor* Motor_Init(ConfItem* dict)
{
	char* motorType = Conf_GetPtr(dict, "type", char);

	Motor *motor = NULL;
	//判断属于哪个子类
	#define MOTOR_TYPE(name,initFunc) \
	if(!strcmp(motorType, name)) \
		motor = initFunc(dict);
	MOTOR_CHILD_LIST
	#undef MOTOR_TYPE
	//将子类未定义的方法设置为空函数
	if(motor)
		Motor_InitDefault(motor);

	return motor;
}

void Motor_InitDefault(Motor* motor)
{
	if(!motor->changeMode)
		motor->changeMode = Motor_ChangeCtrler;
	if(!motor->setTarget)
		motor->setTarget = Motor_SetTarget;
	if(!motor->startStatAngle)
		motor->startStatAngle = Motor_StartStatAngle;
	if(!motor->statAngle)
		motor->statAngle = Motor_StatAngle;
}

//纯虚函数
void Motor_SetTarget(Motor* motor, float targetValue) { }

void Motor_ChangeCtrler(Motor* motor, MotorCtrlMode mode) { }

void Motor_StartStatAngle(Motor* motor) { }

void Motor_StatAngle(Motor* motor) { }
