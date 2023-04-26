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

	uint16_t zeroAngle[2];	//零点
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

void Gimbal_BroadcastCallback(const char* name, SoftBusFrame* frame, void* bindData);
bool Gimbal_SettingCallback(const char* name, SoftBusFrame* frame, void* bindData);
bool Gimbal_StopCallback(const char* name, SoftBusFrame* frame, void* bindData);

void Gimbal_TaskCallback(void const * argument)
{
	//进入临界区
	portENTER_CRITICAL();
	Gimbal gimbal={0};
	Gimbal_Init(&gimbal, (ConfItem*)argument);
	portEXIT_CRITICAL();
	osDelay(2000);
	Gimbal_StartAngleInit(&gimbal); //计算云台零点
	//计算好云台零点后，更改电机模式
	if(gimbal.mode == GIMBAL_ECD_MODE)
	{
		gimbal.motors[0]->changeMode(gimbal.motors[0], MOTOR_ANGLE_MODE);
		gimbal.motors[1]->changeMode(gimbal.motors[1], MOTOR_ANGLE_MODE);
	}
	else if(gimbal.mode == GIMBAL_IMU_MODE)
	{
		gimbal.motors[0]->changeMode(gimbal.motors[0], MOTOR_SPEED_MODE);
		gimbal.motors[1]->changeMode(gimbal.motors[1], MOTOR_SPEED_MODE);
	}
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
		gimbal.relativeAngle = gimbal.motors[0]->getData(gimbal.motors[0], "totalAngle");
		int16_t turns = (int32_t)gimbal.relativeAngle / 360; //转数
		turns = turns < 0 ? turns - 1 : turns; //如果是负数多减一圈使偏离角变成正数
		gimbal.relativeAngle -= turns*360; //0-360度
		Bus_BroadcastSend("/gimbal/yaw/relative-angle", {{"angle", &gimbal.relativeAngle}});
		osDelay(gimbal.taskInterval);
	}
}

void Gimbal_Init(Gimbal* gimbal, ConfItem* dict)
{
	//任务间隔
	gimbal->taskInterval = Conf_GetValue(dict, "taskInterval", uint8_t, 2);

	//云台零点
	gimbal->zeroAngle[0] = Conf_GetValue(dict, "zero-yaw", uint16_t, 0);
	gimbal->zeroAngle[1] = Conf_GetValue(dict, "zero-pitch", uint16_t, 0);

	//云台电机初始化
	gimbal->motors[0] = Motor_Init(Conf_GetPtr(dict, "motor-yaw", ConfItem));
	gimbal->motors[1] = Motor_Init(Conf_GetPtr(dict, "motor-pitch", ConfItem));

	PID_Init(&gimbal->imu.pid[0], Conf_GetPtr(dict, "motor-yaw/imu", ConfItem));
	PID_Init(&gimbal->imu.pid[1], Conf_GetPtr(dict, "motor-pitch/imu", ConfItem));

	//初始化云台模式为 编码器模式
	gimbal->mode = Conf_GetValue(dict, "mode", GimbalCtrlMode, GIMBAL_ECD_MODE);
	//不在这里设置模式，因为在未设置好零点前，pid会驱使电机达到编码器的零点或者imu的初始化零点

	Bus_RegisterReceiver(gimbal, Gimbal_BroadcastCallback, "/imu/euler-angle");
	Bus_RegisterRemoteFunc(gimbal, Gimbal_SettingCallback, "/gimbal");
	Bus_RegisterRemoteFunc(gimbal, Gimbal_StopCallback, "/system/stop"); //急停
}

void Gimbal_BroadcastCallback(const char* name, SoftBusFrame* frame, void* bindData)
{
	Gimbal* gimbal = (Gimbal*)bindData;

	if(!strcmp(name, "/imu/euler-angle"))
	{
		if(!Bus_CheckMapKeys(frame, {"yaw", "pitch", "roll"}))
			return;
		float yaw = *(float*)Bus_GetMapValue(frame, "yaw");
		float pitch = *(float*)Bus_GetMapValue(frame, "pitch");
		float roll = *(float*)Bus_GetMapValue(frame, "roll");
		Gimbal_StatAngle(gimbal, yaw, pitch, roll);
	}
}
bool Gimbal_SettingCallback(const char* name, SoftBusFrame* frame, void* bindData)
{
	Gimbal* gimbal = (Gimbal*)bindData;

	if(Bus_IsMapKeyExist(frame, "yaw"))
	{
		gimbal->angle[0] = *(float*)Bus_GetMapValue(frame, "yaw");
	}
	if(Bus_IsMapKeyExist(frame, "pitch"))
	{
		gimbal->angle[1] = *(float*)Bus_GetMapValue(frame, "pitch");
	}
	return true;
}

bool Gimbal_StopCallback(const char* name, SoftBusFrame* frame, void* bindData) //急停
{
	Gimbal* gimbal = (Gimbal*)bindData;
	for(uint8_t i = 0; i<2; i++)
	{
		gimbal->motors[i]->stop(gimbal->motors[i]);
	}
	return true;
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
	for(uint8_t i = 0; i<2; i++)
	{
		float angle = 0;
		angle = gimbal->motors[i]->getData(gimbal->motors[i], "angle");
		angle = angle - (float)gimbal->zeroAngle[i]*360/8191;  //计算距离零点的角度  
		if(angle < -180)  //将角度转化到-180~180度，这样可以使云台以最近距离旋转至零点
			angle += 360;
		else if(angle > 180)
			angle -= 360;
		gimbal->imu.totalEulerAngle[i] = angle;
		gimbal->motors[i]->setStartAngle(gimbal->motors[i], angle);
	}	
}
