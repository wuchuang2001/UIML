#include "slope.h"
#include "config.h"
#include "softbus.h"
#include "motor.h"
#include "cmsis_os.h"
#include "arm_math.h"
#include "pid.h"

#define CHASSIS_ACC2SLOPE(taskInterval, acc) ((taskInterval) * (acc) / 1000) // mm/s2

// 底盘模式
typedef enum
{
    ChassisMode_Follow = 0,   // 底盘跟随云台模式
    ChassisMode_Spin,     // 小陀螺模式
    ChassisMode_Snipe_10, // 狙击模式10m，底盘与云台成45度夹角，移速大幅降低,鼠标DPS大幅降低
    ChassisMode_Snipe_20  // 20m
} ChassisMode;

typedef struct _Chassis
{
    // 底盘尺寸信息
    struct Info
    {
        float wheelbase;   // 轴距
        float wheeltrack;  // 轮距
        float wheelRadius; // 轮半径
        float offsetX;     // 重心在xy轴上的偏移
        float offsetY;
    } info;
    // 4个电机
    Motor *motors[4];
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
    // 底盘旋转信息
    struct
    {
        PID pid;             // 旋转PID，由relativeAngle计算底盘旋转速度
        float relativeAngle; // 云台与底盘的偏离角 单位度
        int16_t InitAngle;   // 云台与底盘对齐时的编码器值
        int16_t nowAngle;    // 此时云台的编码器值
        ChassisMode mode;    // 底盘模式
    } rotate;
    float relativeAngle; // 与底盘的偏离角，单位度

    uint8_t taskInterval; // 任务间隔 用于设定步长

} Chassis;

Chassis chassis;

void Chassis_Init(Chassis *chassis, ConfItem *dict);
void Chassis_UpdateSlope(Chassis *chassis);
void Chassis_MoveCallback(const char *topic, SoftBusFrame *frame, void *bindData); 
void Chassis_StopCallback(const char *topic, SoftBusFrame *frame, void *bindData);
void Chassis_SwitchModeCallback(const char *topic, SoftBusFrame *frame, void *bindData);
void Chassis_SwitchSpeedCallback(const char *topic, SoftBusFrame *frame, void *bindData);
void Chassis_RegisterEvents(void);
void Chassis_SoftBusCallback(const char *topic, SoftBusFrame *frame, void *bindData);

// 底盘任务回调函数
void Chassis_TaskCallback(void const *argument)
{
    // 进入临界区
    portENTER_CRITICAL();
    Chassis chassis = {0};
    Chassis_Init(&chassis, (ConfItem *)argument);
    portEXIT_CRITICAL();

    osDelay(2000);
    TickType_t tick = xTaskGetTickCount();
    while (1)
    {
        /*************计算底盘平移速度**************/

        Chassis_UpdateSlope(&chassis); // 更新运动斜坡函数数据

        // 将云台坐标系下平移速度解算到底盘平移速度(根据云台偏离角)
        float gimbalAngleSin = arm_sin_f32(chassis.relativeAngle * PI / 180);
        float gimbalAngleCos = arm_cos_f32(chassis.relativeAngle * PI / 180);
        chassis.move.vx = Slope_GetVal(&chassis.move.xSlope) * gimbalAngleCos + Slope_GetVal(&chassis.move.ySlope) * gimbalAngleSin;
        chassis.move.vy = -Slope_GetVal(&chassis.move.xSlope) * gimbalAngleSin + Slope_GetVal(&chassis.move.ySlope) * gimbalAngleCos;
        float vw = chassis.move.vw / 180 * PI;

        /*************解算各轮子转速**************/

        float rotateRatio[4];
        rotateRatio[0] = (chassis.info.wheelbase + chassis.info.wheeltrack) / 2.0f - chassis.info.offsetY + chassis.info.offsetX;
        rotateRatio[1] = (chassis.info.wheelbase + chassis.info.wheeltrack) / 2.0f - chassis.info.offsetY - chassis.info.offsetX;
        rotateRatio[2] = (chassis.info.wheelbase + chassis.info.wheeltrack) / 2.0f + chassis.info.offsetY + chassis.info.offsetX;
        rotateRatio[3] = (chassis.info.wheelbase + chassis.info.wheeltrack) / 2.0f + chassis.info.offsetY - chassis.info.offsetX;
        float wheelRPM[4];
        wheelRPM[0] = (chassis.move.vx + chassis.move.vy - vw * rotateRatio[0]) * 60 / (2 * PI * chassis.info.wheelRadius);   // FL
        wheelRPM[1] = -(-chassis.move.vx + chassis.move.vy + vw * rotateRatio[1]) * 60 / (2 * PI * chassis.info.wheelRadius); // FR
        wheelRPM[2] = (-chassis.move.vx + chassis.move.vy - vw * rotateRatio[2]) * 60 / (2 * PI * chassis.info.wheelRadius);  // BL
        wheelRPM[3] = -(chassis.move.vx + chassis.move.vy + vw * rotateRatio[3]) * 60 / (2 * PI * chassis.info.wheelRadius);  // BR

        for (uint8_t i = 0; i < 4; i++)
        {
            chassis.motors[i]->setTarget(chassis.motors[i], wheelRPM[i]);
        }

        osDelayUntil(&tick, chassis.taskInterval);
    }
}

