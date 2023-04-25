#include "BMI088driver.h"
#include "BMI088reg.h"
#include "softbus.h"
#include "cmsis_os.h"

#define spiWriteAddr(addr) ((addr)&0x7f)
#define spiReadAddr(addr) ((addr)|0x80)

#define BMI088_ACCEL_SEN BMI088_ACCEL_6G_SEN;
#define BMI088_GYRO_SEN  BMI088_GYRO_2000_SEN;

//加速度计、陀螺仪软复位命令(只写2个字节目前测出bug，但传3个字节就没问题了，目前没找到原因暂且传3个字节)
uint8_t accSoftResetCmd[] = {spiWriteAddr(BMI088_ACC_SOFTRESET), BMI088_ACC_SOFTRESET_VALUE, 0};
uint8_t gyroSoftResetCmd[] = {spiWriteAddr(BMI088_ACC_SOFTRESET), BMI088_ACC_SOFTRESET_VALUE, 0};
//加速度计、陀螺仪读id命令
uint8_t accIdCmd[] = {spiReadAddr(BMI088_ACC_CHIP_ID), 0, 0};
uint8_t gyroIdCmd[] = {spiReadAddr(BMI088_GYRO_CHIP_ID), 0};

//加速度计配置命令
uint8_t accPwrCtrlCmd[]={spiWriteAddr(BMI088_ACC_PWR_CTRL), BMI088_ACC_ENABLE_ACC_ON};
uint8_t accPwrConfCmd[]={spiWriteAddr(BMI088_ACC_PWR_CONF), BMI088_ACC_PWR_ACTIVE_MODE};
uint8_t accConfCmd[]={spiWriteAddr(BMI088_ACC_CONF),  BMI088_ACC_NORMAL| BMI088_ACC_800_HZ | BMI088_ACC_CONF_MUST_Set};
uint8_t accRangeCmd[]={spiWriteAddr(BMI088_ACC_RANGE), BMI088_ACC_RANGE_3G};
uint8_t accIOConfCmd[]={spiWriteAddr(BMI088_INT1_IO_CTRL), BMI088_ACC_INT1_IO_ENABLE | BMI088_ACC_INT1_GPIO_PP | BMI088_ACC_INT1_GPIO_LOW};
uint8_t accIOMapCmd[]={spiWriteAddr(BMI088_INT_MAP_DATA), BMI088_ACC_INT1_DRDY_INTERRUPT};

//陀螺仪配置命令
uint8_t gyroRangeCmd[]={spiWriteAddr(BMI088_GYRO_RANGE), BMI088_GYRO_2000};
uint8_t gyroBandWidthCmd[]={spiWriteAddr(BMI088_GYRO_BANDWIDTH), BMI088_GYRO_1000_116_HZ | BMI088_GYRO_BANDWIDTH_MUST_Set};
uint8_t gyroPwrConfCmd[]={spiWriteAddr(BMI088_GYRO_LPM1), BMI088_GYRO_NORMAL_MODE};
uint8_t gyroPwrCtrlCmd[]={spiWriteAddr(BMI088_GYRO_CTRL), BMI088_DRDY_ON};
uint8_t gyroIOConfCmd[]={spiWriteAddr(BMI088_GYRO_INT3_INT4_IO_CONF), BMI088_GYRO_INT3_GPIO_PP | BMI088_GYRO_INT3_GPIO_LOW};
uint8_t gyroIOMapCmd[]={spiWriteAddr(BMI088_GYRO_INT3_INT4_IO_MAP), BMI088_GYRO_DRDY_IO_INT3};

//加速度计陀螺仪获取数据命令
uint8_t accGetDataCmd[8]={spiReadAddr(BMI088_ACCEL_XOUT_L)};
uint8_t gyroGetDataCmd[7]={spiReadAddr(BMI088_GYRO_X_L)};
uint8_t tmpGetDataCmd[4]={spiReadAddr(BMI088_TEMP_M)};

