#include "softbus.h"
#include "motor.h"
#include "pid.h"
#include "config.h"

//各种电机编码值与角度的换算
#define M3508_DGR2CODE(dgr) ((int32_t)(dgr*436.9263f)) //3591/187*8191/360
#define M3508_CODE2DGR(code) ((float)(code/436.9263f))

typedef struct _M3508
{
	Motor motor;
	
	float reductionRatio;
	struct
	{
		uint16_t sendID,recvID;
		uint8_t canX;
		uint8_t bufIndex;
	}canInfo;
	MotorCtrlMode mode;
	
	int16_t angle,speed;
	
	int16_t lastAngle;//记录上一次得到的角度
	
	int32_t totalAngle;//累计转过的编码器值
	
	float  targetValue;//目标值(速度/角度(编码器值))
	
	PID speedPID;//速度pid(单级)
	CascadePID anglePID;//角度pid，串级
	
}M3508;

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

	const SoftBusItem* item = NULL;

	item = SoftBus_GetItem(frame, "can-x");
	if(!item || *(uint8_t*)item->data != m3508->canInfo.canX)
		return;

	item = SoftBus_GetItem(frame, "id");
	if(!item || *(uint16_t*)item->data != m3508->canInfo.recvID)
		return;
		
	item = SoftBus_GetItem(frame, "data");
	if(item)
		M3508_Update(m3508, item->data);
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
	m3508->canInfo.recvID = id + 0x200;
	m3508->canInfo.sendID = (id <= 4) ? 0x200 : 0x1FF;
	m3508->canInfo.bufIndex =  (id - 1) * 2;
	m3508->canInfo.canX = Conf_GetValue(dict, "canX", uint8_t, 0);
	
	m3508->mode = MOTOR_TORQUE_MODE;
	M3508_PIDInit(m3508, dict);
	SoftBus_Subscribe(m3508, M3508_SoftBusCallback, "/can/recv");

	return (Motor*)m3508;
}

void M3508_PIDInit(M3508* m3508, ConfItem* dict)
{
	PID_Init(&m3508->speedPID, Conf_GetPtr(dict, "speedPID", ConfItem));
	PID_Init(&m3508->anglePID.inner, Conf_GetPtr(dict, "anglePID/inner", ConfItem));
	PID_Init(&m3508->anglePID.outer, Conf_GetPtr(dict, "anglePID/outer", ConfItem));
	PID_SetMaxOutput(&m3508->anglePID.outer, m3508->anglePID.outer.maxOutput*m3508->reductionRatio);
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
	SoftBus_PublishMap("/can/set-buf",{
		{"can-x", &m3508->canInfo.canX, sizeof(uint8_t)},
		{"id", &m3508->canInfo.sendID, sizeof(uint16_t)},
		{"pos", &m3508->canInfo.bufIndex, sizeof(uint8_t)},
		{"len", &(uint8_t){2}, sizeof(uint8_t)},
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
