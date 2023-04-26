#include "bmi088_driver.h"
#include "BMI088reg.h"
#include "softbus.h"
#include "config.h"
#include "FreeRTOS.h"
#include "cmsis_os.h"
#include "arm_math.h"
#include "AHRS.h"
// #define ACCEL_BUFFERSIZE 8
// #define GYRO_BUFFERSIZE 9
// #define TEMP_BUFFERSIZE 4
// typedef enum
// {
// 	empty,    
// 	accel,    
// 	gyro,    
// 	temp,    
// } WhoUsingSPI;

// typedef  struct
// {
//         float qw;
//         float qx;
//         float qy;
//         float qz;
// }Quaternion;


// typedef struct
// {
// 	float x;
// 	float y;
// 	float z;
// }vector3_float;


// typedef struct
// {
// 	double imuSpeedRad[3];
// 	double imuAngleRad[3];
// 	double imuSpeed360[3];
// 	double imuAngle360[3];
// 	double imuAngle360_last[3];	
//     vector3_float delta_angle;
//     Quaternion quat;
// 	float accel[3];
// 	float accel_sum;
// 	float accel_quad_sum;
// 	float accel_offset[3];
//     float gyro_offset[3];
// 	float temperature;
	
// 	uint8_t accel_update_ready_flag;
// 	uint8_t gyro_update_ready_flag;
// 	uint8_t temp_update_ready_flag;
	
// 	WhoUsingSPI spi_user;

// 	uint8_t accel_rx[ACCEL_BUFFERSIZE],accel_tx[ACCEL_BUFFERSIZE];
// 	uint8_t gyro_rx[GYRO_BUFFERSIZE],gyro_tx[GYRO_BUFFERSIZE];
// 	uint8_t temp_rx[TEMP_BUFFERSIZE],temp_tx[TEMP_BUFFERSIZE];
// } IMU_Bmi088;
typedef struct 
{
	struct 
	{
    float quat[4];
		float accel[3];
		float gyro[3];
		float mag[3];
		float tmp;
	}imu;
	uint8_t spiX;
	float yaw,pitch,roll;
  uint16_t taskInterval; //任务执行间隔
}INS;


// void IMU_Bmi088_Init(IMU_Bmi088 *IMU);
// void IMU_Bmi088_Update(IMU_Bmi088 *IMU);
// uint8_t BMI088_init(IMU_Bmi088 *IMU);
// float Quaternion_getPitch( Quaternion *quat );
// float Quaternion_getRoll( Quaternion *quat );
// float Quaternion_getYaw( Quaternion *quat );    

// float Calculate_AngleReset(float number, float min, float max)
// {
// 	while(number > max)
// 	{
// 		number -= max - min;
// 	}
// 	while(number < min)
// 	{
// 		number += max - min;
// 	}
// 	return number;
// }
  INS ins = {0};
// IMU_Bmi088 IMU={0};
// float BMI088_time;
void INS_Init(INS* ins, ConfItem* dict);

void INS_TaskCallback(void const * argument)
{
  /* USER CODE BEGIN IMU */
	osDelay(100);
  INS_Init(&ins, (ConfItem*)argument);
	AHRS_init(ins.imu.quat,ins.imu.accel,ins.imu.mag);
  /* Infinite loop */
  while(1)
  {
    BMI088_ReadData(ins.spiX, ins.imu.accel, ins.imu.gyro, &ins.imu.tmp);
    //数据融合	
		AHRS_update(ins.imu.quat,ins.taskInterval/1000.0f,ins.imu.gyro,ins.imu.accel,ins.imu.mag);
		get_angle(ins.imu.quat,&ins.yaw,&ins.pitch,&ins.roll);
    //发布数据
    Bus_BroadcastSend("/imu/euler-angle",{{"yaw",&ins.yaw},{"pitch",&ins.pitch},{"roll",&ins.roll}});
    osDelay(ins.taskInterval);
  }
  /* USER CODE END IMU */
}

void INS_Init(INS* ins, ConfItem* dict)
{
  ins->spiX = Conf_GetValue(dict, "spi-x", uint8_t, 0);
  ins->taskInterval = Conf_GetValue(dict,"taskInterval",uint16_t,10);
	while(BMI088_AccelInit(ins->spiX) || BMI088_GyroInit(ins->spiX))
	{
		osDelay(10);
	}

	BMI088_ReadData(ins->spiX, ins->imu.accel, ins->imu.gyro, &ins->imu.tmp);

//	//创建定时器进行温度pid控制
//	？


}

