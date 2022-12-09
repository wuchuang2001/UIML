#include "slope.h"
#include "config.h"
#include "softbus.h"
#include "motor.h"
#include "cmsis_os.h"

#define PI 3.1415926535f

#define GIMBAL_TASK_INIT_TIME 201			//任务初始化 空闲一段时间
#define GIMBAL_MODE_CHANNEL 0					//状态拨杆通道
#define GIMBAL_CONTROL_TIME 1					//云台任务间隔时间

//陀螺仪控制结构体
typedef struct
{
		float measure_angle_360;    //测量角度值 单位360 相对角度 电机编码器
		float measure_speed_360;    //测量角速度值 单位360/s 相对角度 电机编码器
		float target_angle_360;			//目标角度值 单位360 相对角度
		float target_speed_360;			//目标角速度值 单位360/s 相对角度
		float error_angle_360;			//误差角度值 单位360 相对角度
		float error_speed_360;			//误差角速度值 单位360/s 相对角度
		
//		//卡尔曼滤波结构体
//		struct
//		{
//			float speed_error_kalman, angle_error_kalman;
//			KalmanFilter kalman_speed, kalman_angle;
//		}filter;
//		
//		PidThreeLayer pid;    		//串级pid结构体
//		
} GimbalGyroControlStruct;

typedef struct _Gimbal
{
		
							//  外部数据接收  //

		float INS_angle[3];
		float Target_angle[2];
		//2个电机
		Motor* motors[2];
		
		struct _Yaw
		{		
				float yaw_velocity;				//当前速度 mm/s			
				float maxV; 							//最大速度
			
				struct
				{
						float angle_middle_8192;    						//yaw在中间时的8192值
						float angle_left_360, angle_right_360;  //yaw左右最大转角
						int16_t measure_relative_angle_8192;    //测量角度值 单位8192 相对角度
						float measure_angle_360;    						//测量角度值 单位360 相对角度
						float measure_speed_360;    						//测量角速度值 单位360/s 相对角度
						int16_t output;    											//电机最终输出值
				} info;
				
//				const motor_measure_t *motor_measure;		//用于接收电机数据地址
				GimbalGyroControlStruct gyro;							//陀螺仪控制结构体				
		}yaw;
		
		struct _Pitch
		{		
				float pitch_velocity;			//当前速度 mm/s			
				float maxV; 							//最大速度
			
				struct
				{
						float angle_middle_8192;    						//pitch在中间时的8192值
						float angle_up_360, angle_down_360;  		//pitch上下最大转角
						int16_t measure_relative_angle_8192;    //测量角度值 单位8192 相对角度
						float measure_angle_360;    						//测量角度值 单位360 相对角度
						float measure_speed_360;    						//测量角速度值 单位360/s 相对角度
						int16_t output;    											//电机最终输出值，直接发送就行
				} info;
				
//				const motor_measure_t *motor_measure;		//用于接收电机数据地址
				GimbalGyroControlStruct gyro;							//陀螺仪控制结构体			
		}pitch;
				
		uint8_t taskInterval;
	
		
}Gimbal;

void Gimbal_Init(Gimbal* gimbal, ConfItem* dict);
void Gimbal_MotoAnalysis(Gimbal *gimbal);
void Gimbal_SoftBusCallback(const char* name, SoftBusFrame* frame, void* bindData);
void Gimbal_MoveBusCallback(const char* name, SoftBusFrame* frame, void* bindData);
float angle_zero(float angle, float offset_angle);

void Gimbal_TaskCallback(void const * argument)
{
	
		//进入临界区
		portENTER_CRITICAL();
		Gimbal gimbal={0};
		Gimbal_Init(&gimbal, (ConfItem*)argument);
		portEXIT_CRITICAL();
		
		osDelay(2000);
		TickType_t tick = xTaskGetTickCount();
	
		while(1)
		{
			
				angle_zero(gimbal.Target_angle[0],gimbal.INS_angle[0]);		//yaw
				angle_zero(gimbal.Target_angle[1],gimbal.INS_angle[1]);		//pitch
			
				osDelayUntil(&tick,gimbal.taskInterval);
		}

}

