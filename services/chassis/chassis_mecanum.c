#include "slope.h"
#include "config.h"
#include "softbus.h"
#include "motor.h"
#include "cmsis_os.h"
#include "arm_math.h"

#define CHASSIS_ACC2SLOPE(taskInterval,acc) ((taskInterval)*(acc)/1000) //mm/s2

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
	
	float relativeAngle; //与底盘的偏离角，单位度
	
	uint8_t taskInterval;
	
}Chassis;

void Chassis_Init(Chassis* chassis, ConfItem* dict);
void Chassis_UpdateSlope(Chassis* chassis);
void Chassis_SoftBusCallback(const char* topic, SoftBusFrame* frame, void* bindData);

//底盘任务回调函数
void Chassis_TaskCallback(void const * argument)
{
	//进入临界区
	portENTER_CRITICAL();
	Chassis chassis={0};
	Chassis_Init(&chassis, (ConfItem*)argument);
	portEXIT_CRITICAL();
	
	osDelay(2000);
	TickType_t tick = xTaskGetTickCount();
	while(1)
	{		
		/*************计算底盘平移速度**************/
		
		Chassis_UpdateSlope(&chassis);//更新运动斜坡函数数据

		//将云台坐标系下平移速度解算到底盘平移速度(根据云台偏离角)
		float gimbalAngleSin=arm_sin_f32(chassis.relativeAngle*PI/180);
		float gimbalAngleCos=arm_cos_f32(chassis.relativeAngle*PI/180);
		chassis.move.vx=Slope_GetVal(&chassis.move.xSlope) * gimbalAngleCos
									 +Slope_GetVal(&chassis.move.ySlope) * gimbalAngleSin;
		chassis.move.vy=-Slope_GetVal(&chassis.move.xSlope) * gimbalAngleSin
									 +Slope_GetVal(&chassis.move.ySlope) * gimbalAngleCos;
		float vw = chassis.move.vw/180*PI;
		
		/*************解算各轮子转速**************/
		
		float rotateRatio[4];
		rotateRatio[0]=(chassis.info.wheelbase+chassis.info.wheeltrack)/2.0f-chassis.info.offsetY+chassis.info.offsetX;
		rotateRatio[1]=(chassis.info.wheelbase+chassis.info.wheeltrack)/2.0f-chassis.info.offsetY-chassis.info.offsetX;
		rotateRatio[2]=(chassis.info.wheelbase+chassis.info.wheeltrack)/2.0f+chassis.info.offsetY+chassis.info.offsetX;
		rotateRatio[3]=(chassis.info.wheelbase+chassis.info.wheeltrack)/2.0f+chassis.info.offsetY-chassis.info.offsetX;
		float wheelRPM[4];
		wheelRPM[0]=(chassis.move.vx+chassis.move.vy-vw*rotateRatio[0])*60/(2*PI*chassis.info.wheelRadius);//FL
		wheelRPM[1]=-(-chassis.move.vx+chassis.move.vy+vw*rotateRatio[1])*60/(2*PI*chassis.info.wheelRadius);//FR
		wheelRPM[2]=(-chassis.move.vx+chassis.move.vy-vw*rotateRatio[2])*60/(2*PI*chassis.info.wheelRadius);//BL
		wheelRPM[3]=-(chassis.move.vx+chassis.move.vy+vw*rotateRatio[3])*60/(2*PI*chassis.info.wheelRadius);//BR
		
		for(uint8_t i = 0; i<4; i++)
		{
			chassis.motors[i]->setTarget(chassis.motors[i], wheelRPM[i]);
		}
		
		osDelayUntil(&tick,chassis.taskInterval);
	}
}

void Chassis_Init(Chassis* chassis, ConfItem* dict)
{
	//任务间隔
	chassis->taskInterval = Conf_GetValue(dict, "taskInterval", uint8_t, 2);
	//底盘尺寸信息（用于解算轮速）
	chassis->info.wheelbase = Conf_GetValue(dict, "info/wheelbase", float, 0);
	chassis->info.wheeltrack = Conf_GetValue(dict, "info/wheeltrack", float, 0);
	chassis->info.wheelRadius = Conf_GetValue(dict, "info/wheelRadius", float, 76);
	chassis->info.offsetX = Conf_GetValue(dict, "info/offsetX", float, 0);
	chassis->info.offsetY = Conf_GetValue(dict, "info/offsetY", float, 0);
	//移动参数初始化
	chassis->move.maxVx = Conf_GetValue(dict, "move/maxVx", float, 2000);
	chassis->move.maxVy = Conf_GetValue(dict, "move/maxVy", float, 2000);
	chassis->move.maxVw = Conf_GetValue(dict, "move/maxVw", float, 2);
	//底盘加速度初始化
	float xAcc = Conf_GetValue(dict, "move/xAcc", float, 1000);
	float yAcc = Conf_GetValue(dict, "move/yAcc", float, 1000);
	Slope_Init(&chassis->move.xSlope, CHASSIS_ACC2SLOPE(chassis->taskInterval, xAcc),0);
	Slope_Init(&chassis->move.ySlope, CHASSIS_ACC2SLOPE(chassis->taskInterval, yAcc),0);
	//底盘电机初始化
	chassis->motors[0] = Motor_Init(Conf_GetPtr(dict, "motorFL", ConfItem));
	chassis->motors[1] = Motor_Init(Conf_GetPtr(dict, "motorFR", ConfItem));
	chassis->motors[2] = Motor_Init(Conf_GetPtr(dict, "motorBL", ConfItem));
	chassis->motors[3] = Motor_Init(Conf_GetPtr(dict, "motorBR", ConfItem));
	//设置底盘电机为速度模式
	for(uint8_t i = 0; i<4; i++)
	{
		chassis->motors[i]->changeMode(chassis->motors[i], MOTOR_SPEED_MODE);
	}
	SoftBus_Subscribe(chassis, Chassis_SoftBusCallback, "chassis");
}


//更新斜坡计算速度
void Chassis_UpdateSlope(Chassis* chassis)
{
	Slope_NextVal(&chassis->move.xSlope);
	Slope_NextVal(&chassis->move.ySlope);
}

void Chassis_SoftBusCallback(const char* topic, SoftBusFrame* frame, void* bindData)
{
	Chassis* chassis = (Chassis*)bindData;
//	if(!strcmp(topic, "chassis"))
//	{
		if(SoftBus_IsMapKeyExist(frame, "relativeAngle"))
			chassis->relativeAngle = *(float*)SoftBus_GetMapValue(frame, "relativeAngle");
		if(SoftBus_IsMapKeyExist(frame, "vx"))
			Slope_SetTarget(&chassis->move.xSlope, *(float*)SoftBus_GetMapValue(frame, "vx"));
		if(SoftBus_IsMapKeyExist(frame, "vy"))
			Slope_SetTarget(&chassis->move.ySlope, *(float*)SoftBus_GetMapValue(frame, "vy"));
		if(SoftBus_IsMapKeyExist(frame, "vw"))
			chassis->move.vw = *(float*)SoftBus_GetMapValue(frame, "vw");
		if(SoftBus_IsMapKeyExist(frame, "ax"))
			Slope_SetStep(&chassis->move.xSlope, CHASSIS_ACC2SLOPE(chassis->taskInterval, *(float*)SoftBus_GetMapValue(frame, "ax")));
		if(SoftBus_IsMapKeyExist(frame, "ay"))
			Slope_SetStep(&chassis->move.ySlope, CHASSIS_ACC2SLOPE(chassis->taskInterval, *(float*)SoftBus_GetMapValue(frame, "ay")));
//	}
}