// uint8_t BMI088_init(IMU_Bmi088 *IMU)
// {
//     uint8_t error = BMI088_NO_ERROR;
//     // GPIO and SPI  Init .
//     BMI088_GPIO_init();
//     BMI088_com_init();
//     error |= bmi088_accel_init();
//     error |= bmi088_gyro_init();
// //	static fp32 gyro[3],temp,gyro_1[3];
// //		//校准零偏
// //		for(int i=0;i<10000;i++)
// //		{
// //			BMI088_read(gyro,gyro_1,&temp);
// //			offset[0] +=gyro[0];
// //			offset[1] +=gyro[1];
// //			offset[2] +=gyro[2];
// //			HAL_Delay(1);
// //		}
// //		offset[0] = offset[0]/10000.0f;
// //		offset[1] = offset[1]/10000.0f;
// //		offset[2] = offset[2]/10000.0f;
// //      /*2022_4_30 s*/
// //      offset[0]=0.005070529;//10000.f,9 times 
// //		  offset[1]=0.002304478;
// //	    offset[2]=0.007660185;
// //      /*2022_5_15 s*/
// //      offset[0]=0.0052387896;//10000.f,6 times 
// //		  offset[1]=0.00262732455;
// ////	    offset[2]=0.00762535026;
// //	    offset[0]=0.0052387896;//10000.f,6 times 
// //		  offset[1]=0.00208825921;
// //	    offset[2]=0.00762535026;
//    /*2022_5_30 s*/
// //	 offset[0]=0.005242626;
// //   offset[1]=0.00241899;
// //   offset[2]=0.007580573;
// /*以上为老车*/
// /*以下为新车*/
//    /*2022_6_6 s*/
//    IMU->gyro_offset[0]=-0.001690499;
//    IMU->gyro_offset[1]=-0.000245874;
//    IMU->gyro_offset[2]=-0.000813253;
//    return error;
// }
// void IMU_Bmi088_Init(IMU_Bmi088 *IMU)
// {
//     while(BMI088_init(IMU)){};
// 	HAL_TIM_PWM_Start(&htim10,TIM_CHANNEL_1);
//     IMU->accel_tx[0]= BMI088_ACCEL_XOUT_L | 0x80;
//     IMU->accel_tx[1]= BMI088_ACCEL_XOUT_L | 0x80;
// 	for(uint8_t i=2;i<ACCEL_BUFFERSIZE;i++)
// 	{
// 		IMU->accel_tx[i] = 0x55;
// 	}
// 	IMU->gyro_tx[0] = BMI088_GYRO_CHIP_ID | 0x80;
// 	for(uint8_t i=1;i<GYRO_BUFFERSIZE;i++)
// 	{
// 		IMU->gyro_tx[i] = 0x55;//表示已经校准
// 	}
// 	IMU->temp_tx[0] = BMI088_TEMP_M | 0x80;
// 	IMU->temp_tx[1] = BMI088_TEMP_M | 0x80;
// 	for(uint8_t i=2;i<TEMP_BUFFERSIZE;i++)
// 	{
// 		IMU->temp_tx[i] = 0x55;
// 	}
	
//     IMU->quat.qw=1;
//     IMU->quat.qx=0;
//     IMU->quat.qy=0;
//     IMU->quat.qz=0;
// }

// void Acc_Compensation(IMU_Bmi088 *IMU)
// {
// 		float q0q0;
// 		float q1q1,q2q2,q3q3;
// 	    q0q0 = IMU->quat.qw*IMU->quat.qw;
// 		q1q1 = IMU->quat.qx*IMU->quat.qx;
// 	  q2q2 = IMU->quat.qy*IMU->quat.qy;
// 	  q3q3 = IMU->quat.qz*IMU->quat.qz;
// 	  float s0,s1,s2,s3;
// 		float _4q0,_2q1,_4q1,_2q3,_2q2,_2q0,_4q2,_8q2,_8q1;
// 		_4q0 = 4.0f*IMU->quat.qw;
// 		_2q1 = 2.0f*IMU->quat.qx;
// 		_4q1 = 4.0f*IMU->quat.qx;
// 		_2q3 = 2.0f*IMU->quat.qz;
// 		_2q2 = 2.0f*IMU->quat.qy;
// 		_2q0 = 2.0f*IMU->quat.qw;
// 		_4q2 = 4.0f*IMU->quat.qy;
// 		_8q2 = 8.0f*IMU->quat.qy;
// 		_8q1 = 8.0f*IMU->quat.qx;
// 		s0 = _4q0 * q2q2 + _2q2 * IMU->accel[0] + _4q0 * q1q1 - _2q1 * IMU->accel[1];
// 		s1 = _4q1 * q3q3 - _2q3 * IMU->accel[0] + 4.0f * q0q0 * IMU->quat.qx - _2q0 * IMU->accel[1] - _4q1 + _8q1 * q1q1 + _8q1 * q2q2 + _4q1 * IMU->accel[2];
// 		s2 = 4.0f * q0q0 * IMU->quat.qy + _2q0 * IMU->accel[0] + _4q2 * q3q3 - _2q3 * IMU->accel[1] - _4q2 + _8q2 * q1q1 + _8q2 * q2q2 + _4q2 * IMU->accel[2];
// 		s3 = 4.0f * q1q1 * IMU->quat.qz - _2q1 * IMU->accel[0] + 4.0f * q2q2 * IMU->quat.qz - _2q2 * IMU->accel[1];
// 		float recipNorm;
// 		recipNorm = 1.0f/sqrtf(s0 * s0 + s1 * s1 + s2 * s2 + s3 * s3); // normalise step magnitude
// 		s0 *= recipNorm;
// 		s1 *= recipNorm;
// 		s2 *= recipNorm;
// 		s3 *= recipNorm;
// 		IMU->quat.qw -= 0.001f*s0;
//         IMU->quat.qx -= 0.001f*s1;
// 		IMU->quat.qy -= 0.001f*s2;
// 		IMU->quat.qz -= 0.001f*s3;
// }

