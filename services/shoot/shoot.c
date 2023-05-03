#include "config.h"
#include "softbus.h"
#include "motor.h"
#include "cmsis_os.h"

typedef enum
{
	SHOOTER_MODE_IDLE,
	SHOOTER_MODE_ONCE,
	SHOOTER_MODE_CONTINUE,
	SHOOTER_MODE_BLOCK
}ShooterMode;

typedef struct
{
	Motor *fricMotors[2],*triggerMotor;
	bool fricEnable;
	float fricSpeed; //摩擦轮速度
	float triggerAngle,targetTrigAngle; //拨弹一次角度及累计角度
	uint16_t intervalTime; //连发间隔ms
	uint8_t mode;
	uint8_t taskInterval;

	char* settingName;
	char* changeModeName;
	char* triggerStallName;
}Shooter;

void Shooter_Init(Shooter* shooter, ConfItem* dict);
bool Shooter_SettingCallback(const char* name, SoftBusFrame* frame, void* bindData);
bool Shoot_ChangeModeCallback(const char* name, SoftBusFrame* frame, void* bindData);
void Shooter_BlockCallback(const char* name, SoftBusFrame* frame, void* bindData);
void Shoot_StopCallback(const char* name, SoftBusFrame* frame, void* bindData);

void Shooter_TaskCallback(void const * argument)
{
	portENTER_CRITICAL();
	float totalAngle; //为解决warning，C语言中标签的下一条语句不能是定义变量的表达式，而case恰好就是标签
	Shooter shooter={0};
	Shooter_Init(&shooter, (ConfItem*)argument);
	portEXIT_CRITICAL();

	osDelay(2000);
	while(1)
	{
		switch(shooter.mode)
		{
			case SHOOTER_MODE_IDLE:
				osDelay(shooter.taskInterval);
				break;
			case SHOOTER_MODE_BLOCK:
				totalAngle = shooter.triggerMotor->getData(shooter.triggerMotor, "totalAngle"); //重置电机角度为当前累计角度
				shooter.targetTrigAngle = totalAngle - shooter.triggerAngle*0.5f;  //反转
				shooter.triggerMotor->setTarget(shooter.triggerMotor,shooter.targetTrigAngle);
				osDelay(500);   //等待电机堵转恢复
				shooter.mode = SHOOTER_MODE_IDLE;
				break;
			case SHOOTER_MODE_ONCE:   //单发
				if(shooter.fricEnable == false)   //若摩擦轮未开启则先开启
				{
					Bus_RemoteCall("/shooter/setting",{"fric-enable",IM_PTR(bool,true)});
					osDelay(200);     //等待摩擦轮转速稳定
				}
				shooter.targetTrigAngle += shooter.triggerAngle; 
				shooter.triggerMotor->setTarget(shooter.triggerMotor,shooter.targetTrigAngle);
				shooter.mode = SHOOTER_MODE_IDLE;
				break;
			case SHOOTER_MODE_CONTINUE:  //以一定的时间间隔连续发射 
				if(shooter.fricEnable == false)   //若摩擦轮未开启则先开启
				{
					Bus_RemoteCall("/shooter/setting",{"fric-enable",IM_PTR(bool,true)});
					osDelay(200);   //等待摩擦轮转速稳定
				}
				shooter.targetTrigAngle += shooter.triggerAngle;  //增加拨弹电机目标角度
				shooter.triggerMotor->setTarget(shooter.triggerMotor,shooter.targetTrigAngle);
				osDelay(shooter.intervalTime);  
				break;
			default:
				break;
		}
	}	
}

