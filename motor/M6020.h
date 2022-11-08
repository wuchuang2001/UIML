#ifndef _M6020_H_
#define _M6020_H_
#include "Motor.h"
#include "PID.h"
#include "config.h"

//各种电机编码值与角度的换算
#define MOTO_M6020_DGR2CODE(dgr) ((int32_t)(dgr*22.7528f)) //8191/360
#define MOTO_M6020_CODE2DGR(code) ((float)(code/22.7528f))

typedef struct _M6020
{
	Motor motor;
	
	float reductionRatio;
	uint16_t id;
	
	int16_t angle,speed;
	
	int16_t lastAngle;//记录上一次得到的角度
	
	int32_t totalAngle;//累计转过的编码器值	
	
	PID speedPID;//速度pid(单级)
	CascadePID anglePID;//角度pid，串级
	
}M6020;

void M6020_Init(Motor* motor, ConfItem* dict);

#endif
