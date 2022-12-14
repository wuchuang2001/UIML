#include "softbus.h"
#include "motor.h"
#include "config.h"
#include "pid.h"
#include <stdio.h>

//各种电机编码值与角度的换算
#define DCMOTOR_DGR2CODE(dgr,rdcr,encode) ((int32_t)((dgr)*encode/360.0f*(rdcr))) //减速比*编码器一圈值/360
#define DCMOTOR_CODE2DGR(code,rdcr,encode) ((float)((code)/(encode/360.0f*(rdcr))))

typedef struct
{
	uint8_t timX;
	uint8_t	channelX;
}TimInfo;

typedef struct _DCmotor
{
	Motor motor;
	TimInfo encodeTim,posRotateTim,negRotateTim;
	float reductionRatio;
	float circleEncode;

	MotorCtrlMode mode;
	
	uint32_t angle,lastAngle;//记录上一次得到的角度
	
	int16_t speed;
	
	int32_t totalAngle;//累计转过的编码器值
	
	float  targetValue;//目标值(输出轴/速度/角度(单位度))
	
	PID speedPID;//速度pid(单级)
	CascadePID anglePID;//角度pid，串级
	
}DcMotor;

void DcMotor_TimInit(DcMotor* dcMotor, ConfItem* dict);
void DcMotor_PIDInit(DcMotor* dcMotor, ConfItem* dict);
void DcMotor_SetStartAngle(Motor *motor, float startAngle);
void DcMotor_SetTarget(Motor* motor, float targetValue);
void DcMotor_ChangeMode(Motor* motor, MotorCtrlMode mode);

void DcMotor_StatAngle(DcMotor* dcMotor);
void DcMotor_CtrlerCalc(DcMotor* dcMotor, float reference);

//软件定时器回调函数
void DcMotor_TimerCallback(void const *argument)
{
	DcMotor* dcMotor = pvTimerGetTimerID((TimerHandle_t)argument); 
	DcMotor_StatAngle(dcMotor);
	DcMotor_CtrlerCalc(dcMotor, dcMotor->targetValue);
}

Motor* DcMotor_Init(ConfItem* dict)
{
	//分配子类内存空间
	DcMotor* dcMotor = MOTOR_MALLOC_PORT(sizeof(DcMotor));
	memset(dcMotor,0,sizeof(DcMotor));
	//子类多态
	dcMotor->motor.setTarget = DcMotor_SetTarget;
	dcMotor->motor.changeMode = DcMotor_ChangeMode;
	dcMotor->motor.setStartAngle = DcMotor_SetStartAngle;
	//电机减速比
	dcMotor->reductionRatio = Conf_GetValue(dict, "reductionRatio", float, 19.0f);//如果未配置电机减速比参数，则使用原装电机默认减速比
	dcMotor->circleEncode = Conf_GetValue(dict, "maxEncode", float, 48.0f); //倍频后编码器转一圈的最大值
	//初始化电机绑定tim信息
	DcMotor_TimInit(dcMotor, dict);
	//设置电机默认模式为速度模式
	dcMotor->mode = MOTOR_TORQUE_MODE;
	//初始化电机pid
	DcMotor_PIDInit(dcMotor, dict);
	
	//开启软件定时器
	osTimerDef(DCMOTOR, DcMotor_TimerCallback);
	osTimerStart(osTimerCreate(osTimer(DCMOTOR), osTimerPeriodic,dcMotor), 2);

	return (Motor*)dcMotor;
}

//初始化pid
void DcMotor_PIDInit(DcMotor* dcMotor, ConfItem* dict)
{
	PID_Init(&dcMotor->speedPID, Conf_GetPtr(dict, "speedPID", ConfItem));
	PID_Init(&dcMotor->anglePID.inner, Conf_GetPtr(dict, "anglePID/inner", ConfItem));
	PID_Init(&dcMotor->anglePID.outer, Conf_GetPtr(dict, "anglePID/outer", ConfItem));
	PID_SetMaxOutput(&dcMotor->anglePID.outer, dcMotor->anglePID.outer.maxOutput*dcMotor->reductionRatio);//将输出轴速度限幅放大到转子上
}

//初始化tim
void DcMotor_TimInit(DcMotor* dcMotor, ConfItem* dict)
{
	dcMotor->posRotateTim.timX = Conf_GetValue(dict,"posRotateTim/tim-x",uint8_t,0);
	dcMotor->posRotateTim.channelX = Conf_GetValue(dict,"posRotateTim/channel-x",uint8_t,0);
	dcMotor->negRotateTim.timX = Conf_GetValue(dict,"negRotateTim/tim-x",uint8_t,0);
	dcMotor->negRotateTim.channelX = Conf_GetValue(dict,"negRotateTim/channel-x",uint8_t,0);
	dcMotor->encodeTim.channelX = Conf_GetValue(dict,"encodeTim/tim-x",uint8_t,0);
}

