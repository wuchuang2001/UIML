#include "M3508.h"
#include "softbus.h"

Motor* M3508_Init(ConfItem* dict);

void M3508_StartStatAngle(Motor *motor);
void M3508_StatAngle(Motor* motor);
void M3508_SetTarget(Motor* motor, float targetValue);
void M3508_ChangeMode(Motor* motor, MotorCtrlMode mode);

void M3508_Update(M3508* m3508,uint8_t* data);
void M3508_PIDInit(M3508* m3508, ConfItem* dict);
void M3508_CtrlerCalc(M3508* m3508, float reference);

void M3508_SoftBusCallback(const char* topic, SoftBusFrame* frame, void* bindData)
{
	M3508* m3508 = (M3508*)bindData;
	if(!strcmp(topic, m3508->canInfo.canX[0]))
	{
		uint8_t* data = (uint8_t*)frame->data;
		if(m3508->canInfo.id[0] == data[0])
		{
			M3508_Update(m3508, data+1);
		}
	}
}

void M3508_TimerCallback(void const *argument)
{
	M3508* m3508 = pvTimerGetTimerID((TimerHandle_t)argument); 
	M3508_CtrlerCalc(m3508, m3508->targetValue);
}

Motor* M3508_Init(ConfItem* dict)
{
	M3508* m3508 = MOTOR_MALLOC_PORT(sizeof(M3508));
	memset(m3508,0,sizeof(M3508));
	
	m3508->motor.setTarget = M3508_SetTarget;
	m3508->motor.changeMode = M3508_ChangeMode;
	m3508->motor.startStatAngle = M3508_StartStatAngle;
	m3508->motor.statAngle = M3508_StatAngle;
	
	m3508->reductionRatio = 19;
	uint16_t id = Conf_GetValue(dict, "id", uint16_t, 0);
	m3508->canInfo.id[0] = id+0x200;
	m3508->canInfo.id[1] = (m3508->canInfo.id[0]<0x205)?0x200:0x1FF;
	id = (id-1)%4;
	m3508->canInfo.sendBits =  1<<(2*id-2) | 1<<(2*id-1);
	char* canX = Conf_GetPtr(dict, "canX", char);
	char* send = (char*)MOTOR_MALLOC_PORT((MOTOR_STRLEN_PORT(canX)+4+1)*sizeof(char));
	char* receive = (char*)MOTOR_MALLOC_PORT((MOTOR_STRLEN_PORT(canX)+7+1)*sizeof(char));
	MOTOR_STRCAT_PORT(send, "Send");
	MOTOR_STRCAT_PORT(receive, "Receive");
	m3508->canInfo.canX[0] = receive;
	m3508->canInfo.canX[1] = send;
	m3508->mode = MOTOR_TORQUE_MODE;
	M3508_PIDInit(m3508, dict);
	SoftBus_Subscribe(m3508, M3508_SoftBusCallback, m3508->canInfo.canX[0]);

	return (Motor*)m3508;
}

void M3508_PIDInit(M3508* m3508, ConfItem* dict)
{
	float speedP, speedI, speedD, speedMaxI, speedMaxOut;
	speedP = Conf_GetValue(dict, "speedPID/p", uint16_t, 0);
	speedI = Conf_GetValue(dict, "speedPID/i", uint16_t, 0);
	speedD = Conf_GetValue(dict, "speedPID/d", uint16_t, 0);
	speedMaxI = Conf_GetValue(dict, "speedPID/maxI", uint16_t, 0);
	speedMaxOut = Conf_GetValue(dict, "speedPID/maxOut", uint16_t, 0);
	PID_Init(&m3508->speedPID, speedP, speedI, speedD, speedMaxI, speedMaxOut);
	float angleInnerP, angleInnerI, angleInnerD, angleInnerMaxI, angleInnerMaxOut;
	angleInnerP = Conf_GetValue(dict, "anglePID/inner/p", uint16_t, 0);
	angleInnerI = Conf_GetValue(dict, "anglePID/inner/i", uint16_t, 0);
	angleInnerD = Conf_GetValue(dict, "anglePID/inner/d", uint16_t, 0);
	angleInnerMaxI = Conf_GetValue(dict, "anglePID/inner/maxI", uint16_t, 0);
	angleInnerMaxOut = Conf_GetValue(dict, "anglePID/inner/maxOut", uint16_t, 0);
	PID_Init(&m3508->anglePID.inner, angleInnerP, angleInnerI, angleInnerD, angleInnerMaxI, angleInnerMaxOut);
	float angleOuterP, angleOuterI, angleOuterD, angleOuterMaxI, angleOuterMaxOut;
	angleOuterP = Conf_GetValue(dict, "anglePID/outer/p", uint16_t, 0);
	angleOuterI = Conf_GetValue(dict, "anglePID/outer/i", uint16_t, 0);
	angleOuterD = Conf_GetValue(dict, "anglePID/outer/d", uint16_t, 0);
	angleOuterMaxI = Conf_GetValue(dict, "anglePID/outer/maxI", uint16_t, 0);
	angleOuterMaxOut = Conf_GetValue(dict, "anglePID/outer/maxOut", uint16_t, 0);
	PID_Init(&m3508->anglePID.outer, angleOuterP, angleOuterI, angleOuterD, angleOuterMaxI, angleOuterMaxOut*m3508->reductionRatio);
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

void M3508_CtrlerCalc(M3508* m3508, float reference)
{
	int16_t output;
	if(m3508->mode == MOTOR_SPEED_MODE)
	{
		PID_SingleCalc(&m3508->speedPID, reference*m3508->reductionRatio, m3508->speed);
		output = m3508->speedPID.output;
	}
	else if(m3508->mode == MOTOR_ANGLE_MODE)
	{
		PID_CascadeCalc(&m3508->anglePID, reference, m3508->totalAngle, m3508->speed);
		output = m3508->anglePID.output;
	}
	else if(m3508->mode == MOTOR_TORQUE_MODE)
	{
		output = (int16_t)reference;
	}
	SoftBus_PublishMap(m3508->canInfo.canX[1],{
		{"id", &m3508->canInfo.id[1], sizeof(uint32_t)},
		{"bits", &m3508->canInfo.sendBits, sizeof(uint8_t)},
		{"data", &output, sizeof(int16_t)}
	});
}

void M3508_SetTarget(Motor* motor, float targetValue)
{
	M3508* m3508 = (M3508*)motor;
	if(m3508->mode == MOTOR_ANGLE_MODE)
	{
		m3508->targetValue = M3508_DGR2CODE(targetValue);
	}
	else 
	{
		m3508->targetValue = targetValue;
	}
}

void M3508_ChangeMode(Motor* motor, MotorCtrlMode mode)
{
	M3508* m3508 = (M3508*)motor;
	if(m3508->mode == MOTOR_SPEED_MODE)
	{
		PID_Clear(&m3508->speedPID);
	}
	else if(m3508->mode == MOTOR_ANGLE_MODE)
	{
		PID_Clear(&m3508->anglePID.inner);
		PID_Clear(&m3508->anglePID.outer);
	}
	m3508->mode = mode;
}

//更新电机数据(可能进行滤波)
void M3508_Update(M3508* m3508,uint8_t* data)
{
	m3508->angle = (data[0]<<8 | data[1]);
	m3508->speed = (data[2]<<8 | data[3]);
}
