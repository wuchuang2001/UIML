#include "softbus.h"
#include "motor.h"
#include "pid.h"
#include "config.h"

//各种电机编码值与角度的换算
#define M3508_DGR2CODE(dgr,rdcr) ((int32_t)((dgr)*22.7528f*(rdcr))) //减速比*8191/360
#define M3508_CODE2DGR(code,rdcr) ((float)((code)/(22.7528f*(rdcr))))

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
	
	float  targetValue;//目标值(输出轴扭矩矩/速度/角度(单位度))
	
	PID speedPID;//速度pid(单级)
	CascadePID anglePID;//角度pid，串级
	
}M3508;

Motor* M3508_Init(ConfItem* dict);

void M3508_SetStartAngle(Motor *motor, float startAngle);
void M3508_SetTarget(Motor* motor, float targetValue);
void M3508_ChangeMode(Motor* motor, MotorCtrlMode mode);
void M3508_SoftBusCallback(const char* topic, SoftBusFrame* frame, void* bindData);

void M3508_Update(M3508* m3508,uint8_t* data);
void M3508_PIDInit(M3508* m3508, ConfItem* dict);
void M3508_StatAngle(M3508* m3508);
void M3508_CtrlerCalc(M3508* m3508, float reference);

//软件定时器回调函数
void M3508_TimerCallback(void const *argument)
{
	M3508* m3508 = pvTimerGetTimerID((TimerHandle_t)argument); 
	M3508_StatAngle(m3508);
	M3508_CtrlerCalc(m3508, m3508->targetValue);
}

Motor* M3508_Init(ConfItem* dict)
{
	//分配子类内存空间
	M3508* m3508 = MOTOR_MALLOC_PORT(sizeof(M3508));
	memset(m3508,0,sizeof(M3508));
	//子类多态
	m3508->motor.setTarget = M3508_SetTarget;
	m3508->motor.changeMode = M3508_ChangeMode;
	m3508->motor.setStartAngle = M3508_SetStartAngle;
	//电机减速比
	m3508->reductionRatio = Conf_GetValue(dict, "reductionRatio", float, 19.2f);//如果未配置电机减速比参数，则使用原装电机默认减速比
	//初始化电机绑定can信息
	uint16_t id = Conf_GetValue(dict, "id", uint16_t, 0);
	m3508->canInfo.recvID = id + 0x200;
	m3508->canInfo.sendID = (id <= 4) ? 0x200 : 0x1FF;
	m3508->canInfo.bufIndex =  ((id - 1)%4) * 2;
	m3508->canInfo.canX = Conf_GetValue(dict, "canX", uint8_t, 0);
	//设置电机默认模式为扭矩模式
	m3508->mode = MOTOR_TORQUE_MODE;
	//初始化电机pid
	M3508_PIDInit(m3508, dict);
	//订阅can信息
	char topic[] = "/can_/recv";
	topic[4] = m3508->canInfo.canX + '0';
	SoftBus_Subscribe(m3508, M3508_SoftBusCallback, topic);
	//开启软件定时器
	osTimerDef(M3508, M3508_TimerCallback);
	osTimerStart(osTimerCreate(osTimer(M3508), osTimerPeriodic, m3508), 2);

	return (Motor*)m3508;
}
//初始化pid
void M3508_PIDInit(M3508* m3508, ConfItem* dict)
{
	PID_Init(&m3508->speedPID, Conf_GetPtr(dict, "speedPID", ConfItem));
	PID_Init(&m3508->anglePID.inner, Conf_GetPtr(dict, "anglePID/inner", ConfItem));
	PID_Init(&m3508->anglePID.outer, Conf_GetPtr(dict, "anglePID/outer", ConfItem));
	PID_SetMaxOutput(&m3508->anglePID.outer, m3508->anglePID.outer.maxOutput*m3508->reductionRatio);//将输出轴速度限幅放大到转子上
}
//软总线回调函数
void M3508_SoftBusCallback(const char* topic, SoftBusFrame* frame, void* bindData)
{
	M3508* m3508 = (M3508*)bindData;

	uint16_t id = *(uint16_t*)SoftBus_GetListValue(frame, 0);
	if(id != m3508->canInfo.recvID)
		return;
		
	uint8_t* data = (uint8_t*)SoftBus_GetListValue(frame, 1);
	if(data)
		M3508_Update(m3508, data);
}

//开始统计电机累计角度
void M3508_SetStartAngle(Motor *motor, float startAngle)
{
	M3508* m3508 = (M3508*)motor;
	
	m3508->totalAngle=M3508_DGR2CODE(startAngle, m3508->reductionRatio);
	m3508->lastAngle=m3508->angle;
}

//统计电机累计转过的圈数
void M3508_StatAngle(M3508* m3508)
{
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
//控制器根据模式计算输出
void M3508_CtrlerCalc(M3508* m3508, float reference)
{
	int16_t output=0;
	uint8_t buffer[2]={0};
	if(m3508->mode == MOTOR_SPEED_MODE)
	{
		PID_SingleCalc(&m3508->speedPID, reference, m3508->speed);
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
	buffer[0] = (output>>8)&0xff;
	buffer[1] = (output)&0xff;
	SoftBus_Publish("/can/set-buf",{
		{"can-x", &m3508->canInfo.canX},
		{"id", &m3508->canInfo.sendID},
		{"pos", &m3508->canInfo.bufIndex},
		{"len", &(uint8_t){2}},
		{"data", buffer}
	});
}
//设置电机期望值
void M3508_SetTarget(Motor* motor, float targetValue)
{
	M3508* m3508 = (M3508*)motor;
	if(m3508->mode == MOTOR_ANGLE_MODE)
	{
		m3508->targetValue = M3508_DGR2CODE(targetValue, m3508->reductionRatio);
	}
	else if(m3508->mode == MOTOR_SPEED_MODE)
	{
		m3508->targetValue = targetValue*m3508->reductionRatio;
	}
	else
	{
		m3508->targetValue = targetValue;
	}
}
//切换电机模式
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