//开始统计电机累计角度
void DcMotor_SetStartAngle(Motor *motor, float startAngle)
{
	DcMotor* dcMotor = (DcMotor*)motor;
	
	dcMotor->totalAngle=DCMOTOR_DGR2CODE(startAngle, dcMotor->reductionRatio,dcMotor->circleEncode);
	dcMotor->lastAngle=dcMotor->angle;
}

//统计电机累计转过的圈数
void DcMotor_StatAngle(DcMotor* dcMotor)
{
	int32_t dAngle=0;
	uint32_t autoReload=0;
	
	Bus_RemoteCall("/tim/encode",{{"tim-x",&dcMotor->encodeTim.timX},{"count",&dcMotor->angle},{"auto-reload",&autoReload}});
	
	if(dcMotor->angle - dcMotor->lastAngle < -(autoReload/2.0f))
		dAngle = dcMotor->angle+(autoReload-dcMotor->lastAngle);
	else if(dcMotor->angle - dcMotor->lastAngle > (autoReload/2.0f))
		dAngle = -dcMotor->lastAngle - (autoReload - dcMotor->angle);
	else
		dAngle = dcMotor->angle - dcMotor->lastAngle;
	//将角度增量加入计数器
	dcMotor->totalAngle += dAngle;
	//计算速度
	dcMotor->speed = (float)dAngle/(dcMotor->circleEncode*dcMotor->reductionRatio)*500*60;//rpm  
	//记录角度
	dcMotor->lastAngle=dcMotor->angle;
}

//控制器根据模式计算输出
void DcMotor_CtrlerCalc(DcMotor* dcMotor, float reference)
{
	float output=0;
	if(dcMotor->mode == MOTOR_SPEED_MODE)
	{
		PID_SingleCalc(&dcMotor->speedPID, reference, dcMotor->speed);
		output = dcMotor->speedPID.output;
	}
	else if(dcMotor->mode == MOTOR_ANGLE_MODE)
	{
		PID_CascadeCalc(&dcMotor->anglePID, reference, dcMotor->totalAngle, dcMotor->speed);
		output = dcMotor->anglePID.output;
	}
	else if(dcMotor->mode == MOTOR_TORQUE_MODE)
	{
		output = reference;
	}
  
	if(output>0)
	{
		Bus_BroadcastSend("/tim/pwm/set-duty",{{"tim-x",&dcMotor->posRotateTim.timX},{"channel-x",&dcMotor->posRotateTim.channelX},{"duty",&output}});
		Bus_BroadcastSend("/tim/pwm/set-duty",{{"tim-x",&dcMotor->posRotateTim.timX},{"channel-x",&dcMotor->negRotateTim.channelX},{"duty",IM_PTR(float, 0)}});
	}
	else
	{
		output = ABS(output);
		Bus_BroadcastSend("/tim/pwm/set-duty",{{"tim-x",&dcMotor->posRotateTim.timX},{"channel-x",&dcMotor->posRotateTim.channelX},{"duty",IM_PTR(float, 0)});
		Bus_BroadcastSend("/tim/pwm/set-duty",{{"tim-x",&dcMotor->posRotateTim.timX},{"channel-x",&dcMotor->negRotateTim.channelX},{"duty",&output}});
	}

}

//设置电机期望值
void DcMotor_SetTarget(Motor* motor, float targetValue)
{
	DcMotor* dcMotor = (DcMotor*)motor;
	if(dcMotor->mode == MOTOR_ANGLE_MODE)
	{
		dcMotor->targetValue = DCMOTOR_DGR2CODE(targetValue, dcMotor->reductionRatio,dcMotor->circleEncode);
	}
	else if(dcMotor->mode == MOTOR_SPEED_MODE)
	{
		dcMotor->targetValue = targetValue*dcMotor->reductionRatio;
	}
	else
	{
		dcMotor->targetValue = targetValue;
	}
}

//切换电机模式
void DcMotor_ChangeMode(Motor* motor, MotorCtrlMode mode)
{
	DcMotor* dcMotor = (DcMotor*)motor;
	if(dcMotor->mode == MOTOR_SPEED_MODE)
	{
		PID_Clear(&dcMotor->speedPID);
	}
	else if(dcMotor->mode == MOTOR_ANGLE_MODE)
	{
		PID_Clear(&dcMotor->anglePID.inner);
		PID_Clear(&dcMotor->anglePID.outer);
	}
	dcMotor->mode = mode;
}





