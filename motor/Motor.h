#ifndef _MOTOR_H_
#define _MOTOR_H_
#include "config.h"

#include "stdint.h"

typedef struct _Motor
{
	float (*getReductionRatio)(struct _Motor* motor);
	uint16_t (*getID)(struct _Motor* motor);
	
	float (*speedPIDCalc)(struct _Motor* motor,float reference);
	float (*anglePIDCalc)(struct _Motor* motor,float reference);
	
	void (*startStatAngle)(struct _Motor* motor);
	void (*statAngle)(struct _Motor* motor);
	void (*update)(struct _Motor* motor,void* data);
}Motor;

void Motor_Init(Motor* motor, ConfItem* dict);

#endif