void Gimbal_Init(Gimbal* gimbal, ConfItem* dict)
{

		//任务间隔
		gimbal->taskInterval = Conf_GetValue(dict, "taskInterval", uint8_t, 2);

		gimbal->motors[0] = Motor_Init(Conf_GetPtr(dict, "motorYaw", ConfItem));
		gimbal->motors[1] = Motor_Init(Conf_GetPtr(dict, "motorPitch", ConfItem));
		
		//pitch初始化数据
		gimbal->pitch.info.angle_middle_8192 = 124.f;
		gimbal->pitch.info.angle_up_360 		 = 10.f;
		gimbal->pitch.info.angle_down_360  	 = -40.f;
	
		//yaw初始化数据
		gimbal->yaw.info.angle_middle_8192 = 6144.f;
		gimbal->yaw.info.angle_left_360 	 = -180.f;
		gimbal->yaw.info.angle_right_360 	 = 180.f;
	
		//移动参数初始化
		gimbal->yaw.maxV 	 = Conf_GetValue(dict, "moveYaw/maxSpeed", float, 2000);
		gimbal->pitch.maxV = Conf_GetValue(dict, "movePitch/maxSpeed", float, 2000);	
	
		//云台电机初始化
		gimbal->motors[0] = Motor_Init(Conf_GetPtr(dict, "motorYaw", ConfItem));
		gimbal->motors[1] = Motor_Init(Conf_GetPtr(dict, "motorPitch", ConfItem));
	
		for(uint8_t i = 0; i<2; i++)
		{
			gimbal->motors[i]->changeMode(gimbal->motors[i], MOTOR_SPEED_MODE);
		}
		
		
		Bus_MultiRegisterReceiver(gimbal, Gimbal_SoftBusCallback, {"/gimbal/INS_angle"});
		Bus_MultiRegisterReceiver(gimbal, Gimbal_MoveBusCallback, {"rc/mouse-move"});		
}

void Gimbal_SoftBusCallback(const char* name, SoftBusFrame* frame, void* bindData)
{
		Gimbal* gimbal = (Gimbal*)bindData;

		if(!strcmp(name, "/gimbal/INS_angle"))
		{
				if(Bus_IsMapKeyExist(frame, "ins_angle[0]"))
					gimbal->INS_angle[0] = *(float*)Bus_GetMapValue(frame, "INS_angle[0]");
				if(Bus_IsMapKeyExist(frame, "ins_angle[0]"))
					gimbal->INS_angle[1] = *(float*)Bus_GetMapValue(frame, "INS_angle[1]");
				if(Bus_IsMapKeyExist(frame, "ins_angle[0]"))
					gimbal->INS_angle[2] = *(float*)Bus_GetMapValue(frame, "INS_angle[2]");				
		}
}

void Gimbal_MoveBusCallback(const char* name, SoftBusFrame* frame, void* bindData)
{	
		Gimbal* gimbal = (Gimbal*)bindData;
	
		if(!strcmp(name, "rc/mouse-move"))
		{
				if(Bus_IsMapKeyExist(frame, "x"))
					gimbal->Target_angle[0] += *(float*)Bus_GetMapValue(frame, "x");
				
				if(Bus_IsMapKeyExist(frame, "y"))
					gimbal->Target_angle[1] += *(float*)Bus_GetMapValue(frame, "y");		
		}
	
}	
	
float angle_zero(float angle, float offset_angle)
{
		float relative_angle = angle - offset_angle;

		if(relative_angle >  1.25f * PI)
		{
					relative_angle -= 2*PI;
		}
		else if(relative_angle < - 1.25f * PI)
		{
					relative_angle += 2*PI;		
		}

		return relative_angle;
}

