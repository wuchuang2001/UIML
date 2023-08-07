#ifndef _SLOPE_H_
#define _SLOPE_H_

#ifndef ABS
#define ABS(x) ((x)>=0?(x):-(x))
#endif

//斜坡结构体
typedef struct{
	float target; //目标值
	float step; //步进值
	float value; //当前值
	float deadzone; //死区，若差值小于该值则不进行增减
}Slope;

void Slope_Init(Slope *slope,float step,float deadzone);
void Slope_SetTarget(Slope *slope,float target);
void Slope_SetStep(Slope *slope,float step);
float Slope_NextVal(Slope *slope);
float Slope_GetVal(Slope *slope);

#endif