// void IMU_Bmi088_Update(IMU_Bmi088 *IMU)
// {
//     if(IMU->accel_update_ready_flag)
// 	{
// 		BMI088_ACCEL_NS_L();
// 		HAL_SPI_TransmitReceive_DMA(&hspi1,IMU->accel_tx,IMU->accel_rx,ACCEL_BUFFERSIZE);
// 		IMU->spi_user = accel;
// 		IMU->accel_update_ready_flag = 0;
// 	}
// 	else if(IMU->gyro_update_ready_flag)
// 	{
// 		BMI088_GYRO_NS_L();
// 		HAL_SPI_TransmitReceive_DMA(&hspi1,IMU->gyro_tx,IMU->gyro_rx,GYRO_BUFFERSIZE);
// 		IMU->spi_user = gyro;
// 		IMU->gyro_update_ready_flag = 0;
// 	}
// 	else if(IMU->temp_update_ready_flag)
// 	{
// 		BMI088_ACCEL_NS_L();
// 		HAL_SPI_TransmitReceive_DMA(&hspi1,IMU->temp_tx,IMU->temp_rx,TEMP_BUFFERSIZE);
// 		IMU->spi_user = temp;
// 		IMU->temp_update_ready_flag = 0;
// 	}
// 	else
// 	{
// 		IMU->spi_user = empty;
// 	}
// }


// void Quaternion_normalize( Quaternion *quat )
// {
// 	float inv_length = quat->qw*quat->qw + quat->qx*quat->qx + quat->qy*quat->qy + quat->qz*quat->qz;
// 	arm_sqrt_f32 ( inv_length , &inv_length );
// 	inv_length = 1.0f / inv_length;
// 	quat->qw *= inv_length;
// 	quat->qx *= inv_length;
// 	quat->qy *= inv_length;
// 	quat->qz *= inv_length;
	
// }

// void Quaternion_Integral_Runge1( Quaternion *quat , vector3_float *delta_angle )
// {
// 	float tqw=quat->qw;	float tqx=quat->qx;	float tqy=quat->qy;	float tqz=quat->qz;
// 	quat->qw += 0.5f * ( -tqx*delta_angle->x - tqy*delta_angle->y - tqz*delta_angle->z );
// 	quat->qx += 0.5f * ( tqw*delta_angle->x + tqy*delta_angle->z - tqz*delta_angle->y );
// 	quat->qy += 0.5f * ( tqw*delta_angle->y - tqx*delta_angle->z + tqz*delta_angle->x );
// 	quat->qz += 0.5f * ( tqw*delta_angle->z + tqx*delta_angle->y - tqy*delta_angle->x );
// 	Quaternion_normalize( quat );
	
// }


// float Quaternion_getPitch( Quaternion *quat )
// {
// 	return asinf( 2.0f*(quat->qw*quat->qy-quat->qx*quat->qz) );
// }
// float Quaternion_getRoll( Quaternion *quat )
// {
// 	return atan2f( 2.0f*(quat->qw*quat->qx+quat->qy*quat->qz) ,\
// 		1.0f-2.0f*(quat->qx*quat->qx+quat->qy*quat->qy) );
// }	
// float Quaternion_getYaw( Quaternion *quat )
// {
// 	return atan2f( 2.0f*(quat->qw*quat->qz+quat->qx*quat->qy) ,\
// 		1.0f-2.0f*(quat->qy*quat->qy+quat->qz*quat->qz) );
// }