bool BMI088_AccelInit(uint8_t spiX)
{
	uint8_t accId[3]= {0};
	Bus_RemoteCall("/spi/block", {{"spi-x", &spiX}, 
	                              {"txData", accIdCmd}, 
	                              {"len", IM_PTR(uint16_t, 3)}, 
	                              {"timeout", IM_PTR(uint32_t, 1000)}, 
	                              {"csName", "acc"}, 
	                              {"isBlock", IM_PTR(bool, true)}}); //读加速度计id
	osDelay(1);
	Bus_RemoteCall("/spi/block", {{"spi-x", &spiX}, 
	                              {"txData", accPwrConfCmd}, 
	                              {"len", IM_PTR(uint16_t, 2)}, 
	                              {"timeout", IM_PTR(uint32_t, 1000)}, 
	                              {"csName", "acc"}, 
	                              {"isBlock", IM_PTR(bool, true)}}); //配置进入活动模式
	osDelay(1);
	Bus_RemoteCall("/spi/block", {{"spi-x", &spiX}, 
	                              {"txData", accPwrCtrlCmd}, 
	                              {"len", IM_PTR(uint16_t, 2)}, 
	                              {"timeout", IM_PTR(uint32_t, 1000)}, 
	                              {"csName", "acc"}, 
	                              {"isBlock", IM_PTR(bool, true)}}); //打开加速度计电源
	osDelay(1);
	Bus_RemoteCall("/spi/block", {{"spi-x", &spiX}, 
	                              {"txData", accIdCmd}, 
	                              {"rxData", accId}, 
	                              {"len", IM_PTR(uint16_t, 3)}, 
	                              {"timeout", IM_PTR(uint32_t, 1000)}, 
	                              {"csName", "acc"}, 
	                              {"isBlock", IM_PTR(bool, true)}}); //读加速度计id
	osDelay(1);

	if(accId[2] != BMI088_ACC_CHIP_ID_VALUE) //如果加速度计id不对
	{
		Bus_RemoteCall("/spi/block", {{"spi-x", &spiX}, 
		                              {"txData", accSoftResetCmd}, 
		                              {"len", IM_PTR(uint16_t, 3)}, 
		                              {"timeout", IM_PTR(uint32_t, 1000)}, 
		                              {"csName", "acc"}, 
		                              {"isBlock", IM_PTR(bool, true)}}); //软复位
		osDelay(50);
        return true;
	}

	Bus_RemoteCall("/spi/block", {{"spi-x", &spiX}, 
	                              {"txData", accConfCmd}, 
	                              {"len", IM_PTR(uint16_t, 2)}, 
	                              {"timeout", IM_PTR(uint32_t, 1000)}, 
	                              {"csName", "acc"}, 
	                              {"isBlock", IM_PTR(bool, true)}}); //配置加速度计输出速率800Hz, Normal输出模式
	osDelay(1);
	Bus_RemoteCall("/spi/block", {{"spi-x", &spiX}, 
	                              {"txData", accRangeCmd}, 
	                              {"len", IM_PTR(uint16_t, 2)}, 
	                              {"timeout", IM_PTR(uint32_t, 1000)}, 
	                              {"csName", "acc"}, 
	                              {"isBlock", IM_PTR(bool, true)}}); //配置加速度计量程+-3g
	osDelay(1);
	Bus_RemoteCall("/spi/block", {{"spi-x", &spiX}, 
	                              {"txData", gyroPwrConfCmd}, 
	                              {"len", IM_PTR(uint16_t, 2)}, 
	                              {"timeout", IM_PTR(uint32_t, 1000)}, 
	                              {"csName", "acc"}, 
	                              {"isBlock", IM_PTR(bool, true)}}); //配置进入正常模式
	osDelay(1);
	Bus_RemoteCall("/spi/block", {{"spi-x", &spiX}, 
	                              {"txData", accIOConfCmd}, 
	                              {"len", IM_PTR(uint16_t, 2)}, 
	                              {"timeout", IM_PTR(uint32_t, 1000)}, 
	                              {"csName", "acc"}, 
	                              {"isBlock", IM_PTR(bool, true)}}); //
	osDelay(1);
	Bus_RemoteCall("/spi/block", {{"spi-x", &spiX}, 
	                              {"txData", accIOMapCmd}, 
	                              {"len", IM_PTR(uint16_t, 2)}, 
	                              {"timeout", IM_PTR(uint32_t, 1000)}, 
	                              {"csName", "acc"}, 
	                              {"isBlock", IM_PTR(bool, true)}}); //
	osDelay(1);
    return false;
}

