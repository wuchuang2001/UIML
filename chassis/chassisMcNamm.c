#include "chassisMcNamm.h"
#include "slope.h"
#include "config.h"
#include "softbus.h"
#include "cmsis_os.h"
#include "arm_math.h"

#define CHASSIS_ACC2SLOPE(acc) (chassis.taskInterval*(acc)/1000)

typedef enum{
	ChassisMode_Follow, //底盘跟随云台模式
	ChassisMode_Alone, //分离模式，底盘不旋转
	ChassisMode_Spin, //小陀螺模式
	ChassisMode_45 //45度模式，底盘与云台成45度夹角
}ChassisMode;

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
//	//4个电机
//	struct Motor
//	{
//		int16_t targetSpeed;//目标速度
//		int32_t targetAngle;//目标角度
//	}motors[4];
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

Chassis chassis={0};

void Chassis_Init(ConfItem* dict)
{
	//任务间隔
	chassis.taskInterval = Conf_GetValue(dict, chassis/taskInterval, uint8_t, 2);
	//底盘尺寸信息（用于解算轮速）
	chassis.info.wheelbase = Conf_GetValue(dict, chassis/info/wheelbase, float, 0);
	chassis.info.wheeltrack = Conf_GetValue(dict, chassis/info/wheeltrack, float, 0);
	chassis.info.wheelRadius = Conf_GetValue(dict, chassis/info/wheelRadius, float, 76);
	chassis.info.offsetX = Conf_GetValue(dict, chassis/info/offsetX, float, 0);
	chassis.info.offsetY = Conf_GetValue(dict, chassis/info/offsetY, float, 0);
	//移动参数初始化
	chassis.move.maxVx = Conf_GetValue(dict, chassis/move/offsetX, float, 2000);
	chassis.move.maxVy = Conf_GetValue(dict, chassis/move/offsetX, float, 2000);
	chassis.move.maxVw = Conf_GetValue(dict, chassis/move/offsetX, float, 2);
	float xAcc = Conf_GetValue(dict, chassis/move/xAcc, float, 1000);
	float yAcc = Conf_GetValue(dict, chassis/move/yAcc, float, 1000);
	Slope_Init(&chassis.move.xSlope,CHASSIS_ACC2SLOPE(xAcc),0);
	Slope_Init(&chassis.move.ySlope,CHASSIS_ACC2SLOPE(yAcc),0);
//	//电机pid参数初始化
//	Chassis_InitPID();
//	//初始化旋转信息
//	Chassis_InitRotate();
}

////底盘pid参数初始化
//void Chassis_InitPID()
//{
//	PID_Init(&chassis.rotate.pid,0.05,0,0,0,1);
//}

////初始化旋转计算
//void Chassis_InitRotate()
//{
//	chassis.rotate.angle=IMU_GetYaw();
//	chassis.rotate.lastAngle=chassis.rotate.angle;
//	chassis.rotate.zeroAdjust=chassis.rotate.angle;
//	chassis.rotate.totalAngle=0;
//	chassis.rotate.lastTotalAngle=0;
//	chassis.rotate.totalRound=0;
//	chassis.rotate.normalOffsetAngle=0;
//	chassis.rotate.relativeAngle=0;
//}


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
		wheelRPM[0]=(chassis.move.vx+chassis.move.vy-chassis.move.vw*rotateRatio[0])*60/(2*PI*chassis.info.wheelRadius)*19;//FL
		wheelRPM[1]=-(-chassis.move.vx+chassis.move.vy+chassis.move.vw*rotateRatio[1])*60/(2*PI*chassis.info.wheelRadius)*19;//FR
		wheelRPM[2]=(-chassis.move.vx+chassis.move.vy-chassis.move.vw*rotateRatio[2])*60/(2*PI*chassis.info.wheelRadius)*19;//BL
		wheelRPM[3]=-(chassis.move.vx+chassis.move.vy+chassis.move.vw*rotateRatio[3])*60/(2*PI*chassis.info.wheelRadius)*19;//BR
		
//		SoftBus_PublishMap("Motor",{{"chassis/motorFL",&wheelRPM[0],sizeof(float)},
//																{"chassis/motorFR",&wheelRPM[1],sizeof(float)},
//																{"chassis/motorBL",&wheelRPM[2],sizeof(float)},
//																{"chassis/motorBR",&wheelRPM[3],sizeof(float)}});
//		
		osDelayUntil(&tick,chassis.taskInterval);
	}
}
