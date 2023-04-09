#include "config.h"
#include "softbus.h"
#include "pid.h"
#include "motor.h"
#include "cmsis_os.h"

#ifndef PI
#define PI 3.1415926535f
#endif

typedef enum
{
	GIMBAL_ECD_MODE, 	//编码器模式
	GIMBAL_IMU_MODE		//IMU模式
}GimbalCtrlMode; //云台模式

typedef struct _Gimbal
{
	//yaw、pitch电机
	Motor* motors[2];

	GimbalCtrlMode mode;

	int16_t zeroAngle[2];	//零点
	float relativeAngle;	//云台偏离角度
	struct 
	{
		// float eulerAngle[3];	//欧拉角
		float lastEulerAngle[3];
		float totalEulerAngle[3];
		PID pid[2];
	}imu;
	float angle[2];	//云台角度
	uint8_t taskInterval;		
}Gimbal;

void Gimbal_Init(Gimbal* gimbal, ConfItem* dict);
void Gimbal_StartAngleInit(Gimbal* gimbal);
void Gimbal_StatAngle(Gimbal* gimbal, float yaw, float pitch, float roll);

void Gimbal_SoftBusCallback(const char* topic, SoftBusFrame* frame, void* bindData);
void Gimbal_StopCallback(const char* name, SoftBusFrame* frame, void* bindData);

void Gimbal_TaskCallback(void const * argument)
{
	//进入临界区
	portENTER_CRITICAL();
	Gimbal gimbal={0};
	Gimbal_Init(&gimbal, (ConfItem*)argument);
	portEXIT_CRITICAL();
	osDelay(2000);
	Gimbal_StartAngleInit(&gimbal);
	TickType_t tick = xTaskGetTickCount();
	while(1)
	{
		if(gimbal.mode == GIMBAL_ECD_MODE)
		{
			gimbal.motors[0]->setTarget(gimbal.motors[0], gimbal.angle[0]);
			gimbal.motors[1]->setTarget(gimbal.motors[1], gimbal.angle[1]);
		}
		else if(gimbal.mode == GIMBAL_IMU_MODE)
		{
			PID_SingleCalc(&gimbal.imu.pid[0], gimbal.angle[0], gimbal.imu.totalEulerAngle[0]);
			PID_SingleCalc(&gimbal.imu.pid[1], gimbal.angle[1], gimbal.imu.totalEulerAngle[1]);
			gimbal.motors[0]->setTarget(gimbal.motors[0], gimbal.imu.pid[0].output);
			gimbal.motors[1]->setTarget(gimbal.motors[1], gimbal.imu.pid[1].output);
		}
		Bus_BroadcastSend("/motor/getValve", {{"motor", gimbal.motors[0]}, {"totalAngle", &gimbal.relativeAngle}});
		uint16_t temp = gimbal.relativeAngle / 360;
		gimbal.relativeAngle -= temp*360;
		if(gimbal.relativeAngle > 180)
			gimbal.relativeAngle -= 360;
		Bus_BroadcastSend("/gimbal/yaw/relative-angle", {{"angle", &gimbal.relativeAngle}});
		osDelayUntil(&tick,gimbal.taskInterval);
	}
}

