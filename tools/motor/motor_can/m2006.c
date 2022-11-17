#include "softbus.h"
#include "motor.h"
#include "pid.h"
#include "config.h"

//各种电机编码值与角度的换算	
#define M2006_DGR2CODE(dgr) ((int32_t)(dgr*819.1f)) //36*8191/360
#define M2006_CODE2DGR(code) ((float)(code/819.1f))

typedef struct _M2006
{
	Motor motor;
	
	float reductionRatio;
	struct
	{
		uint16_t id[2];
		char* canX;
		uint8_t sendBits;
	}canInfo;
	MotorCtrlMode mode;
	
	int16_t angle,speed;
	
	int16_t lastAngle;//记录上一次得到的角度
	
	int32_t totalAngle;//累计转过的编码器值
	
	float  targetValue;//目标值(速度/角度(编码器值))
	
	PID speedPID;//速度pid(单级)
	CascadePID anglePID;//角度pid，串级
	
}M2006;

Motor* M2006_Init(ConfItem* dict);

void M2006_StartStatAngle(Motor *motor);
void M2006_StatAngle(Motor* motor);
void M2006_SetTarget(Motor* motor, float targetValue);
void M2006_ChangeMode(Motor* motor, MotorCtrlMode mode);

void M2006_Update(M2006* m2006,uint8_t* data);
void M2006_PIDInit(M2006* m2006, ConfItem* dict);
void M2006_CtrlerCalc(M2006* m2006, float reference);

void M2006_SoftBusCallback(const char* topic, SoftBusFrame* frame, void* bindData)
{
	M2006* m2006 = (M2006*)bindData;
	const SoftBusItem* item = SoftBus_GetItem(frame, m2006->canInfo.canX);
	if(item)
	{
		uint8_t* data = (uint8_t*)item->data;
		if(m2006->canInfo.id[0] == *(uint16_t*)data)
		{
			M2006_Update(m2006, data+2);
		}
	}
}

void M2006_TimerCallback(void const *argument)
{
	M2006* m2006 = pvTimerGetTimerID((TimerHandle_t)argument); 
	M2006_CtrlerCalc(m2006, m2006->targetValue);
}

Motor* M2006_Init(ConfItem* dict)
{
	M2006* m2006 = MOTOR_MALLOC_PORT(sizeof(M2006));
	memset(m2006,0,sizeof(M2006));
	
	m2006->motor.setTarget = M2006_SetTarget;
	m2006->motor.changeMode = M2006_ChangeMode;
	m2006->motor.startStatAngle = M2006_StartStatAngle;
	m2006->motor.statAngle = M2006_StatAngle;
	
	m2006->reductionRatio = 36;
	
	uint16_t id = Conf_GetValue(dict, "id", uint16_t, 0);
	m2006->canInfo.id[0] = id+0x200;
	m2006->canInfo.id[1] = (m2006->canInfo.id[0]<0x205)?0x200:0x1FF;
	id = (id-1)%4;
	m2006->canInfo.sendBits =  1<<(2*id-2) | 1<<(2*id-1);
	m2006->canInfo.canX = Conf_GetPtr(dict, "canX", char);
	
	m2006->mode = MOTOR_TORQUE_MODE;
	M2006_PIDInit(m2006, dict);
	SoftBus_Subscribe(m2006, M2006_SoftBusCallback, "canReceive");
	
	osTimerDef(M2006, M2006_TimerCallback);
	osTimerStart(osTimerCreate(osTimer(M2006), osTimerPeriodic, m2006), 2);

	return (Motor*)m2006;
}

void M2006_PIDInit(M2006* m2006, ConfItem* dict)
{
	PID_Init(&m2006->speedPID, Conf_GetPtr(dict, "speedPID", ConfItem));
	PID_Init(&m2006->anglePID.inner, Conf_GetPtr(dict, "anglePID/inner", ConfItem));
	PID_Init(&m2006->anglePID.outer, Conf_GetPtr(dict, "anglePID/outer", ConfItem));
	PID_SetMaxOutput(&m2006->anglePID.outer, m2006->anglePID.outer.maxOutput*m2006->reductionRatio);
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

void M2006_CtrlerCalc(M2006* m2006, float reference)
{
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
	SoftBus_PublishMap("canSend",{
		{"canX", m2006->canInfo.canX, 5},
		{"id", &m2006->canInfo.id[1], sizeof(uint32_t)},
		{"bits", &m2006->canInfo.sendBits, sizeof(uint8_t)},
		{"data", &output, sizeof(int16_t)}
	});
}

void M2006_SetTarget(Motor* motor, float targetValue)
{
	M2006* m2006 = (M2006*)motor;
	if(m2006->mode == MOTOR_ANGLE_MODE)
	{
		m2006->targetValue = M2006_DGR2CODE(targetValue);
	}
	else 
	{
		m2006->targetValue = targetValue;
	}
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
