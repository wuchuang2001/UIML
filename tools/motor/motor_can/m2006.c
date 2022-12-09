#include "softbus.h"
#include "motor.h"
#include "pid.h"
#include "config.h"

//各种电机编码值与角度的换算	
#define M2006_DGR2CODE(dgr,rdcr) ((int32_t)((dgr)*22.7528f*(rdcr))) //减速比*8191/360
#define M2006_CODE2DGR(code,rdcr) ((float)((code)/(22.7528f*(rdcr))))

typedef struct _M2006
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
	
}M2006;

Motor* M2006_Init(ConfItem* dict);

void M2006_SetStartAngle(Motor *motor, float startAngle);
void M2006_SetTarget(Motor* motor, float targetValue);
void M2006_ChangeMode(Motor* motor, MotorCtrlMode mode);
void M2006_SoftBusCallback(const char* name, SoftBusFrame* frame, void* bindData);

void M2006_Update(M2006* m2006,uint8_t* data);
void M2006_PIDInit(M2006* m2006, ConfItem* dict);
void M2006_StatAngle(M2006* m2006);
void M2006_CtrlerCalc(M2006* m2006, float reference);

//软件定时器回调函数
void M2006_TimerCallback(void const *argument)
{
	M2006* m2006 = pvTimerGetTimerID((TimerHandle_t)argument);
	M2006_StatAngle(m2006);
	M2006_CtrlerCalc(m2006, m2006->targetValue);
}

Motor* M2006_Init(ConfItem* dict)
{
	//分配子类内存空间
	M2006* m2006 = MOTOR_MALLOC_PORT(sizeof(M2006));
	memset(m2006,0,sizeof(M2006));
	//子类多态
	m2006->motor.setTarget = M2006_SetTarget;
	m2006->motor.changeMode = M2006_ChangeMode;
	m2006->motor.setStartAngle = M2006_SetStartAngle;
	//电机减速比
	m2006->reductionRatio = Conf_GetValue(dict, "reductionRatio", float, 36);//如果未配置电机减速比参数，则使用原装电机默认减速比
	//初始化电机绑定can信息
	uint16_t id = Conf_GetValue(dict, "id", uint16_t, 0);
	m2006->canInfo.recvID = id + 0x200;
	m2006->canInfo.sendID = (id <= 4) ? 0x200 : 0x1FF;
	m2006->canInfo.bufIndex =  ((id - 1)%4) * 2;
	m2006->canInfo.canX = Conf_GetValue(dict, "canX", uint8_t, 0);
	//设置电机默认模式为扭矩模式
	m2006->mode = MOTOR_TORQUE_MODE;
	//初始化电机pid
	M2006_PIDInit(m2006, dict);
	//订阅can信息
	char name[] = "/can_/recv";
	name[4] = m2006->canInfo.canX + '0';
	Bus_RegisterReceiver(m2006, M2006_SoftBusCallback, name);
	//开启软件定时器
	osTimerDef(M2006, M2006_TimerCallback);
	osTimerStart(osTimerCreate(osTimer(M2006), osTimerPeriodic, m2006), 2);

	return (Motor*)m2006;
}
//初始化pid
void M2006_PIDInit(M2006* m2006, ConfItem* dict)
{
	PID_Init(&m2006->speedPID, Conf_GetPtr(dict, "speedPID", ConfItem));
	PID_Init(&m2006->anglePID.inner, Conf_GetPtr(dict, "anglePID/inner", ConfItem));
	PID_Init(&m2006->anglePID.outer, Conf_GetPtr(dict, "anglePID/outer", ConfItem));
	PID_SetMaxOutput(&m2006->anglePID.outer, m2006->anglePID.outer.maxOutput*m2006->reductionRatio);//将输出轴速度限幅放大到转子上
}
//软总线回调函数
void M2006_SoftBusCallback(const char* name, SoftBusFrame* frame, void* bindData)
{
	M2006* m2006 = (M2006*)bindData;

	uint16_t id = *(uint16_t*)Bus_GetListValue(frame, 0);
	if(id != m2006->canInfo.recvID)
		return;
		
	uint8_t* data = (uint8_t*)Bus_GetListValue(frame, 1);
	if(data)
		M2006_Update(m2006, data);
}

//开始统计电机累计角度
void M2006_SetStartAngle(Motor *motor, float startAngle)
{
	M2006* m2006 = (M2006*)motor;
	
	m2006->totalAngle= M2006_DGR2CODE(startAngle, m2006->reductionRatio);
	m2006->lastAngle=m2006->angle;
}

//统计电机累计转过的圈数
void M2006_StatAngle(M2006* m2006)
{
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
//控制器根据模式计算输出
void M2006_CtrlerCalc(M2006* m2006, float reference)
{
	int16_t output=0;
	uint8_t buffer[2]={0};
	if(m2006->mode == MOTOR_SPEED_MODE)
	{
		PID_SingleCalc(&m2006->speedPID, reference, m2006->speed);
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
	buffer[0] = (output>>8)&0xff;
	buffer[1] = (output)&0xff;
	//发布can信息
	Bus_BroadcastSend("/can/set-buf",{
		{"can-x", &m2006->canInfo.canX},
		{"id", &m2006->canInfo.sendID},
		{"pos", &m2006->canInfo.bufIndex},
		{"len", &(uint8_t){2}},
		{"data", buffer}
	});
}
//设置电机期望值
void M2006_SetTarget(Motor* motor, float targetValue)
{
	M2006* m2006 = (M2006*)motor;
	if(m2006->mode == MOTOR_ANGLE_MODE)
	{
		m2006->targetValue = M2006_DGR2CODE(targetValue, m2006->reductionRatio);
	}
	else if(m2006->mode == MOTOR_SPEED_MODE)
	{
		m2006->targetValue = targetValue*m2006->reductionRatio;
	}
	else
	{
		m2006->targetValue = targetValue;
	}
}
//切换电机模式
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
