#include "softbus.h"
#include "motor.h"
#include "config.h"

typedef struct _Servo
{
	Motor motor;
	struct
	{
		uint8_t timX;
		uint8_t	channelX;
	}timInfo;
	float  targetAngle;//目标角度(单位度)
	float  maxAngle;
	float  maxDuty,minDuty;
}Servo;

Motor* Servo_Init(ConfItem* dict);

void Servo_SetTarget(Motor* motor,float targetValue);

Motor* Servo_Init(ConfItem* dict)
{
	//分配子类内存空间
	Servo* servo = MOTOR_MALLOC_PORT(sizeof(Servo));
	memset(servo,0,sizeof(Servo));
	//子类多态
	servo->motor.setTarget = Servo_SetTarget;
	//初始化电机绑定TIM信息
	servo->timInfo.timX = Conf_GetValue(dict,"tim-x",uint8_t,0);
	servo->timInfo.channelX = Conf_GetValue(dict,"channel-x",uint8_t,0);
	servo->maxAngle = Conf_GetValue(dict,"max-angle",float,180);
	servo->maxDuty = Conf_GetValue(dict,"max-duty",float,0.125f);
	servo->minDuty = Conf_GetValue(dict,"min-duty",float,0.025f);

	return (Motor*)servo;
}

//设置舵机角度
void Servo_SetTarget(Motor* motor,float targetValue)
{
	Servo* servo=(Servo*) motor;
	servo->targetAngle = targetValue; //无实际用途，可用于debug
	float pwmDuty = servo->targetAngle / servo->maxAngle * (servo->maxDuty - servo->minDuty) + servo->minDuty;
	Bus_RemoteCall("/tim/pwm/set-duty", {{"tim-x", &servo->timInfo.timX}, {"channel-x", &servo->timInfo.channelX}, {"duty", &pwmDuty}});
}