bool BMI088_GyroInit(uint8_t spiX)
{
    uint8_t gyroId[3]= {0};
	Bus_RemoteCall("/spi/block", {{"spi-x", &spiX}, 
	                              {"txData", gyroIdCmd}, 
	                              {"len", IM_PTR(uint16_t, 2)}, 
	                              {"timeout", IM_PTR(uint32_t, 1000)}, 
	                              {"csName", "gyro"}, 
	                              {"isBlock", IM_PTR(bool, true)}}); //读陀螺仪id
	osDelay(1);
	Bus_RemoteCall("/spi/block", {{"spi-x", &spiX}, 
	                              {"txData", gyroIdCmd}, 
	                              {"rxData", gyroId}, 
	                              {"len", IM_PTR(uint16_t, 2)}, 
	                              {"timeout", IM_PTR(uint32_t, 1000)}, 
	                              {"csName", "gyro"}, 
	                              {"isBlock", IM_PTR(bool, true)}}); //读陀螺仪id
	osDelay(1);

	if(gyroId[1] != BMI088_GYRO_CHIP_ID_VALUE) //如果陀螺仪id不对
	{
		Bus_RemoteCall("/spi/block", {{"spi-x", &spiX}, 
		                              {"txData", gyroSoftResetCmd}, 
		                              {"len", IM_PTR(uint16_t, 3)}, 
		                              {"timeout", IM_PTR(uint32_t, 1000)}, 
		                              {"csName", "gyro"}, 
		                              {"isBlock", IM_PTR(bool, true)}}); //软复位
		osDelay(50);
        return true;
	}

	Bus_RemoteCall("/spi/block", {{"spi-x", &spiX}, 
	                              {"txData", gyroBandWidthCmd}, 
	                              {"len", IM_PTR(uint16_t, 2)}, 
	                              {"timeout", IM_PTR(uint32_t, 1000)}, 
	                              {"csName", "gyro"}, 
	                              {"isBlock", IM_PTR(bool, true)}}); //配置陀螺仪输出速率1000Hz, 带宽116Hz
	osDelay(1);
	Bus_RemoteCall("/spi/block", {{"spi-x", &spiX}, 
	                              {"txData", gyroRangeCmd}, 
	                              {"len", IM_PTR(uint16_t, 2)}, 
	                              {"timeout", IM_PTR(uint32_t, 1000)}, 
	                              {"csName", "gyro"}, 
	                              {"isBlock", IM_PTR(bool, true)}}); //配置陀螺仪输出范围+-2000°/s
	osDelay(1);
	Bus_RemoteCall("/spi/block", {{"spi-x", &spiX}, 
	                              {"txData", accPwrConfCmd}, 
	                              {"len", IM_PTR(uint16_t, 2)}, 
	                              {"timeout", IM_PTR(uint32_t, 1000)}, 
	                              {"csName", "gyro"}, 
	                              {"isBlock", IM_PTR(bool, true)}}); //配置进入活动模式
	osDelay(1);
	Bus_RemoteCall("/spi/block", {{"spi-x", &spiX}, 
	                              {"txData", accPwrCtrlCmd}, 
	                              {"len", IM_PTR(uint16_t, 2)}, 
	                              {"timeout", IM_PTR(uint32_t, 1000)}, 
	                              {"csName", "gyro"}, 
	                              {"isBlock", IM_PTR(bool, true)}}); //打开陀螺仪电源
	osDelay(1);
	Bus_RemoteCall("/spi/block", {{"spi-x", &spiX}, 
	                              {"txData", gyroIOConfCmd}, 
	                              {"len", IM_PTR(uint16_t, 2)}, 
	                              {"timeout", IM_PTR(uint32_t, 1000)}, 
	                              {"csName", "gyro"}, 
	                              {"isBlock", IM_PTR(bool, true)}}); //
	osDelay(1);
	Bus_RemoteCall("/spi/block", {{"spi-x", &spiX}, 
	                              {"txData", gyroIOMapCmd}, 
	                              {"len", IM_PTR(uint16_t, 2)}, 
	                              {"timeout", IM_PTR(uint32_t, 1000)}, 
	                              {"csName", "gyro"}, 
	                              {"isBlock", IM_PTR(bool, true)}}); //
	osDelay(1);
    return false;
}