void Gimbal_Init(Gimbal* gimbal, ConfItem* dict)
{
	//任务间隔
	gimbal->taskInterval = Conf_GetValue(dict, "taskInterval", uint8_t, 2);

	//云台电机初始化
	gimbal->motors[0] = Motor_Init(Conf_GetPtr(dict, "motorYaw", ConfItem));
	gimbal->motors[1] = Motor_Init(Conf_GetPtr(dict, "motorPitch", ConfItem));

	PID_Init(&gimbal->imu.pid[0], Conf_GetPtr(dict, "motorYaw/imu", ConfItem));
	PID_Init(&gimbal->imu.pid[1], Conf_GetPtr(dict, "motorPitch/imu", ConfItem));

	//初始化云台模式为 编码器模式
	gimbal->mode = Conf_GetValue(dict, "mode", GimbalCtrlMode, GIMBAL_ECD_MODE);
	if(gimbal->mode == GIMBAL_ECD_MODE)
	{
		gimbal->motors[0]->changeMode(gimbal->motors[0], MOTOR_ANGLE_MODE);
		gimbal->motors[1]->changeMode(gimbal->motors[1], MOTOR_ANGLE_MODE);
	}
	else if(gimbal->mode == GIMBAL_IMU_MODE)
	{
		gimbal->motors[0]->changeMode(gimbal->motors[0], MOTOR_SPEED_MODE);
		gimbal->motors[1]->changeMode(gimbal->motors[1], MOTOR_SPEED_MODE);
	}

	Bus_MultiRegisterReceiver(gimbal, Gimbal_SoftBusCallback, {"/imu/euler-angle", "/gimbal"});	
	Bus_RegisterReceiver(gimbal, Gimbal_StopCallback, "/system/stop");
}

void Gimbal_SoftBusCallback(const char* topic, SoftBusFrame* frame, void* bindData)
{
	Gimbal* gimbal = (Gimbal*)bindData;

	if(!strcmp(topic, "/imu/euler-angle"))
	{
		if(!Bus_CheckMapKeys(frame, {"yaw", "pitch", "roll"}))
			return;
		float yaw = *(float*)Bus_GetMapValue(frame, "yaw");
		float pitch = *(float*)Bus_GetMapValue(frame, "pitch");
		float roll = *(float*)Bus_GetMapValue(frame, "roll");
		Gimbal_StatAngle(gimbal, yaw, pitch, roll);
	}
	else if (!strcmp(topic, "/gimbal"))
	{
		if(Bus_IsMapKeyExist(frame, "yaw"))
		{
			gimbal->angle[0] = *(float*)Bus_GetMapValue(frame, "yaw");
		}
		if(Bus_IsMapKeyExist(frame, "pitch"))
		{
			gimbal->angle[1] = *(float*)Bus_GetMapValue(frame, "pitch");
		}
	}
}

void Gimbal_StatAngle(Gimbal* gimbal, float yaw, float pitch, float roll)
{
	float dAngle=0;
	float eulerAngle[3] = {yaw, pitch, roll};
	for (uint8_t i = 0; i < 3; i++)
	{
		if(eulerAngle[i] - gimbal->imu.lastEulerAngle[i] < -180)
			dAngle = eulerAngle[i] + (360 - gimbal->imu.lastEulerAngle[i]);
		else if(eulerAngle[i] - gimbal->imu.lastEulerAngle[i] > 180)
			dAngle = -gimbal->imu.lastEulerAngle[i] - (360 - eulerAngle[i]);
		else
			dAngle = eulerAngle[i] - gimbal->imu.lastEulerAngle[i];
		//将角度增量加入计数器
		gimbal->imu.totalEulerAngle[i] += dAngle;
		//记录角度
		gimbal->imu.lastEulerAngle[i] = eulerAngle[i];
	}
}

void Gimbal_StartAngleInit(Gimbal* gimbal)
{
	float angle[2] = {0};
	for(uint8_t i = 0; i<2; i++)
	{
		angle[i] = gimbal->motors[i]->getData(gimbal->motors[i], "angle");
		angle[i] = (gimbal->zeroAngle[i] - angle[i])/22.7528f;    //未做处理
		if(angle[i] < -180)
			angle[i] += 360;
		else if(angle[i] > 180)
			angle[i] -= 360;
		gimbal->imu.totalEulerAngle[i] = angle[i];
		gimbal->motors[i]->setStartAngle(gimbal->motors[i], angle[i]);
	}	
}

void Gimbal_StopCallback(const char* name, SoftBusFrame* frame, void* bindData)
{
	Gimbal* gimbal = (Gimbal*)bindData;
	for(uint8_t i = 0; i<2; i++)
	{
		gimbal->motors[i]->stop(gimbal->motors[i]);
	}
}