void Chassis_Init(Chassis *chassis, ConfItem *dict)
{
    // 任务间隔
    chassis->taskInterval = Conf_GetValue(dict, "taskInterval", uint8_t, 2);
    // 底盘尺寸信息（用于解算轮速）
    chassis->info.wheelbase = Conf_GetValue(dict, "info/wheelbase", float, 0);
    chassis->info.wheeltrack = Conf_GetValue(dict, "info/wheeltrack", float, 0);
    chassis->info.wheelRadius = Conf_GetValue(dict, "info/wheelRadius", float, 76);
    chassis->info.offsetX = Conf_GetValue(dict, "info/offsetX", float, 0);
    chassis->info.offsetY = Conf_GetValue(dict, "info/offsetY", float, 0);
    // 移动参数初始化
    chassis->move.maxVx = Conf_GetValue(dict, "move/maxVx", float, 2000);
    chassis->move.maxVy = Conf_GetValue(dict, "move/maxVy", float, 2000);
    chassis->move.maxVw = Conf_GetValue(dict, "move/maxVw", float, 2);
    // 底盘加速度初始化
    float xAcc = Conf_GetValue(dict, "move/xAcc", float, 1000);
    float yAcc = Conf_GetValue(dict, "move/yAcc", float, 1000);
    Slope_Init(&chassis->move.xSlope, CHASSIS_ACC2SLOPE(chassis->taskInterval, xAcc), 0);
    Slope_Init(&chassis->move.ySlope, CHASSIS_ACC2SLOPE(chassis->taskInterval, yAcc), 0);
    // 底盘电机初始化
    chassis->motors[0] = Motor_Init(Conf_GetPtr(dict, "motorFL", ConfItem));
    chassis->motors[1] = Motor_Init(Conf_GetPtr(dict, "motorFR", ConfItem));
    chassis->motors[2] = Motor_Init(Conf_GetPtr(dict, "motorBL", ConfItem));
    chassis->motors[3] = Motor_Init(Conf_GetPtr(dict, "motorBR", ConfItem));
    // 设置底盘电机为速度模式
    for (uint8_t i = 0; i < 4; i++)
    {
        chassis->motors[i]->changeMode(chassis->motors[i], MOTOR_SPEED_MODE);
    }
    // 设置初始底盘状态
    chassis->rotate.mode = Conf_GetValue(dict, "rotate/mode", uint8_t, ChassisMode_Follow);


    SoftBus_MultiSubscribe(chassis, Chassis_SoftBusCallback, {"/chassis/move", "/chassis/acc", "/chassis/relativeAngle"});
    Chassis_RegisterEvents();
}

// 更新斜坡计算速度
void Chassis_UpdateSlope(Chassis *chassis)
{
    Slope_NextVal(&chassis->move.xSlope);
    Slope_NextVal(&chassis->move.ySlope);
}

