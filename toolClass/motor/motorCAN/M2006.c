#include "M2006.h"
#include "softbus.h"

Motor* M2006_Init(ConfItem* dict);

void M2006_StartStatAngle(Motor *motor);
void M2006_StatAngle(Motor* motor);
void M2006_CtrlerCalc(Motor* motor, float reference);
void M2006_ChangeMode(Motor* motor, MotorCtrlMode mode);

void M2006_Update(M2006* m2006,uint8_t* data);
void M2006_PIDInit(M2006* m2006, ConfItem* dict);

void M2006_SoftBusCallback(const char* topic, SoftBusFrame* frame, void* bindData)
{
	M2006* m2006 = (M2006*)bindData;
	if(!strcmp(topic, m2006->canInfo.canX[0]))
	{
		uint8_t* data = (uint8_t*)frame->data;
		if(m2006->canInfo.id[0] == data[0])
		{
			M2006_Update(m2006, data+1);
		}
	}
}

Motor* M2006_Init(ConfItem* dict)
{
	M2006* m2006 = pvPortMalloc(sizeof(M2006));
	memset(m2006,0,sizeof(M2006));
	
	m2006->motor.ctrlerCalc = M2006_CtrlerCalc;
	m2006->motor.changeMode = M2006_ChangeMode;
	m2006->motor.startStatAngle = M2006_StartStatAngle;
	m2006->motor.statAngle = M2006_StatAngle;
	
	m2006->reductionRatio = 36;
	uint16_t id = Conf_GetValue(dict, "id", uint16_t, 0);
	m2006->canInfo.id[0] = id+0x200;
	m2006->canInfo.id[1] = (m2006->canInfo.id[0]<0x205)?0x200:0x1FF;
	id = (id-1)%4;
	m2006->canInfo.sendBits =  1<<(2*id-2) | 1<<(2*id-1);
	char* canX = Conf_GetPtr(dict, "canX", char);
	char* send = (char*)MOTOR_MALLOC_PORT((MOTOR_STRLEN_PORT(canX)+4+1)*sizeof(char));
	char* receive = (char*)MOTOR_MALLOC_PORT((MOTOR_STRLEN_PORT(canX)+7+1)*sizeof(char));
	MOTOR_STRCAT_PORT(send, "Send");
	MOTOR_STRCAT_PORT(receive, "Receive");
	m2006->canInfo.canX[0] = receive;
	m2006->canInfo.canX[1] = send;
	m2006->mode = MOTOR_TORQUE_MODE;
	M2006_PIDInit(m2006, dict);
	SoftBus_Subscribe(m2006, M2006_SoftBusCallback, m2006->canInfo.canX[0]);

	return (Motor*)m2006;
}

void M2006_PIDInit(M2006* m2006, ConfItem* dict)
{
	float speedP, speedI, speedD, speedMaxI, speedMaxOut;
	speedP = Conf_GetValue(dict, "speedPID/p", uint16_t, 0);
	speedI = Conf_GetValue(dict, "speedPID/i", uint16_t, 0);
	speedD = Conf_GetValue(dict, "speedPID/d", uint16_t, 0);
	speedMaxI = Conf_GetValue(dict, "speedPID/maxI", uint16_t, 0);
	speedMaxOut = Conf_GetValue(dict, "speedPID/maxOut", uint16_t, 0);
	PID_Init(&m2006->speedPID, speedP, speedI, speedD, speedMaxI, speedMaxOut);
	float angleInnerP, angleInnerI, angleInnerD, angleInnerMaxI, angleInnerMaxOut;
	angleInnerP = Conf_GetValue(dict, "anglePID/inner/p", uint16_t, 0);
	angleInnerI = Conf_GetValue(dict, "anglePID/inner/i", uint16_t, 0);
	angleInnerD = Conf_GetValue(dict, "anglePID/inner/d", uint16_t, 0);
	angleInnerMaxI = Conf_GetValue(dict, "anglePID/inner/maxI", uint16_t, 0);
	angleInnerMaxOut = Conf_GetValue(dict, "anglePID/inner/maxOut", uint16_t, 0);
	PID_Init(&m2006->anglePID.inner, angleInnerP, angleInnerI, angleInnerD, angleInnerMaxI, angleInnerMaxOut);
	float angleOuterP, angleOuterI, angleOuterD, angleOuterMaxI, angleOuterMaxOut;
	angleOuterP = Conf_GetValue(dict, "anglePID/outer/p", uint16_t, 0);
	angleOuterI = Conf_GetValue(dict, "anglePID/outer/i", uint16_t, 0);
	angleOuterD = Conf_GetValue(dict, "anglePID/outer/d", uint16_t, 0);
	angleOuterMaxI = Conf_GetValue(dict, "anglePID/outer/maxI", uint16_t, 0);
	angleOuterMaxOut = Conf_GetValue(dict, "anglePID/outer/maxOut", uint16_t, 0);
	PID_Init(&m2006->anglePID.outer, angleOuterP, angleOuterI, angleOuterD, angleOuterMaxI, angleOuterMaxOut*m2006->reductionRatio);
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

void M2006_CtrlerCalc(Motor* motor, float reference)
{
	M2006* m2006 = (M2006*)motor;
	int16_t output;
	if(m2006->mode == MOTOR_SPEED_MODE)
	{
		PID_SingleCalc(&m2006->speedPID, reference*m2006->reductionRatio, m2006->speed);
		output = m2006->speedPID.output;
	}
	else if(m2006->mode == MOTOR_ANGLE_MODE)
	{
		PID_CascadeCalc(&m2006->anglePID, reference, m2006->totalAngle, m2006->speed);
		output = m2006->anglePID.output;
	}
	else if(m2006->mode == MOTOR_TORQUE_MODE)
	{
		output = (int16_t)reference;
	}
	SoftBus_PublishMap(m2006->canInfo.canX[1],{
		{"id", &m2006->canInfo.id[1], sizeof(uint32_t)},
		{"bits", &m2006->canInfo.sendBits, sizeof(uint8_t)},
		{"data", &output, sizeof(int16_t)}
	});
}

void M2006_ChangeMode(Motor* motor, MotorCtrlMode mode)
{
	M2006* m2006 = (M2006*)motor;
	if(m2006->mode == MOTOR_SPEED_MODE)
	{
		PID_Clear(&m2006->speedPID);
	}
	else if(m2006->mode == MOTOR_ANGLE_MODE)
	{
		PID_Clear(&m2006->anglePID.inner);
		PID_Clear(&m2006->anglePID.outer);
	}
	m2006->mode = mode;
}

//更新电机数据(可能进行滤波)
void M2006_Update(M2006* m2006,uint8_t* data)
{
	m2006->angle = (data[0]<<8 | data[1]);
	m2006->speed = (data[2]<<8 | data[3]);
}
