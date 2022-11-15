#include "chassisMecanum.h"
#include "slope.h"
#include "config.h"
#include "softbus.h"
#include "Motor.h"
#include "cmsis_os.h"
#include "arm_math.h"

#define CHASSIS_ACC2SLOPE(acc) (chassis.taskInterval*(acc)/1000) //mm/s2

typedef struct _Chassis
{
	//底盘尺寸信息
	struct Info
	{
		float wheelbase;//轴距
		float wheeltrack;//轮距
		float wheelRadius;//轮半径
		float offsetX;//重心在xy轴上的偏移
		float offsetY;
	}info;
	//4个电机
	Motor* motors[4];
	//底盘移动信息
	struct Move
	{
		float vx;//当前左右平移速度 mm/s
		float vy;//当前前后移动速度 mm/s
		float vw;//当前旋转速度 rad/s
		
		float maxVx,maxVy,maxVw; //三个分量最大速度
		Slope xSlope,ySlope; //斜坡
	}move;
	
	float relativeAngle; //云台与底盘的偏离角，单位度
	
	uint8_t taskInterval;
	
}Chassis;

void Chassis_DataCallback(const char* topic, SoftBusFrame* frame, void* userData);

Chassis chassis={0};

void Chassis_Init(ConfItem* dict)
{
	//任务间隔
	chassis.taskInterval = Conf_GetValue(dict, "chassis/taskInterval", uint8_t, 2);
	//底盘尺寸信息（用于解算轮速）
	chassis.info.wheelbase = Conf_GetValue(dict, "chassis/info/wheelbase", float, 0);
	chassis.info.wheeltrack = Conf_GetValue(dict, "chassis/info/wheeltrack", float, 0);
	chassis.info.wheelRadius = Conf_GetValue(dict, "chassis/info/wheelRadius", float, 76);
	chassis.info.offsetX = Conf_GetValue(dict, "chassis/info/offsetX", float, 0);
	chassis.info.offsetY = Conf_GetValue(dict, "chassis/info/offsetY", float, 0);
	//移动参数初始化
	chassis.move.maxVx = Conf_GetValue(dict, "chassis/move/offsetX", float, 2000);
	chassis.move.maxVy = Conf_GetValue(dict, "chassis/move/offsetX", float, 2000);
	chassis.move.maxVw = Conf_GetValue(dict, "chassis/move/offsetX", float, 2);
	float xAcc = Conf_GetValue(dict, "chassis/move/xAcc", float, 1000);
	float yAcc = Conf_GetValue(dict, "chassis/move/yAcc", float, 1000);
	Slope_Init(&chassis.move.xSlope,CHASSIS_ACC2SLOPE(xAcc),0);
	Slope_Init(&chassis.move.ySlope,CHASSIS_ACC2SLOPE(yAcc),0);
	Motor_Init(chassis.motors[0], Conf_GetPtr(dict, "chassis/motorFL", ConfItem));
	Motor_Init(chassis.motors[1], Conf_GetPtr(dict, "chassis/motorFR", ConfItem));
	Motor_Init(chassis.motors[2], Conf_GetPtr(dict, "chassis/motorBL", ConfItem));
	Motor_Init(chassis.motors[3], Conf_GetPtr(dict, "chassis/motorBR", ConfItem));
	for(uint8_t i = 0; i<4; i++)
	{
		chassis.motors[i]->changeCtrler(chassis.motors[i], speed);
	}
	SoftBus_Subscribe(NULL, Chassis_DataCallback, "chassis");
}


//更新斜坡计算速度
void Chassis_UpdateSlope()
{
	Slope_NextVal(&chassis.move.xSlope);
	Slope_NextVal(&chassis.move.ySlope);
}

