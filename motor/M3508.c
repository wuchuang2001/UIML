#include "M3508.h"

void M3508_StartStatAngle(Motor *motor);
void M3508_StatAngle(Motor* motor);
void M3508_Update(Motor *motor,void* data);
float M3508_GetReductionRatio(Motor* motor);
uint16_t M3508_GetID(Motor* motor);
float M3508_SpeedPIDCalc(Motor* motor, float reference);
float M3508_AnglePIDCalc(Motor* motor, float reference);

void M3508_Init(Motor* motor, ConfItem* dict)
{
	M3508* m3508 = (M3508*)motor;
	
	motor->getReductionRatio = M3508_GetReductionRatio;
	motor->getID = M3508_GetID;
	motor->speedPIDCalc = M3508_SpeedPIDCalc;
	motor->anglePIDCalc = M3508_AnglePIDCalc;
	motor->startStatAngle = M3508_StartStatAngle;
	motor->statAngle = M3508_StatAngle;
	motor->update = M3508_Update;
	
	m3508->reductionRatio = 19;
	m3508->id = Conf_GetValue(dict, id, uint16_t, 0);
	float speedP, speedI, speedD, speedMaxI, speedMaxOut;
	speedP = Conf_GetValue(dict, speedPID/p, uint16_t, 0);
	speedI = Conf_GetValue(dict, speedPID/i, uint16_t, 0);
	speedD = Conf_GetValue(dict, speedPID/d, uint16_t, 0);
	speedMaxI = Conf_GetValue(dict, speedPID/maxI, uint16_t, 0);
	speedMaxOut = Conf_GetValue(dict, speedPID/maxOut, uint16_t, 0);
	PID_Init(&m3508->speedPID, speedP, speedI, speedD, speedMaxI, speedMaxOut);
	float angleInnerP, angleInnerI, angleInnerD, angleInnerMaxI, angleInnerMaxOut;
	angleInnerP = Conf_GetValue(dict, anglePID/inner/p, uint16_t, 0);
	angleInnerI = Conf_GetValue(dict, anglePID/inner/i, uint16_t, 0);
	angleInnerD = Conf_GetValue(dict, anglePID/inner/d, uint16_t, 0);
	angleInnerMaxI = Conf_GetValue(dict, anglePID/inner/maxI, uint16_t, 0);
	angleInnerMaxOut = Conf_GetValue(dict, anglePID/inner/maxOut, uint16_t, 0);
	PID_Init(&m3508->anglePID.inner, angleInnerP, angleInnerI, angleInnerD, angleInnerMaxI, angleInnerMaxOut);
	float angleOuterP, angleOuterI, angleOuterD, angleOuterMaxI, angleOuterMaxOut;
	angleOuterP = Conf_GetValue(dict, anglePID/outer/p, uint16_t, 0);
	angleOuterI = Conf_GetValue(dict, anglePID/outer/i, uint16_t, 0);
	angleOuterD = Conf_GetValue(dict, anglePID/outer/d, uint16_t, 0);
	angleOuterMaxI = Conf_GetValue(dict, anglePID/outer/maxI, uint16_t, 0);
	angleOuterMaxOut = Conf_GetValue(dict, anglePID/outer/maxOut, uint16_t, 0);
	PID_Init(&m3508->anglePID.outer, angleOuterP, angleOuterI, angleOuterD, angleOuterMaxI, angleOuterMaxOut);
}

//开始统计电机累计角度
void M3508_StartStatAngle(Motor *motor)
{
	M3508* m3508 = (M3508*)motor;
	
	m3508->totalAngle=0;
	m3508->lastAngle=m3508->angle;
}

//统计电机累计转过的圈数
void M3508_StatAngle(Motor* motor)
{
	M3508* m3508 = (M3508*)motor;
	
	int32_t dAngle=0;
	if(m3508->angle-m3508->lastAngle<-4000)
		dAngle=m3508->angle+(8191-m3508->lastAngle);
	else if(m3508->angle-m3508->lastAngle>4000)
		dAngle=-m3508->lastAngle-(8191-m3508->angle);
	else
		dAngle=m3508->angle-m3508->lastAngle;
	//将角度增量加入计数器
	m3508->totalAngle+=dAngle;
	//记录角度
	m3508->lastAngle=m3508->angle;
}

//更新电机数据(可能进行滤波)
void M3508_Update(Motor *motor,void* data)
{
	M3508* m3508 = (M3508*)motor;
	uint8_t* bytes = (uint8_t*)data;
	m3508->angle = (bytes[0]<<8 | bytes[1]);
	m3508->speed = (bytes[2]<<8 | bytes[3]);
}

inline float M3508_GetReductionRatio(Motor* motor)
{
	M3508* m3508 = (M3508*)motor;
	return m3508->reductionRatio;
}

inline uint16_t M3508_GetID(Motor* motor)
{
	M3508* m3508 = (M3508*)motor;
	return m3508->id;
}

float M3508_SpeedPIDCalc(Motor* motor, float reference)
{
	M3508* m3508 = (M3508*)motor;
	PID_SingleCalc(&m3508->speedPID, reference, m3508->speed);
	return m3508->speedPID.output;
}

float M3508_AnglePIDCalc(Motor* motor, float reference)
{
	M3508* m3508 = (M3508*)motor;
	PID_CascadeCalc(&m3508->anglePID, reference, m3508->totalAngle, m3508->speed);
	return m3508->anglePID.output;
}