// WASD移动
void Chassis_MoveCallback(const char *topic, SoftBusFrame *frame, void *bindData)
{
    if (!strcmp(topic, "rc/key"))
    {
        char *key_type = SoftBus_GetMapValue(frame, "key");
        switch (*key_type)
        {
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

// 停止移动
void Chassis_StopCallback(const char *topic, SoftBusFrame *frame, void *bindData)
{
    if (!strcmp(topic, "rc/key"))
    {
        char *key_type = SoftBus_GetMapValue(frame, "key");
        switch (*key_type)
        {
        case 'W':
        case 'S':
            Slope_SetTarget(&chassis.move.ySlope, 0);
            break;
        case 'A':
        case 'D':
            Slope_SetTarget(&chassis.move.xSlope, 0);
            break;
        default:
            break;
        }
    }
}

// 底盘模式切换
void Chassis_SwitchModeCallback(const char *topic, SoftBusFrame *frame, void *bindData)
{
    if (!strcmp(topic, "rc/key"))
    {
        char *key_type = SoftBus_GetMapValue(frame, "key");
        if (chassis.rotate.mode != ChassisMode_Follow && \
                (!strcmp(key_type, "Q") || \
            !strcmp(key_type, "E") || !strcmp(key_type, "R")))
        {
            chassis.rotate.mode = ChassisMode_Follow;
            chassis.speedRto = 1;
        }
        else
        {
            switch (*key_type)
            {
            case 'Q': // Spin
                PID_Clear(&chassis.rotate.pid);
                chassis.rotate.mode = ChassisMode_Spin;
                break;
            case 'E': // snipe-10m
                chassis.rotate.mode = ChassisMode_Snipe_10;
                // gimbal&shooter
                chassis.speedRto = 0.2;
                break;
            case 'R': // snipe-20m
                chassis.rotate.mode = ChassisMode_Snipe_20;
                // gimbal&shooter
                chassis.speedRto = 0.1;
                break;
            default:
                break;
            }
        }
    }
}

// 快/慢速移动切换
void Chassis_SwitchSpeedCallback(const char *topic, SoftBusFrame *frame, void *bindData)
{
    if (!strcmp(topic, "rc/key"))
    {
        char *key_type = (char *)SoftBus_GetMapValue(frame, "key");

        if (chassis.speedRto == 1 && !strcmp(key_type, "C"))
            chassis.speedRto = 0.5;
        else
            chassis.speedRto = 1;
    }
}

void Chassis_RegisterEvents(void)
{
    SoftBus_MultiSubscribe(NULL, Chassis_MoveCallback, {"rc/key/on-pressing"});   // WASD按下底盘移动
    SoftBus_MultiSubscribe(NULL, Chassis_StopCallback, {"rc/key/on-up"});         // WASD松开底盘停止
    SoftBus_MultiSubscribe(NULL, Chassis_SwitchModeCallback, {"rc/key/on-down"}); // QER按下切换底盘模式
    SoftBus_Subscribe(NULL, Chassis_SwitchSpeedCallback, "rc/key/on-down");       // C按下下蹲
}

void Chassis_SoftBusCallback(const char *topic, SoftBusFrame *frame, void *bindData)
{
    Chassis *chassis = (Chassis *)bindData;
    if (!strcmp(topic, "/chassis/move"))
    {
        if (SoftBus_IsMapKeyExist(frame, "vx"))
            Slope_SetTarget(&chassis->move.xSlope, *(float *)SoftBus_GetMapValue(frame, "vx"));
        if (SoftBus_IsMapKeyExist(frame, "vy"))
            Slope_SetTarget(&chassis->move.ySlope, *(float *)SoftBus_GetMapValue(frame, "vy"));
        if (SoftBus_IsMapKeyExist(frame, "vw"))
            chassis->move.vw = *(float *)SoftBus_GetMapValue(frame, "vw");
    }
    else if (!strcmp(topic, "/chassis/acc"))
    {
        if (SoftBus_IsMapKeyExist(frame, "ax"))
            Slope_SetStep(&chassis->move.xSlope, CHASSIS_ACC2SLOPE(chassis->taskInterval, *(float *)SoftBus_GetMapValue(frame, "ax")));
        if (SoftBus_IsMapKeyExist(frame, "ay"))
            Slope_SetStep(&chassis->move.ySlope, CHASSIS_ACC2SLOPE(chassis->taskInterval, *(float *)SoftBus_GetMapValue(frame, "ay")));
    }
    else if(!strcmp(topic,"/chassis/rotate"))
    {
        if(SoftBus_IsMapKeyExist(frame, "relativeAngle"))
            chassis->rotate.relativeAngle = *(float*)SoftBus_GetMapValue(frame, "relativeAngle");
        if(SoftBus_IsMapKeyExist(frame,"InitAngle"))
            chassis->rotate.InitAngle = *(int16_t *) SoftBus_GetMapValue(frame,"InitAngle");
        if(SoftBus_IsMapKeyExist(frame, "nowAngle"))
            chassis->rotate.nowAngle = *(int16_t *)SoftBus_GetMapValue(frame, "nowAngle");
        if(SoftBus_IsMapKeyExist(frame, "mode"))
            chassis->rotate.mode = *(ChassisMode *)SoftBus_GetMapValue(frame, "mode");
    }
}

