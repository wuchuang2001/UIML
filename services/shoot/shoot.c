#include "config.h"
#include "softbus.h"
#include "motor.h"
#include "cmsis_os.h"
#include "math.h"

#ifndef ecd_range
#define ecd_range 8191
#endif

//卡单时间 以及反转时间
#define BLOCK_TRIGGER_SPEED         1.0f
#define BLOCK_TIME                  350		
#define REVERSE_TIME                500

typedef struct _Shoot
{

		//3个电机
		Motor* motors[3];
		
		float speed[3];											//摩擦轮速度 	[0]为左摩擦轮 [1]为右摩擦轮 [2]为拨弹轮
		float target_ecd[3];
		float ecd[3];
		float target[3];
	
		struct
		{
				uint16_t block_time;
				uint16_t reverse_time;
					
				float target_set;
				float speed;
		} shoot_control;
	
		uint8_t taskInterval;
	
}Shoot;

void Shoot_Init(Shoot* shoot, ConfItem* dict);
void Shoot_SoftBusCallback(const char* name, SoftBusFrame* frame, void* bindData);
static float Shoot_ecd_zero(uint16_t ecd, uint16_t offset_ecd);
static void Feedsprocket_turn_back(Shoot *shoot);

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
				//过零处理
				shoot.target[0] = Shoot_ecd_zero(shoot.target_ecd[0], shoot.ecd[0]);
				shoot.target[1] = Shoot_ecd_zero(shoot.target_ecd[1], shoot.ecd[1]);
				shoot.target[2] = Shoot_ecd_zero(shoot.target_ecd[2], shoot.ecd[2]);
						
				//堵转处理
				Feedsprocket_turn_back(&shoot);
			
				//电机输出
				shoot.motors[0]->setTarget(shoot.motors[0], shoot.target[0]);
				shoot.motors[1]->setTarget(shoot.motors[1], shoot.target[1]);
				shoot.motors[2]->setTarget(shoot.motors[2], shoot.shoot_control.target_set);
	
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
		shoot->shoot_control.speed = 0.0f;
	
		//摩擦轮电机初始化
		shoot->motors[0] = Motor_Init(Conf_GetPtr(dict, "motorFrictiongear_L", ConfItem));
		shoot->motors[1] = Motor_Init(Conf_GetPtr(dict, "motorFrictiongear_R", ConfItem));
	
		//拨弹轮电机初始化
		shoot->motors[2] = Motor_Init(Conf_GetPtr(dict, "motorFeedsprocket", ConfItem));
	
		for(uint8_t i = 0; i<3; i++)
		{
			shoot->motors[i]->changeMode(shoot->motors[i], MOTOR_ANGLE_MODE);
		}	
	
		Bus_MultiRegisterReceiver(shoot, Shoot_SoftBusCallback, {"/shoot/target_ecd", "/shoot/ecd"});
}
	
void Shoot_SoftBusCallback(const char* name, SoftBusFrame* frame, void* bindData)
{
		Shoot* shoot = (Shoot*)bindData;

		if(!strcmp(name, "/shoot/target_ecd"))
		{
			if(Bus_IsMapKeyExist(frame, "Frictiongear_L_target_ecd"))
				shoot->target_ecd[0] = *(float*)Bus_GetMapValue(frame, "Frictiongear_L_target_ecd");
			if(Bus_IsMapKeyExist(frame, "Frictiongear_R_target_ecd"))
				shoot->target_ecd[1] = *(float*)Bus_GetMapValue(frame, "Frictiongear_R_target_ecd");
			if(Bus_IsMapKeyExist(frame, "Feedsprocket_target_ecd"))
				shoot->target_ecd[2] = *(float*)Bus_GetMapValue(frame, "Feedsprocket_target_ecd");
		}
		
		if(!strcmp(name, "/shoot/ecd"))
		{
			if(Bus_IsMapKeyExist(frame, "Frictiongear_L_ecd"))
				shoot->ecd[0] = *(float*)Bus_GetMapValue(frame, "Frictiongear_L_ecd");
			if(Bus_IsMapKeyExist(frame, "Frictiongear_R_ecd"))
				shoot->ecd[1] = *(float*)Bus_GetMapValue(frame, "Frictiongear_R_ecd");
			if(Bus_IsMapKeyExist(frame, "Feedsprocket_ecd"))
				shoot->ecd[2] = *(float*)Bus_GetMapValue(frame, "Feedsprocket_ecd");
		}

}

static float Shoot_ecd_zero(uint16_t ecd, uint16_t offset_ecd)
{
		int32_t relative_ecd = ecd - offset_ecd;

		if(relative_ecd > (ecd_range/2))
		{
					relative_ecd -= ecd_range;
		}
		if(relative_ecd < - (ecd_range/2))
		{
					relative_ecd += ecd_range;
		}

		return relative_ecd;
}

static void Feedsprocket_turn_back(Shoot *shoot)
{
    if( shoot->shoot_control.block_time < BLOCK_TIME)
    {
        shoot->shoot_control.target_set = 	shoot->target_ecd[2];
    }
    else
    {
        shoot->shoot_control.target_set = - shoot->target_ecd[2];
    }

    if(fabs(shoot->shoot_control.speed) < BLOCK_TRIGGER_SPEED && shoot->shoot_control.block_time < BLOCK_TIME)
    {
        shoot->shoot_control.block_time++;
        shoot->shoot_control.reverse_time = 0;
    }
    else if (shoot->shoot_control.block_time == BLOCK_TIME && shoot->shoot_control.reverse_time < REVERSE_TIME)
    {
        shoot->shoot_control.reverse_time++;
    }
    else
    {
        shoot->shoot_control.block_time = 0;
    }
}
