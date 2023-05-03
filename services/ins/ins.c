#include "bmi088_driver.h"
#include "cmsis_os.h"
#include "softbus.h"
#include "config.h"
#include "AHRS.h"
#include "pid.h"
#include "filter.h"

typedef struct 
{
	struct 
	{
		float quat[4];
		float accel[3];
		float gyro[3];
		float mag[3];
		float gyroOffset[3];
		float tmp;
	}imu;
	uint8_t spiX;
	float yaw,pitch,roll;
	float targetTmp;
	uint8_t timX;
	uint8_t channelX;

	PID tmpPID;
	Filter *filter;

	uint16_t taskInterval; //任务执行间隔

	char* eulerAngleName;
}INS;

void INS_Init(INS* ins, ConfItem* dict);
void INS_TmpPIDTimerCallback(void const *argument);

void INS_TaskCallback(void const * argument)
{ 

	/* USER CODE BEGIN IMU */ 
	INS ins = {0};
	osDelay(50);
	INS_Init(&ins, (ConfItem*)argument);
	AHRS_init(ins.imu.quat,ins.imu.accel,ins.imu.mag);
	//校准零偏
	// for(int i=0;i<10000;i++)
	// {
	// 	BMI088_ReadData(ins.spiX, ins.imu.gyro,ins.imu.accel, &ins.imu.tmp);
	// 	ins.imu.gyroOffset[0] +=ins.imu.gyro[0];
	// 	ins.imu.gyroOffset[1] +=ins.imu.gyro[1];
	// 	ins.imu.gyroOffset[2] +=ins.imu.gyro[2];
	// 	HAL_Delay(1);
	// }
	// ins.imu.gyroOffset[0] = ins.imu.gyroOffset[0]/10000.0f;
	// ins.imu.gyroOffset[1] = ins.imu.gyroOffset[1]/10000.0f;
	// ins.imu.gyroOffset[2] = ins.imu.gyroOffset[2]/10000.0f;

	ins.imu.gyroOffset[0] = -0.000767869;   //10次校准取均值
	ins.imu.gyroOffset[1] = 0.000771033;  
	ins.imu.gyroOffset[2] = 0.001439746;
	
  /* Infinite loop */
	while(1)
	{
		BMI088_ReadData(ins.spiX, ins.imu.gyro,ins.imu.accel, &ins.imu.tmp);
		for(uint8_t i=0;i<3;i++)
			ins.imu.gyro[i] -= ins.imu.gyroOffset[i];

		//滤波
		// for(uint8_t i=0;i<3;i++)
		// 	ins.imu.accel[i] = ins.filter->cala(ins.filter , ins.imu.accel[i]);
		//数据融合	
		AHRS_update(ins.imu.quat,ins.taskInterval/1000.0f,ins.imu.gyro,ins.imu.accel,ins.imu.mag);
		get_angle(ins.imu.quat,&ins.yaw,&ins.pitch,&ins.roll);
		ins.yaw = ins.yaw/PI*180;
		ins.pitch = ins.pitch/PI*180;
		ins.roll = ins.roll/PI*180;
		//发布数据
		Bus_BroadcastSend(ins.eulerAngleName, {{"yaw",&ins.yaw}, {"pitch",&ins.pitch}, {"roll",&ins.roll}});
		osDelay(ins.taskInterval);
	}
  /* USER CODE END IMU */
}

void INS_Init(INS* ins, ConfItem* dict)
{
	ins->spiX = Conf_GetValue(dict, "spi-x", uint8_t, 0);
	ins->targetTmp = Conf_GetValue(dict, "target-temperature", uint8_t, 40);
	ins->timX = Conf_GetValue(dict,"tim-x",uint8_t,10);
	ins->channelX = Conf_GetValue(dict,"channel-x",uint8_t,1);
	ins->taskInterval = Conf_GetValue(dict,"taskInterval",uint16_t,10);

	// ins->filter = Filter_Init(Conf_GetPtr(dict, "filter", ConfItem));
	PID_Init(&ins->tmpPID, Conf_GetPtr(dict, "tmpPID", ConfItem));

	ins->eulerAngleName = Conf_GetPtr(dict,"/ins/euler-angle",char);
	ins->eulerAngleName = ins->eulerAngleName?ins->eulerAngleName:"/ins/euler-angle";

	while(BMI088_AccelInit(ins->spiX) || BMI088_GyroInit(ins->spiX))
	{
		osDelay(10);
	}

	BMI088_ReadData(ins->spiX, ins->imu.gyro,ins->imu.accel, &ins->imu.tmp);

	//创建定时器进行温度pid控制
	osTimerDef(tmp, INS_TmpPIDTimerCallback);
	osTimerStart(osTimerCreate(osTimer(tmp), osTimerPeriodic, ins), 2);
}

//软件定时器回调函数
void INS_TmpPIDTimerCallback(void const *argument)
{
	INS* ins = pvTimerGetTimerID((TimerHandle_t)argument);
	PID_SingleCalc(&ins->tmpPID, ins->targetTmp, ins->imu.tmp);
	ins->tmpPID.output = ins->tmpPID.output > 0? ins->tmpPID.output : 0;
	Bus_RemoteCall("/tim/pwm/set-duty", {{"tim-x", &ins->timX}, {"channel-x", &ins->channelX}, {"duty", &ins->tmpPID.output}});
}