void BMI088_ReadData(uint8_t spiX, float gyro[3], float accel[3], float *temperate)
{
	uint8_t buf[8] = {0};
	int16_t temp;

	Bus_RemoteCall("/spi/block", {{"spi-x", &spiX}, 
	                              {"txData", accGetDataCmd}, 
	                              {"rxData", buf}, 
	                              {"len", IM_PTR(uint16_t, 8)}, 
	                              {"timeout", IM_PTR(uint32_t, 1000)}, 
	                              {"csName", "acc"}, 
	                              {"isBlock", IM_PTR(bool, true)}}); //获取加速度计数据
	temp = (int16_t)((buf[3]) << 8) | buf[2];
	accel[0] = temp * BMI088_ACCEL_SEN;
	temp = (int16_t)((buf[5]) << 8) | buf[4];
	accel[1] = temp * BMI088_ACCEL_SEN;
	temp = (int16_t)((buf[7]) << 8) | buf[6];
	accel[2] = temp * BMI088_ACCEL_SEN;

	Bus_RemoteCall("/spi/block", {{"spi-x", &spiX}, 
	                              {"txData", gyroGetDataCmd}, 
	                              {"rxData", buf}, 
	                              {"len", IM_PTR(uint16_t, 7)}, 
	                              {"timeout", IM_PTR(uint32_t, 1000)}, 
	                              {"csName", "gyro"}, 
	                              {"isBlock", IM_PTR(bool, true)}}); //获取陀螺仪数据
	// if(buf[0] == BMI088_GYRO_CHIP_ID_VALUE)
	// {
	temp = (int16_t)((buf[2]) << 8) | buf[1];
	gyro[0] = temp * BMI088_GYRO_SEN;
	temp = (int16_t)((buf[4]) << 8) | buf[3];
	gyro[1] = temp * BMI088_GYRO_SEN;
	temp = (int16_t)((buf[6]) << 8) | buf[5];
	gyro[2] = temp * BMI088_GYRO_SEN;
	// }

	Bus_RemoteCall("/spi/block", {{"spi-x", &spiX}, 
	                              {"txData", tmpGetDataCmd}, 
	                              {"rxData", buf}, 
	                              {"len", IM_PTR(uint16_t, 4)}, 
	                              {"timeout", IM_PTR(uint32_t, 1000)}, 
	                              {"csName", "acc"}, 
	                              {"isBlock", IM_PTR(bool, true)}}); //获取陀螺仪数据
	temp = (int16_t)((buf[2] << 3) | (buf[3] >> 5));

	if (temp > 1023)
	{
		temp -= 2048;
	}

	*temperate = temp * BMI088_TEMP_FACTOR + BMI088_TEMP_OFFSET;
}




















extern float offset[3];
fp32 BMI088_ACCEL_SEN = BMI088_ACCEL_3G_SEN;
fp32 BMI088_GYRO_SEN = BMI088_GYRO_2000_SEN;



#if defined(BMI088_USE_SPI)

#define BMI088_accel_write_single_reg(reg, data) \
    {                                            \
        BMI088_ACCEL_NS_L();                     \
        BMI088_write_single_reg((reg), (data));  \
        BMI088_ACCEL_NS_H();                     \
    }
#define BMI088_accel_read_single_reg(reg, data) \
    {                                           \
        BMI088_ACCEL_NS_L();                    \
        BMI088_read_write_byte((reg) | 0x80);   \
        BMI088_read_write_byte(0x55);           \
        (data) = BMI088_read_write_byte(0x55);  \
        BMI088_ACCEL_NS_H();                    \
    }
//#define BMI088_accel_write_muli_reg( reg,  data, len) { BMI088_ACCEL_NS_L(); BMI088_write_muli_reg(reg, data, len); BMI088_ACCEL_NS_H(); }
#define BMI088_accel_read_muli_reg(reg, data, len) \
    {                                              \
        BMI088_ACCEL_NS_L();                       \
        BMI088_read_write_byte((reg) | 0x80);      \
        BMI088_read_muli_reg(reg, data, len);      \
        BMI088_ACCEL_NS_H();                       \
    }

#define BMI088_gyro_write_single_reg(reg, data) \
    {                                           \
        BMI088_GYRO_NS_L();                     \
        BMI088_write_single_reg((reg), (data)); \
        BMI088_GYRO_NS_H();                     \
    }
