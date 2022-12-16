#include "config.h"
#include "softbus.h"
#include "motor.h"
#include "cmsis_os.h"

#ifndef PI
#define PI 3.1415926535f
#endif

typedef enum
{
	GIMBAL_ZERO_FORCE = 0, 		//无力模式
	GIMBAL_ECD_CONTROL = 1, 	//ECD模式
	GIMBAL_IMU_GIMBAL = 2, 		//IMU模式
	
}GIMBAL_MODE_e; //云台模式

typedef struct _Gimbal
{

	float ins_angle[3];
	float target_angle[2];

	//2个电机
	Motor* motors[2];
	
	struct _Yaw
	{		
		float yaw_velocity;				//当前速度 mm/s			
		float maxV; 							//最大速度				
		float Target_Yaw;		
	}yaw;
		
	struct _Pitch
	{		
		float pitch_velocity;			//当前速度 mm/s			
		float maxV; 							//最大速度
		float Target_Pitch;							
	}pitch;
				
	uint8_t taskInterval;
	uint8_t GIMBAL_MODE;
		
}Gimbal;

void Gimbal_Init(Gimbal* gimbal, ConfItem* dict);
void Gimbal_Limit(Gimbal* gimbal);
float Gimbal_angle_zero(float angle, float offset_angle);

void Gimbal_SoftBusCallback(const char* topic, SoftBusFrame* frame, void* bindData);

void Gimbal_TaskCallback(void const * argument)
{
	
	//进入临界区
	portENTER_CRITICAL();
	Gimbal gimbal={0};
	Gimbal_Init(&gimbal, (ConfItem*)argument);
	portEXIT_CRITICAL();
	
	osDelay(2000);
	TickType_t tick = xTaskGetTickCount();

	while(1)
	{
		//角度限制
		Gimbal_Limit(&gimbal);		
		
		//过零处理
		gimbal.yaw.Target_Yaw 		= Gimbal_angle_zero(gimbal.target_angle[0], gimbal.ins_angle[0]);		
		gimbal.pitch.Target_Pitch = Gimbal_angle_zero(gimbal.target_angle[1], gimbal.ins_angle[1]);		
	
		//电机输入
		gimbal.motors[0]->setTarget(gimbal.motors[0], gimbal.yaw.Target_Yaw);
		gimbal.motors[1]->setTarget(gimbal.motors[1], gimbal.pitch.Target_Pitch);

		osDelayUntil(&tick,gimbal.taskInterval);
	}

}

void Gimbal_Init(Gimbal* gimbal, ConfItem* dict)
{

	//任务间隔
	gimbal->taskInterval = Conf_GetValue(dict, "taskInterval", uint8_t, 2);

	gimbal->motors[0] = Motor_Init(Conf_GetPtr(dict, "motorYaw", ConfItem));
	gimbal->motors[1] = Motor_Init(Conf_GetPtr(dict, "motorPitch", ConfItem));

	//移动参数初始化
	gimbal->yaw.maxV 	 = Conf_GetValue(dict, "moveYaw/maxSpeed", float, 2000);
	gimbal->pitch.maxV = Conf_GetValue(dict, "movePitch/maxSpeed", float, 2000);	

	//云台电机初始化
	gimbal->motors[0] = Motor_Init(Conf_GetPtr(dict, "motorYaw", ConfItem));
	gimbal->motors[1] = Motor_Init(Conf_GetPtr(dict, "motorPitch", ConfItem));

	for(uint8_t i = 0; i<2; i++)
	{
		gimbal->motors[i]->changeMode(gimbal->motors[i], MOTOR_ANGLE_MODE);
	}
			
	//初始化云台模式为 云台无力模式
	gimbal->GIMBAL_MODE = GIMBAL_ZERO_FORCE;
	
	Bus_RegisterReceiver(gimbal, Gimbal_SoftBusCallback, "/gimbal/ins_angle");		
}

void Gimbal_SoftBusCallback(const char* topic, SoftBusFrame* frame, void* bindData)
{
	Gimbal* gimbal = (Gimbal*)bindData;

	if(!strcmp(topic, "/gimbal/ins_angle"))
	{
		if(Bus_IsMapKeyExist(frame, "ins_angle[0]"))
			gimbal->ins_angle[0] = *(float*)Bus_GetMapValue(frame, "ins_angle[0]");
		if(Bus_IsMapKeyExist(frame, "ins_angle[1]"))
			gimbal->ins_angle[1] = *(float*)Bus_GetMapValue(frame, "ins_angle[1]");
		if(Bus_IsMapKeyExist(frame, "ins_angle[2]"))
			gimbal->ins_angle[2] = *(float*)Bus_GetMapValue(frame, "ins_angle[2]");				
	}
}

void Gimbal_Limit(Gimbal* gimbal)
{
	
	//yaw角度限制
	if(gimbal->target_angle[0] > PI)
		gimbal->target_angle[0] = -PI;
	if(gimbal->target_angle[0] < -PI)
		gimbal->target_angle[0] = PI;	

	//pitch角度限制
	if(gimbal->target_angle[1] > PI)
		gimbal->target_angle[1] = -PI;
	if(gimbal->target_angle[1] < -PI)
		gimbal->target_angle[1] = PI;	
			
}
	
float Gimbal_angle_zero(float angle, float offset_angle)
{
	float relative_angle = angle - offset_angle;

	if(relative_angle >  1.25f * PI)
	{
		relative_angle -= 2*PI;
	}
	else if(relative_angle < - 1.25f * PI)
	{
		relative_angle += 2*PI;		
	}

	return relative_angle;
}

