#include "config.h"
#include "softbus.h"
#include "motor.h"
#include "cmsis_os.h"

typedef struct _Shoot
{

		//3个电机
		Motor* motors[3];
		
		float speed[3];											//摩擦轮速度 	[0]为左摩擦轮 [1]为右摩擦轮 [2]为拨弹轮

		uint8_t taskInterval;
	
}Shoot;

void Shoot_Init(Shoot* shoot, ConfItem* dict);
void Shoot_SoftBusCallback(const char* name, SoftBusFrame* frame, void* bindData);

void Shoot_TaskCallback(void const * argument)
{
		//进入临界区
		portENTER_CRITICAL();
		Shoot shoot={0};
		Shoot_Init(&shoot, (ConfItem*)argument);
		portEXIT_CRITICAL();

		osDelay(2000);
		TickType_t tick = xTaskGetTickCount();	
	
		while(1)
		{	
	
				for(uint8_t i = 0; i<3; i++)
				{
					shoot.motors[i]->setTarget(shoot.motors[i], shoot.speed[i]);
				}
	
				osDelayUntil(&tick,shoot.taskInterval);
		}
	
}
	
void Shoot_Init(Shoot* shoot, ConfItem* dict)
{	
	
		//任务间隔
		shoot->taskInterval = Conf_GetValue(dict, "taskInterval", uint8_t, 2);
	
		//摩擦轮速度初始化
		shoot->speed[0] = Conf_GetValue(dict, "Frictiongear_L/velocity", float, 2000);
		shoot->speed[1] = Conf_GetValue(dict, "Frictiongear_R/velocity", float, 2000);
	
		//拨弹轮速度初始化
		shoot->speed[2] = Conf_GetValue(dict, "Feedsprocket/velocity", float, 2);
	
		//摩擦轮电机初始化
		shoot->motors[0] = Motor_Init(Conf_GetPtr(dict, "motorFrictiongear_L", ConfItem));
		shoot->motors[1] = Motor_Init(Conf_GetPtr(dict, "motorFrictiongear_R", ConfItem));
	
		//拨弹轮电机初始化
		shoot->motors[2] = Motor_Init(Conf_GetPtr(dict, "motorFeedsprocket", ConfItem));
	
		for(uint8_t i = 0; i<3; i++)
		{
			shoot->motors[i]->changeMode(shoot->motors[i], MOTOR_SPEED_MODE);
		}	
	
		Bus_MultiRegisterReceiver(shoot, Shoot_SoftBusCallback, {"/shoot/velocity"});
}
	
void Shoot_SoftBusCallback(const char* name, SoftBusFrame* frame, void* bindData)
{
		Shoot* shoot = (Shoot*)bindData;

		if(!strcmp(name, "/shoot/velocity"))
		{
			if(Bus_IsMapKeyExist(frame, "Frictiongear_Lv"))
				shoot->speed[0] = *(float*)Bus_GetMapValue(frame, "Frictiongear_Lv");
			if(Bus_IsMapKeyExist(frame, "Frictiongear_Rv"))
				shoot->speed[1] = *(float*)Bus_GetMapValue(frame, "Frictiongear_Rv");
			if(Bus_IsMapKeyExist(frame, "Feedsprocket_v"))
				shoot->speed[2] = *(float*)Bus_GetMapValue(frame, "Feedsprocket_v");
		}

}
