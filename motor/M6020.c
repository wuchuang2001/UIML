#include "M6020.h"

void M6020_StartStatAngle(Motor *motor);
void M6020_StatAngle(Motor* motor);
void M6020_Update(Motor *motor,void* data);
float M6020_GetReductionRatio(Motor* motor);
uint16_t M6020_GetID(Motor* motor);
float M6020_SpeedPIDCalc(Motor* motor, float reference);
float M6020_AnglePIDCalc(Motor* motor, float reference);

void M6020_Init(Motor* motor, ConfItem* dict)
{
	M6020* m6020 = (M6020*)motor;
	
	motor->getReductionRatio = M6020_GetReductionRatio;
	motor->getID = M6020_GetID;
	motor->speedPIDCalc = M6020_SpeedPIDCalc;
	motor->anglePIDCalc = M6020_AnglePIDCalc;
	motor->startStatAngle = M6020_StartStatAngle;
	motor->statAngle = M6020_StatAngle;
	motor->update = M6020_Update;
	
	m6020->reductionRatio = 1;
	m6020->id = Conf_GetValue(dict, id, uint16_t, 0);
	float speedP, speedI, speedD, speedMaxI, speedMaxOut;
	speedP = Conf_GetValue(dict, speedPID/p, uint16_t, 0);
	speedI = Conf_GetValue(dict, speedPID/i, uint16_t, 0);
	speedD = Conf_GetValue(dict, speedPID/d, uint16_t, 0);
	speedMaxI = Conf_GetValue(dict, speedPID/maxI, uint16_t, 0);
	speedMaxOut = Conf_GetValue(dict, speedPID/maxOut, uint16_t, 0);
	PID_Init(&m6020->speedPID, speedP, speedI, speedD, speedMaxI, speedMaxOut);
	float angleInnerP, angleInnerI, angleInnerD, angleInnerMaxI, angleInnerMaxOut;
	angleInnerP = Conf_GetValue(dict, anglePID/inner/p, uint16_t, 0);
	angleInnerI = Conf_GetValue(dict, anglePID/inner/i, uint16_t, 0);
	angleInnerD = Conf_GetValue(dict, anglePID/inner/d, uint16_t, 0);
	angleInnerMaxI = Conf_GetValue(dict, anglePID/inner/maxI, uint16_t, 0);
	angleInnerMaxOut = Conf_GetValue(dict, anglePID/inner/maxOut, uint16_t, 0);
	PID_Init(&m6020->anglePID.inner, angleInnerP, angleInnerI, angleInnerD, angleInnerMaxI, angleInnerMaxOut);
	float angleOuterP, angleOuterI, angleOuterD, angleOuterMaxI, angleOuterMaxOut;
	angleOuterP = Conf_GetValue(dict, anglePID/outer/p, uint16_t, 0);
	angleOuterI = Conf_GetValue(dict, anglePID/outer/i, uint16_t, 0);
	angleOuterD = Conf_GetValue(dict, anglePID/outer/d, uint16_t, 0);
	angleOuterMaxI = Conf_GetValue(dict, anglePID/outer/maxI, uint16_t, 0);
	angleOuterMaxOut = Conf_GetValue(dict, anglePID/outer/maxOut, uint16_t, 0);
	PID_Init(&m6020->anglePID.outer, angleOuterP, angleOuterI, angleOuterD, angleOuterMaxI, angleOuterMaxOut);
}

//开始统计电机累计角度
void M6020_StartStatAngle(Motor *motor)
{
	M6020* m6020 = (M6020*)motor;
	
	m6020->totalAngle=0;
	m6020->lastAngle=m6020->angle;
}

//统计电机累计转过的圈数
void M6020_StatAngle(Motor* motor)
{
	M6020* m6020 = (M6020*)motor;
	
	int32_t dAngle=0;
	if(m6020->angle-m6020->lastAngle<-4000)
		dAngle=m6020->angle+(8191-m6020->lastAngle);
	else if(m6020->angle-m6020->lastAngle>4000)
		dAngle=-m6020->lastAngle-(8191-m6020->angle);
	else
		dAngle=m6020->angle-m6020->lastAngle;
	//将角度增量加入计数器
	m6020->totalAngle+=dAngle;
	//记录角度
	m6020->lastAngle=m6020->angle;
}

//更新电机数据(可能进行滤波)
void M6020_Update(Motor *motor,void* data)
{
	M6020* m6020 = (M6020*)motor;
	uint8_t* bytes = (uint8_t*)data;
	m6020->angle = (bytes[0]<<8 | bytes[1]);
	m6020->speed = (bytes[2]<<8 | bytes[3]);
}

inline float M6020_GetReductionRatio(Motor* motor)
{
	M6020* m6020 = (M6020*)motor;
	return m6020->reductionRatio;
}

inline uint16_t M6020_GetID(Motor* motor)
{
	M6020* m6020 = (M6020*)motor;
	return m6020->id;
}

float M6020_SpeedPIDCalc(Motor* motor, float reference)
{
	M6020* m6020 = (M6020*)motor;
	PID_SingleCalc(&m6020->speedPID, reference, m6020->speed);
	return m6020->speedPID.output;
}

float M6020_AnglePIDCalc(Motor* motor, float reference)
{
	M6020* m6020 = (M6020*)motor;
	PID_CascadeCalc(&m6020->anglePID, reference, m6020->totalAngle, m6020->speed);
	return m6020->anglePID.output;
}

