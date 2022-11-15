#include "M6020.h"
#include "softbus.h"

void M6020_StartStatAngle(Motor *motor);
void M6020_StatAngle(Motor* motor);
void M6020_CtrlerCalc(Motor* motor, float reference);
void M6020_ChangeCtrler(Motor* motor, Ctrler ctrler);

void M6020_Update(M6020* m6020,uint8_t* data);
void M6020_PIDInit(M6020* m6020, ConfItem* dict);

void m6020DataCallback(const char* topic, SoftBusFrame* frame, void* userData)
{
	M6020* m6020 = (M6020*)userData;
	if(!strcmp(topic, m6020->canInfo.canX[0]))
	{
		uint8_t* data = (uint8_t*)frame->data;
		if(m6020->canInfo.id[0] == data[0])
		{
			M6020_Update(m6020, data+1);
		}
	}
}

void M6020_Init(Motor* motor, ConfItem* dict)
{
	M6020* m6020 = (M6020*)motor;
	
	motor->ctrlerCalc = M6020_CtrlerCalc;
	motor->changeCtrler = M6020_ChangeCtrler;
	motor->startStatAngle = M6020_StartStatAngle;
	motor->statAngle = M6020_StatAngle;
	
	m6020->reductionRatio = 1;
	uint16_t id = Conf_GetValue(dict, "id", uint16_t, 0);
	m6020->canInfo.id[0] = id+0x200;
	m6020->canInfo.id[1] = (m6020->canInfo.id[0]<0x205)?0x1FF:0x2FF;
	id = (id-1)%4;
	m6020->canInfo.sendBits =  1<<(2*id-2) | 1<<(2*id-1);
	char* canX = Conf_GetPtr(dict, "canX", char);
	char* send = (char*)MOTOR_MALLOC_PORT((MOTOR_STRLEN_PORT(canX)+4+1)*sizeof(char));
	char* receive = (char*)MOTOR_MALLOC_PORT((MOTOR_STRLEN_PORT(canX)+7+1)*sizeof(char));
	MOTOR_STRCAT_PORT(send, "Send");
	MOTOR_STRCAT_PORT(receive, "Receive");
	m6020->canInfo.canX[0] = receive;
	m6020->canInfo.canX[1] = send;
	m6020->ctrler = torque;
	M6020_PIDInit(m6020, dict);
	SoftBus_Subscribe(m6020, m6020DataCallback, m6020->canInfo.canX[0]);
}

void M6020_PIDInit(M6020* m6020, ConfItem* dict)
{
	float speedP, speedI, speedD, speedMaxI, speedMaxOut;
	speedP = Conf_GetValue(dict, "speedPID/p", uint16_t, 0);
	speedI = Conf_GetValue(dict, "speedPID/i", uint16_t, 0);
	speedD = Conf_GetValue(dict, "speedPID/d", uint16_t, 0);
	speedMaxI = Conf_GetValue(dict, "speedPID/maxI", uint16_t, 0);
	speedMaxOut = Conf_GetValue(dict, "speedPID/maxOut", uint16_t, 0);
	PID_Init(&m6020->speedPID, speedP, speedI, speedD, speedMaxI, speedMaxOut);
	float angleInnerP, angleInnerI, angleInnerD, angleInnerMaxI, angleInnerMaxOut;
	angleInnerP = Conf_GetValue(dict, "anglePID/inner/p", uint16_t, 0);
	angleInnerI = Conf_GetValue(dict, "anglePID/inner/i", uint16_t, 0);
	angleInnerD = Conf_GetValue(dict, "anglePID/inner/d", uint16_t, 0);
	angleInnerMaxI = Conf_GetValue(dict, "anglePID/inner/maxI", uint16_t, 0);
	angleInnerMaxOut = Conf_GetValue(dict, "anglePID/inner/maxOut", uint16_t, 0);
	PID_Init(&m6020->anglePID.inner, angleInnerP, angleInnerI, angleInnerD, angleInnerMaxI, angleInnerMaxOut);
	float angleOuterP, angleOuterI, angleOuterD, angleOuterMaxI, angleOuterMaxOut;
	angleOuterP = Conf_GetValue(dict, "anglePID/outer/p", uint16_t, 0);
	angleOuterI = Conf_GetValue(dict, "anglePID/outer/i", uint16_t, 0);
	angleOuterD = Conf_GetValue(dict, "anglePID/outer/d", uint16_t, 0);
	angleOuterMaxI = Conf_GetValue(dict, "anglePID/outer/maxI", uint16_t, 0);
	angleOuterMaxOut = Conf_GetValue(dict, "anglePID/outer/maxOut", uint16_t, 0);
	PID_Init(&m6020->anglePID.outer, angleOuterP, angleOuterI, angleOuterD, angleOuterMaxI, angleOuterMaxOut*m6020->reductionRatio);
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

void M6020_CtrlerCalc(Motor* motor, float reference)
{
	M6020* m6020 = (M6020*)motor;
	int16_t output;
	if(m6020->ctrler == speed)
	{
		PID_SingleCalc(&m6020->speedPID, reference*m6020->reductionRatio, m6020->speed);
		output = m6020->speedPID.output;
	}
	else if(m6020->ctrler == angle)
	{
		PID_CascadeCalc(&m6020->anglePID, reference, m6020->totalAngle, m6020->speed);
		output = m6020->anglePID.output;
	}
	else if(m6020->ctrler == torque)
	{
		output = (int16_t)reference;
	}
	SoftBus_PublishMap(m6020->canInfo.canX[1],{{"id", &m6020->canInfo.id[1], sizeof(uint32_t)},
																						 {"bits", &m6020->canInfo.sendBits, sizeof(uint8_t)},
																						 {"data", &output, sizeof(int16_t)}});
}

void M6020_ChangeCtrler(Motor* motor, Ctrler ctrler)
{
	M6020* m6020 = (M6020*)motor;
	if(m6020->ctrler == speed)
	{
		PID_Clear(&m6020->speedPID);
	}
	else if(m6020->ctrler == angle)
	{
		PID_Clear(&m6020->anglePID.inner);
		PID_Clear(&m6020->anglePID.outer);
	}
	m6020->ctrler = ctrler;
}

//更新电机数据(可能进行滤波)
void M6020_Update(M6020* m6020,uint8_t* data)
{
	m6020->angle = (data[0]<<8 | data[1]);
	m6020->speed = (data[2]<<8 | data[3]);
}