//底盘任务回调函数
void Chassis_TaskCallback(void const * argument)
{
	Chassis_Init((ConfItem*)argument);
	TickType_t tick = xTaskGetTickCount();
	while(1)
	{		
		/*************计算底盘平移速度**************/
		
		Chassis_UpdateSlope();//更新运动斜坡函数数据

		//将云台坐标系下平移速度解算到底盘平移速度(根据云台偏离角)
		float gimbalAngleSin=arm_sin_f32(chassis.relativeAngle*PI/180);
		float gimbalAngleCos=arm_cos_f32(chassis.relativeAngle*PI/180);
		chassis.move.vx=Slope_GetVal(&chassis.move.xSlope) * gimbalAngleCos
									 +Slope_GetVal(&chassis.move.ySlope) * gimbalAngleSin;
		chassis.move.vy=-Slope_GetVal(&chassis.move.xSlope) * gimbalAngleSin
									 +Slope_GetVal(&chassis.move.ySlope) * gimbalAngleCos;
		
		/*************解算各轮子转速**************/
		
		float rotateRatio[4];
		rotateRatio[0]=(chassis.info.wheelbase+chassis.info.wheeltrack)/2.0f-chassis.info.offsetY+chassis.info.offsetX;
		rotateRatio[1]=(chassis.info.wheelbase+chassis.info.wheeltrack)/2.0f-chassis.info.offsetY-chassis.info.offsetX;
		rotateRatio[2]=(chassis.info.wheelbase+chassis.info.wheeltrack)/2.0f+chassis.info.offsetY+chassis.info.offsetX;
		rotateRatio[3]=(chassis.info.wheelbase+chassis.info.wheeltrack)/2.0f+chassis.info.offsetY-chassis.info.offsetX;
		float wheelRPM[4];
		wheelRPM[0]=(chassis.move.vx+chassis.move.vy-chassis.move.vw*rotateRatio[0])*60/(2*PI*chassis.info.wheelRadius);//FL
		wheelRPM[1]=-(-chassis.move.vx+chassis.move.vy+chassis.move.vw*rotateRatio[1])*60/(2*PI*chassis.info.wheelRadius);//FR
		wheelRPM[2]=(-chassis.move.vx+chassis.move.vy-chassis.move.vw*rotateRatio[2])*60/(2*PI*chassis.info.wheelRadius);//BL
		wheelRPM[3]=-(chassis.move.vx+chassis.move.vy+chassis.move.vw*rotateRatio[3])*60/(2*PI*chassis.info.wheelRadius);//BR
		
		for(uint8_t i = 0; i<4; i++)
		{
			chassis.motors[i]->ctrlerCalc(chassis.motors[i], wheelRPM[i]);
		}
		
		osDelayUntil(&tick,chassis.taskInterval);
	}
}

void Chassis_DataCallback(const char* topic, SoftBusFrame* frame, void* userData)
{
//	if(!strcmp(topic, "chassis"))
//	{
		const SoftBusItem* item = SoftBus_GetItem(frame, "relativeAngle");
		if(!item)
		{
			chassis.relativeAngle = *(float*)item->data;
		}
		item = SoftBus_GetItem(frame, "vx");
		if(!item)
		{
			Slope_SetTarget(&chassis.move.xSlope, *(float*)item->data);
		}
		item = SoftBus_GetItem(frame, "vy");
		if(!item)
		{
			Slope_SetTarget(&chassis.move.ySlope, *(float*)item->data);
		}
		item = SoftBus_GetItem(frame, "vw");
		if(!item)
		{
			chassis.move.vw = *(float*)item->data;
		}
		item = SoftBus_GetItem(frame, "ax");
		if(!item)
		{
			Slope_SetStep(&chassis.move.xSlope, CHASSIS_ACC2SLOPE(*(float*)item->data));
		}
		item = SoftBus_GetItem(frame, "ay");
		if(!item)
		{
			Slope_SetStep(&chassis.move.ySlope, CHASSIS_ACC2SLOPE(*(float*)item->data));
		}
//	}
}
