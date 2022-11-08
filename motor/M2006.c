#include "M2006.h"

void M2006_StartStatAngle(Motor *motor);
void M2006_StatAngle(Motor* motor);
void M2006_Update(Motor *motor,void* data);
float M2006_GetReductionRatio(Motor* motor);
uint16_t M2006_GetID(Motor* motor);
float M2006_SpeedPIDCalc(Motor* motor, float reference);
float M2006_AnglePIDCalc(Motor* motor, float reference);

void M2006_Init(Motor* motor, ConfItem* dict)
{
	M2006* m2006 = (M2006*)motor;
	
	motor->getReductionRatio = M2006_GetReductionRatio;
	motor->getID = M2006_GetID;
	motor->speedPIDCalc = M2006_SpeedPIDCalc;
	motor->anglePIDCalc = M2006_AnglePIDCalc;
	motor->startStatAngle = M2006_StartStatAngle;
	motor->statAngle = M2006_StatAngle;
	motor->update = M2006_Update;
	
	m2006->reductionRatio = 36;
	m2006->id = Conf_GetValue(dict, id, uint16_t, 0);
	float speedP, speedI, speedD, speedMaxI, speedMaxOut;
	speedP = Conf_GetValue(dict, speedPID/p, uint16_t, 0);
	speedI = Conf_GetValue(dict, speedPID/i, uint16_t, 0);
	speedD = Conf_GetValue(dict, speedPID/d, uint16_t, 0);
	speedMaxI = Conf_GetValue(dict, speedPID/maxI, uint16_t, 0);
	speedMaxOut = Conf_GetValue(dict, speedPID/maxOut, uint16_t, 0);
	PID_Init(&m2006->speedPID, speedP, speedI, speedD, speedMaxI, speedMaxOut);
	float angleInnerP, angleInnerI, angleInnerD, angleInnerMaxI, angleInnerMaxOut;
	angleInnerP = Conf_GetValue(dict, anglePID/inner/p, uint16_t, 0);
	angleInnerI = Conf_GetValue(dict, anglePID/inner/i, uint16_t, 0);
	angleInnerD = Conf_GetValue(dict, anglePID/inner/d, uint16_t, 0);
	angleInnerMaxI = Conf_GetValue(dict, anglePID/inner/maxI, uint16_t, 0);
	angleInnerMaxOut = Conf_GetValue(dict, anglePID/inner/maxOut, uint16_t, 0);
	PID_Init(&m2006->anglePID.inner, angleInnerP, angleInnerI, angleInnerD, angleInnerMaxI, angleInnerMaxOut);
	float angleOuterP, angleOuterI, angleOuterD, angleOuterMaxI, angleOuterMaxOut;
	angleOuterP = Conf_GetValue(dict, anglePID/outer/p, uint16_t, 0);
	angleOuterI = Conf_GetValue(dict, anglePID/outer/i, uint16_t, 0);
	angleOuterD = Conf_GetValue(dict, anglePID/outer/d, uint16_t, 0);
	angleOuterMaxI = Conf_GetValue(dict, anglePID/outer/maxI, uint16_t, 0);
	angleOuterMaxOut = Conf_GetValue(dict, anglePID/outer/maxOut, uint16_t, 0);
	PID_Init(&m2006->anglePID.outer, angleOuterP, angleOuterI, angleOuterD, angleOuterMaxI, angleOuterMaxOut);
}

//开始统计电机累计角度
void M2006_StartStatAngle(Motor *motor)
{
	M2006* m2006 = (M2006*)motor;
	
	m2006->totalAngle=0;
	m2006->lastAngle=m2006->angle;
}

//统计电机累计转过的圈数
void M2006_StatAngle(Motor* motor)
{
	M2006* m2006 = (M2006*)motor;
	
	int32_t dAngle=0;
	if(m2006->angle-m2006->lastAngle<-4000)
		dAngle=m2006->angle+(8191-m2006->lastAngle);
	else if(m2006->angle-m2006->lastAngle>4000)
		dAngle=-m2006->lastAngle-(8191-m2006->angle);
	else
		dAngle=m2006->angle-m2006->lastAngle;
	//将角度增量加入计数器
	m2006->totalAngle+=dAngle;
	//记录角度
	m2006->lastAngle=m2006->angle;
}

//更新电机数据(可能进行滤波)
void M2006_Update(Motor *motor,void* data)
{
	M2006* m2006 = (M2006*)motor;
	uint8_t* bytes = (uint8_t*)data;
	m2006->angle = (bytes[0]<<8 | bytes[1]);
	m2006->speed = (bytes[2]<<8 | bytes[3]);
}

inline float M2006_GetReductionRatio(Motor* motor)
{
	M2006* m2006 = (M2006*)motor;
	return m2006->reductionRatio;
}

inline uint16_t M2006_GetID(Motor* motor)
{
	M2006* m2006 = (M2006*)motor;
	return m2006->id;
}

float M2006_SpeedPIDCalc(Motor* motor, float reference)
{
	M2006* m2006 = (M2006*)motor;
	PID_SingleCalc(&m2006->speedPID, reference, m2006->speed);
	return m2006->speedPID.output;
}

float M2006_AnglePIDCalc(Motor* motor, float reference)
{
	M2006* m2006 = (M2006*)motor;
	PID_CascadeCalc(&m2006->anglePID, reference, m2006->totalAngle, m2006->speed);
	return m2006->anglePID.output;
}