void Shooter_Init(Shooter* shooter, ConfItem* dict)
{
	//任务间隔
	shooter->taskInterval = Conf_GetValue(dict, "task-interval", uint8_t, 20);
	//初始弹速
	shooter->fricSpeed = Conf_GetValue(dict,"fric-speed",float,5450);
	//拨弹轮拨出一发弹丸转角
	shooter->triggerAngle = Conf_GetValue(dict,"trigger-angle",float,1/7.0*360);
	//发射机构电机初始化
	shooter->fricMotors[0] = Motor_Init(Conf_GetPtr(dict, "fric-motor-left", ConfItem));
	shooter->fricMotors[1] = Motor_Init(Conf_GetPtr(dict, "fric-motor-right", ConfItem));
	shooter->triggerMotor = Motor_Init(Conf_GetPtr(dict, "trigger-motor", ConfItem));
	//设置发射机构电机模式
	for(uint8_t i = 0; i<2; i++)
	{
		shooter->fricMotors[i]->changeMode(shooter->fricMotors[i], MOTOR_SPEED_MODE);
	}
	shooter->triggerMotor->changeMode(shooter->triggerMotor,MOTOR_ANGLE_MODE);

	shooter->settingName = Conf_GetPtr(dict,"/shooter/setting",char);
	shooter->settingName = shooter->settingName?shooter->settingName:"/shooter/setting";
	shooter->changeModeName = Conf_GetPtr(dict,"/shooter/mode",char);
	shooter->changeModeName = shooter->changeModeName?shooter->changeModeName:"/shooter/mode";
	shooter->triggerStallName = Conf_GetPtr(dict,"/triggerMotor/stall",char);
	shooter->triggerStallName = shooter->triggerStallName?shooter->triggerStallName:"/triggerMotor/stall";

	//注册回调函数
	Bus_RegisterRemoteFunc(shooter,Shooter_SettingCallback, shooter->settingName);
	Bus_RegisterRemoteFunc(shooter,Shoot_ChangeModeCallback, shooter->changeModeName);
	Bus_RegisterReceiver(shooter,Shoot_StopCallback,"/system/stop");
	Bus_RegisterReceiver(shooter,Shooter_BlockCallback, shooter->triggerStallName);
}

//射击模式
bool Shooter_SettingCallback(const char* name, SoftBusFrame* frame, void* bindData)
{
	Shooter *shooter = (Shooter*)bindData ;
	if(Bus_IsMapKeyExist(frame,"fric-speed"))
	{
		shooter->fricSpeed = *(float*)Bus_GetMapValue(frame,"fric-speed");
	}

	if(Bus_IsMapKeyExist(frame,"trigger-angle"))
	{
		shooter->triggerAngle = *(float*)Bus_GetMapValue(frame,"trigger-angle");
	}

	if(Bus_IsMapKeyExist(frame,"fric-enable"))
	{
		shooter->fricEnable = *(bool*)Bus_GetMapValue(frame,"fric-enable");
		if(shooter->fricEnable == false)
		{
			shooter->fricMotors[0]->setTarget(shooter->fricMotors[0],0);
			shooter->fricMotors[1]->setTarget(shooter->fricMotors[1],0);
		}
		else
		{
			shooter->fricMotors[0]->setTarget(shooter->fricMotors[0],-shooter->fricSpeed);
			shooter->fricMotors[1]->setTarget(shooter->fricMotors[1], shooter->fricSpeed);
		}
	}
	return true;
}
bool Shoot_ChangeModeCallback(const char* name, SoftBusFrame* frame, void* bindData)
{
	Shooter *shooter = (Shooter*)bindData;
	if(Bus_IsMapKeyExist(frame,"mode"))
	{
		char* mode = (char*)Bus_GetMapValue(frame,"mode");
		if(!strcmp(mode,"once") && shooter->mode == SHOOTER_MODE_IDLE)  //空闲时才允许修改模式
		{
			shooter->mode = SHOOTER_MODE_ONCE;
			return true;
		}
		else if(!strcmp(mode,"continue") && shooter->mode == SHOOTER_MODE_IDLE)
		{
			if(!Bus_IsMapKeyExist(frame,"interval-time"))
				return false;
			shooter->intervalTime = *(uint16_t*)Bus_GetMapValue(frame,"interval-time");
			shooter->mode = SHOOTER_MODE_CONTINUE;
			return true;
		}
		else if(!strcmp(mode,"idle") && shooter->mode != SHOOTER_MODE_BLOCK)
		{
			shooter->mode = SHOOTER_MODE_IDLE;
			return true;
		}
	}
	return false;
}

//堵转
void Shooter_BlockCallback(const char* name, SoftBusFrame* frame, void* bindData)
{
	Shooter *shooter = (Shooter*)bindData;
	shooter->mode = SHOOTER_MODE_BLOCK;
}
//急停
void Shoot_StopCallback(const char* name, SoftBusFrame* frame, void* bindData)
{
	Shooter *shooter = (Shooter*)bindData;
	for(uint8_t i = 0; i<2; i++)
	{
		shooter->fricMotors[i]->stop(shooter->fricMotors[i]);
	}
	shooter->triggerMotor->stop(shooter->triggerMotor);
}

