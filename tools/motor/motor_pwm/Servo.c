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
		uint32_t pwmValue;
		
	}timInfo;
	float  targetAngle;//目标值(输出轴扭矩矩/速度/角度(单位度))
	float  maxAngle;
	
}Servo;

Motor* Servo_Init(ConfItem* dict);

void Servo_SetTarget(struct _Motor* motor,float targetValue);
void Servo_SetStartAngle(Motor *motor, float startAngle);
void Servo_TimerCallback(void const *argument);

//软件定时器回调函数
void Servo_TimerCallback(void const *argument)
{
	Servo *servo=(Servo*) argument;
	float pwmDuty=(servo->targetAngle/servo->maxAngle+1)/20.0f;
	Bus_BroadcastSend("/tim/set-pwm-duty",{{"timX",&servo->timInfo.timX},{"channelX",&servo->timInfo.channelX},{"duty",&pwmDuty}});
}

Motor* Servo_Init(ConfItem* dict)
{
	//分配子类内存空间
	Servo* servo = MOTOR_MALLOC_PORT(sizeof(Servo));
	memset(servo,0,sizeof(Servo));
	//子类多态
	servo->motor.setTarget = Servo_SetTarget;
	servo->motor.setStartAngle = Servo_SetStartAngle;
	//初始化电机绑定TIM信息
	servo->timInfo.timX=Conf_GetValue(dict,"timX",uint8_t,0);
	servo->timInfo.channelX=Conf_GetValue(dict,"channelX",uint8_t,0);
	servo->maxAngle=Conf_GetValue(dict,"maxAngle",float,180);
	//开启软件定时器
	osTimerDef(Servo, Servo_TimerCallback);
	osTimerStart(osTimerCreate(osTimer(Servo), osTimerPeriodic,servo), 2);

	return (Motor*)servo;
}

//设置舵机初始角度
void Servo_SetStartAngle(Motor *motor, float startAngle)
{
	Servo* servo=(Servo*) motor;
	servo->targetAngle=startAngle;
	//将角度转为占空比
	float pwmDuty=(servo->targetAngle/servo->maxAngle+1)/20.0f;
	Bus_BroadcastSend("/tim/set-pwm-duty",{{"timX",&servo->timInfo.timX},{"channelX",&servo->timInfo.channelX},{"duty",&pwmDuty}});
}

//设置舵机角度
void Servo_SetTarget(struct _Motor* motor,float targetValue)
{
	Servo* servo=(Servo*) motor;
	servo->targetAngle=targetValue;
}