// void BMI088_AccelUpdatedCallback()
// {
// 	//System_SoftWatchDogFeed(&bmi088_dog);
// 	int16_t bmi088_raw_temp;
// 	bmi088_raw_temp = (int16_t)((uint16_t)(IMU.accel_rx[3]) << 8) | (uint16_t)IMU.accel_rx[2];
// 	IMU.accel[0] = bmi088_raw_temp * BMI088_ACCEL_3G_SEN;
// 	bmi088_raw_temp = (int16_t)((uint16_t)(IMU.accel_rx[5]) << 8) | (uint16_t)IMU.accel_rx[4];
// 	IMU.accel[1] = bmi088_raw_temp * BMI088_ACCEL_3G_SEN;
// 	bmi088_raw_temp = (int16_t)((uint16_t)(IMU.accel_rx[7]) << 8) | (uint16_t)IMU.accel_rx[6];
// 	IMU.accel[2] = bmi088_raw_temp * BMI088_ACCEL_3G_SEN;
	
// 	for(uint8_t i=0;i<3;i++)
// 	{
// 		IMU.accel[i] *= IMU.accel_offset[i];
// //		IMU.accel[i] = Filter_LowPass(&IMU.accel_filter[i], IMU.accel[i]);
// 	}
		
// 	IMU.accel_quad_sum = 
// 	IMU.accel[0] * IMU.accel[0] + \
// 	IMU.accel[1] * IMU.accel[1] + \
// 	IMU.accel[2] * IMU.accel[2];
	
// 	arm_sqrt_f32(IMU.accel_quad_sum,&IMU.accel_sum);
// 	Acc_Compensation(&IMU);

// }



// void BMI088_GyroUpdatedCallback()
// {
// 	//System_SoftWatchDogFeed(&bmi088_dog);
// 	int16_t bmi088_raw_temp;
// 	if(IMU.gyro_rx[1] == BMI088_GYRO_CHIP_ID_VALUE)
//   {
// 		bmi088_raw_temp = (int16_t)((uint16_t)(IMU.gyro_rx[4]) << 8) | (uint16_t)IMU.gyro_rx[3];
// 		IMU.imuSpeedRad[0] = bmi088_raw_temp * BMI088_GYRO_2000_SEN;
// 		bmi088_raw_temp = (int16_t)((uint16_t)(IMU.gyro_rx[6]) << 8) | (uint16_t)IMU.gyro_rx[5];
// 		IMU.imuSpeedRad[1] = bmi088_raw_temp * BMI088_GYRO_2000_SEN;
// 		bmi088_raw_temp = (int16_t)((uint16_t)(IMU.gyro_rx[8]) << 8) | (uint16_t)IMU.gyro_rx[7];
// 		IMU.imuSpeedRad[2] = bmi088_raw_temp * BMI088_GYRO_2000_SEN;
// 	  float t;			
// 	  t = (uwTick - BMI088_time)*1.0f/1000.0f; //获取积分时间
// 	  BMI088_time = uwTick;
// 	  IMU.delta_angle.x = (IMU.imuSpeedRad[0]-IMU.gyro_offset[0]) * t;
// 	  IMU.delta_angle.y = (IMU.imuSpeedRad[1]-IMU.gyro_offset[1]) * t;
//     IMU.delta_angle.z = (IMU.imuSpeedRad[2]-IMU.gyro_offset[2]) * t;
// 	  Quaternion_Integral_Runge1(&IMU.quat,&IMU.delta_angle);
// 	}
// }

// void BMI088_TempUpdatedCallback()
// {
// //	System_SoftWatchDogFeed(&bmi088_dog);
// 	int16_t bmi088_raw_temp;
// 	bmi088_raw_temp = (int16_t)((uint16_t)(IMU.temp_rx[2] << 3) | (uint16_t)(IMU.temp_rx[3] >> 5));
// 	if (bmi088_raw_temp > 1023)
// 	{
// 			bmi088_raw_temp -= 2048;
// 	}
// 	IMU.temperature = bmi088_raw_temp * BMI088_TEMP_FACTOR + BMI088_TEMP_OFFSET;
// //	IMU.temperature = Filter_LowPass(&IMU.temp_filter, IMU.temperature);
// }

// void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
// {
// 	BMI088_ACCEL_NS_H();
// 	BMI088_GYRO_NS_H();   
	
// 	switch(IMU.spi_user)
// 	{
// 		case accel:
// 		{
// 			BMI088_AccelUpdatedCallback();
// 			break;
// 		}
// 		case gyro:
// 		{
// 			BMI088_GyroUpdatedCallback();
// 			break;
// 		}
// 		case temp:
// 		{
// 			BMI088_TempUpdatedCallback();
// 			break;
// 		}
		
// 		default:
// 			break;
// 	}
	
// 	IMU.spi_user = empty;
// }

// void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
// {
// 	if(GPIO_Pin == GPIO_PIN_1)
// 	{
// 		IMU.accel_update_ready_flag = 1;
// 		IMU.temp_update_ready_flag = 1;
// 	}
// 	if(GPIO_Pin == GPIO_PIN_0)
// 	{
// 		IMU.gyro_update_ready_flag = 1;
// 	}
// }
