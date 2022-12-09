#include "slope.h"
#include "pid.h"
#include "config.h"
#include "softbus.h"
#include "cmsis_os.h"
#include "motor.h"

#include <stdbool.h>
// 底盘模式
typedef enum
{
    ChassisMode_Follow,   // 底盘跟随云台模式
    ChassisMode_Spin,     // 小陀螺模式
    ChassisMode_Snipe_10, // 狙击模式10m，底盘与云台成45度夹角，移速大幅降低,鼠标DPS大幅降低
    ChassisMode_Snipe_20  // 20m
} ChassisMode;

typedef struct Chassis
{
    // 底盘移动信息
    struct Move
    {
        float vx; // 当前左右平移速度 mm/s
        float vy; // 当前前后移动速度 mm/s
        float vw; // 当前旋转速度 rad/s

        float maxVx, maxVy, maxVw; // 三个分量最大速度
        Slope xSlope, ySlope;      // 斜坡
    } move;
    float speedRto; // 底盘速度百分比 用于低速运动
    // 旋转信息
    struct
    {
        PID pid;             // 旋转PID，由relativeAngle计算底盘旋转速度
        float relativeAngle; // 云台与底盘的偏离角 单位度
        int16_t InitAngle;   // 云台与底盘对齐时的编码器值
        int16_t nowAngle;    // 此时云台的编码器值
        ChassisMode mode;    // 底盘模式
    } rotate;
    bool RockerCtrl; //是否为遥控器控制

} Chassis;



//WASD移动
void Chassis_MoveCallback(const char* topic, SoftBusFrame* frame, void* bindData)
{
    Chassis chassis = *(Chassis*) bindData;
    if(!strcmp(topic,"rc/key"))
    {
        char *key_type = SoftBus_GetMapValue(frame, "key");
        switch (*key_type) {
            case 'W':
                Slope_SetTarget(&chassis.move.ySlope, -chassis.move.maxVy * chassis.speedRto);
                break;
            case 'S':
                Slope_SetTarget(&chassis.move.ySlope, chassis.move.maxVy * chassis.speedRto);
                break;
            case 'A':
                Slope_SetTarget(&chassis.move.xSlope, -chassis.move.maxVy * chassis.speedRto);
                break;
            case 'D':
                Slope_SetTarget(&chassis.move.xSlope, chassis.move.maxVy * chassis.speedRto);
                break;
            default:
                break;
        }
    }
}
//停止移动
void Chassis_StopCallback(const char* topic, SoftBusFrame* frame, void* bindData)
{
    Chassis chassis = *(Chassis*)bindData;
    if(!strcmp(topic,"rc/key"))
    {
        char* key_type = SoftBus_GetMapValue(frame,"key");
        switch (*key_type)
        {
            case 'W':
            case 'S':
                Slope_SetTarget(&chassis.move.ySlope,0);
                break;
            case 'A':
            case 'D':
                Slope_SetTarget(&chassis.move.xSlope,0);
                break;
            default:
                break;
        }
    }
}
//底盘模式切换
void Chassis_SwitchModeCallback(const char* topic,SoftBusFrame* frame,void* bindData)
{
    Chassis chassis = *(Chassis*)bindData;
    float speedRto = 1;
    if(!strcmp(topic,"rc/key"))
    {
        char* key_type = SoftBus_GetMapValue(frame,"key");
        if(chassis.rotate.mode != ChassisMode_Follow && \
        !strcmp(key_type,"Q") || !strcmp(key_type,"E") || !strcmp(key_type,"R"))
        {
            chassis.rotate.mode = ChassisMode_Follow;

        }
        else
        {
            switch (*key_type)
            {
                case 'Q': //Spin
                    PID_Clear(&chassis.rotate.pid);
                    chassis.rotate.mode = ChassisMode_Spin;
                    break;
                case 'E': //snipe-10m
                    chassis.rotate.mode = ChassisMode_Snipe_10;
                    //gimbal&shooter
                    speedRto = 0.2;
                    break;
                case 'R': //snipe-20m
                    chassis.rotate.mode = ChassisMode_Snipe_20;
                    //gimbal&shooter
                    speedRto = 0.1;
                    break;
                default:
                    break;
            }
        }
    }
    SoftBus_Publish("chassis",{
    {"speedRto",&speedRto},
    {"mode",&chassis.rotate.mode}
    });
}
//快/慢速移动切换
void Chassis_SwitchSpeedCallback(const char* topic,SoftBusFrame* frame,void* bindData)
{
    Chassis chassis = *(Chassis*)bindData;
    if(!strcmp(topic,"rc/key"))
    {
        char* key_type = (char *) SoftBus_GetMapValue(frame, "key");
        if(chassis.speedRto == 1 && !strcmp(key_type,"C") )
            chassis.speedRto = 0.5;
        else
            chassis.speedRto = 1;
    }
}

//Spin
//void Chassis_RotateCallback(const char* topic,SoftBusFrame* frame,void* bindData)
//{
//    if(!strcmp(topic,"chassis/rotate/mode"))
//    {
//        char* mode_type = (char*) SoftBus_GetMapValue(frame,"mode");
//        if(strcmp(mode_type,ChassisMode_Follow))
//    }
//}

//控制方式是否为遥控器
void Chassis_isRockerCtrlCallback(const char* topic,SoftBusFrame* frame,void* bindData)
{
    Chassis chassis = *(Chassis*)bindData;
    if(!strcmp(topic,"rc/wheel"))
    {
        int16_t wheel_type = *(int16_t*) SoftBus_GetMapValue(frame,"wheel");
        if(wheel_type < -600)
            chassis.RockerCtrl = false;
        else if(wheel_type > 600)
            chassis.RockerCtrl = true;
    }
}

//底盘功能任务
void Chassis_ControlTaskCallback(const void* argument)
{
    SoftBus_MultiSubscribe(NULL,Chassis_MoveCallback,{"rc/key/on-pressing"}); //WASD按下底盘移动
    SoftBus_MultiSubscribe(NULL,Chassis_StopCallback,{"rc/key/on-up"}); //WASD松开底盘停止
    SoftBus_MultiSubscribe(NULL,Chassis_SwitchModeCallback,{"rc/key/on-down"}); //QER按下切换底盘模式
    SoftBus_MultiSubscribe(NULL,Chassis_isRockerCtrlCallback,{"rc/wheel"});  //左拨轮拨下-遥控器控制
    SoftBus_Subscribe(NULL,Chassis_SwitchSpeedCallback,"rc/key/on-down"); //C按下"下蹲"
}

