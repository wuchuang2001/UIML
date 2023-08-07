#include "slope.h"

//初始化斜坡参数
void Slope_Init(Slope *slope,float step,float deadzone)
{
	slope->target=0;
	slope->step=step;
	slope->value=0;
	slope->deadzone=deadzone;
}

//设定斜坡目标
void Slope_SetTarget(Slope *slope,float target)
{
	slope->target=target;
}

//设定斜坡步长
void Slope_SetStep(Slope *slope,float step)
{
	slope->step=step;
}

//计算下一个斜坡值，更新slope->value并返回该值
float Slope_NextVal(Slope *slope)
{
	float error=slope->value-slope->target;//当前值与目标值的差值
	
	if(ABS(error)<slope->deadzone)//若误差在死区内则当前值不发生变化
		return slope->value;
	
	if(ABS(error)<slope->step)//若误差不足步长则当前值直接设为目标值
		slope->value=slope->target;
	else if(error<0)
		slope->value+=slope->step;
	else if(error>0)
		slope->value-=slope->step;
	return slope->value;
}

//获取斜坡当前值
float Slope_GetVal(Slope *slope)
{
	return slope->value;
}