#define BMI088_gyro_read_single_reg(reg, data)  \
    {                                           \
        BMI088_GYRO_NS_L();                     \
        BMI088_read_single_reg((reg), &(data)); \
        BMI088_GYRO_NS_H();                     \
    }
//#define BMI088_gyro_write_muli_reg( reg,  data, len) { BMI088_GYRO_NS_L(); BMI088_write_muli_reg( ( reg ), ( data ), ( len ) ); BMI088_GYRO_NS_H(); }
#define BMI088_gyro_read_muli_reg(reg, data, len)   \
    {                                               \
        BMI088_GYRO_NS_L();                         \
        BMI088_read_muli_reg((reg), (data), (len)); \
        BMI088_GYRO_NS_H();                         \
    }

static void BMI088_write_single_reg(uint8_t reg, uint8_t data);
static void BMI088_read_single_reg(uint8_t reg, uint8_t *return_data);
//static void BMI088_write_muli_reg(uint8_t reg, uint8_t* buf, uint8_t len );
static void BMI088_read_muli_reg(uint8_t reg, uint8_t *buf, uint8_t len);

#endif


void BMI088_read(fp32 gyro[3], fp32 accel[3], fp32 *temperate)
{
    uint8_t buf[8] = {0, 0, 0, 0, 0, 0};
    int16_t bmi088_raw_temp;

//    BMI088_accel_read_muli_reg(BMI088_ACCEL_XOUT_L, buf, 6);

//    bmi088_raw_temp = (int16_t)((buf[1]) << 8) | buf[0];
//    accel[0] = bmi088_raw_temp * BMI088_ACCEL_SEN;
//    bmi088_raw_temp = (int16_t)((buf[3]) << 8) | buf[2];
//    accel[1] = bmi088_raw_temp * BMI088_ACCEL_SEN;
//    bmi088_raw_temp = (int16_t)((buf[5]) << 8) | buf[4];
//    accel[2] = bmi088_raw_temp * BMI088_ACCEL_SEN;

    BMI088_gyro_read_muli_reg(BMI088_GYRO_CHIP_ID, buf, 8);
    if(buf[0] == BMI088_GYRO_CHIP_ID_VALUE)
    {
        bmi088_raw_temp = (int16_t)((buf[3]) << 8) | buf[2];
        gyro[0] = bmi088_raw_temp * BMI088_GYRO_SEN;
        bmi088_raw_temp = (int16_t)((buf[5]) << 8) | buf[4];
        gyro[1] = bmi088_raw_temp * BMI088_GYRO_SEN;
        bmi088_raw_temp = (int16_t)((buf[7]) << 8) | buf[6];
        gyro[2] = bmi088_raw_temp * BMI088_GYRO_SEN;
    }
    BMI088_accel_read_muli_reg(BMI088_TEMP_M, buf, 2);

    bmi088_raw_temp = (int16_t)((buf[0] << 3) | (buf[1] >> 5));

    if (bmi088_raw_temp > 1023)
    {
        bmi088_raw_temp -= 2048;
    }

    *temperate = bmi088_raw_temp * BMI088_TEMP_FACTOR + BMI088_TEMP_OFFSET;
}

#if defined(BMI088_USE_SPI)

static void BMI088_write_single_reg(uint8_t reg, uint8_t data)
{
    BMI088_read_write_byte(reg);
    BMI088_read_write_byte(data);
}

static void BMI088_read_single_reg(uint8_t reg, uint8_t *return_data)
{
    BMI088_read_write_byte(reg | 0x80);
    *return_data = BMI088_read_write_byte(0x55);
}

//static void BMI088_write_muli_reg(uint8_t reg, uint8_t* buf, uint8_t len )
//{
//    BMI088_read_write_byte( reg );
//    while( len != 0 )
//    {

//        BMI088_read_write_byte( *buf );
//        buf ++;
//        len --;
//    }

//}

static void BMI088_read_muli_reg(uint8_t reg, uint8_t *buf, uint8_t len)
{
    BMI088_read_write_byte(reg | 0x80);

    while (len != 0)
    {

        *buf = BMI088_read_write_byte(0x55);
        buf++;
        len--;
    }
}
#elif defined(BMI088_USE_IIC)

#endif
