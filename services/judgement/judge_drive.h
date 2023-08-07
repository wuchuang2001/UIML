#ifndef __JUDGE_DRIVE_H__
#define __JUDGE_DRIVE_H__

#include "stdint.h"
#include "stdbool.h"
#include "config.h"

#define    JUDGE_DATA_ERROR      0
#define    JUDGE_DATA_CORRECT    1

#define    LEN_HEADER    5        //帧头长
#define    LEN_CMDID     2        //命令码长度
#define    LEN_TAIL      2	      //帧尾CRC16

//起始字节,协议固定为0xA5
#define    JUDGE_FRAME_HEADER         (0xA5)
#define    JUDGE_MAX_FRAME_LENGTH   128
//机器人颜色
typedef enum
{
	RobotColor_Red,
	RobotColor_Blue
}RobotColor;

typedef enum 
{
	FRAME_HEADER         = 0,
	CMD_ID               = 5,
	DATA                 = 7,
}JudgeFrameOffset;

//5字节帧头,偏移位置
typedef enum
{
	SOF          = 0,//起始位
	DATA_LENGTH  = 1,//帧内数据长度,根据这个来获取数据长度
	SEQ          = 3,//包序号
	CRC8         = 4 //CRC8
	
}FrameHeaderOffset;

/***************命令码ID********************/

/* 

	ID: 0x0001  Byte:  11   比赛状态数据               发送频率 1Hz      
	ID: 0x0002  Byte:  1    比赛结果数据               比赛结束后发送      
	ID: 0x0003  Byte:  32   比赛机器人血量数据          1Hz发送  
	ID: 0x0101  Byte:  4    场地事件数据               事件改变后发送
	ID: 0x0102  Byte:  4    场地补给站动作标识数据      动作改变后发送
	ID: 0x0104	Byte:  2    裁判警告信息
	ID: 0x0105	Byte:  1    飞镖发射口倒计时
	ID: 0X0201  Byte:  27   机器人状态数据             10Hz
	ID: 0X0202  Byte:  14   实时功率热量数据           50Hz       
	ID: 0x0203  Byte:  16   机器人位置数据             10Hz
	ID: 0x0204  Byte:  1    机器人增益数据             增益状态改变后发送
	ID: 0x0205  Byte:  1    空中机器人能量状态数据      10Hz
	ID: 0x0206  Byte:  1    伤害状态数据               伤害发生后发送
	ID: 0x0207  Byte:  7    实时射击数据               子弹发射后发送
	ID: 0x0208  Byte:  6    子弹剩余发射数
	ID: 0x0209  Byte:  4    机器人RFID状态
	ID: 0x020A  Byte:  6    飞镖机器人客户端指令数据
	ID: 0x0301  Byte:  n    机器人间交互数据            发送方触发发送,10Hz
*/


//命令码ID,用来判断接收的是什么数据
typedef enum
{ 
	ID_game_state       				= 0x0001,//比赛状态数据
	ID_game_result 	   					= 0x0002,//比赛结果数据
	ID_game_robot_HP      			= 0x0003,//比赛机器人血量数据
	ID_event_data  							= 0x0101,//场地事件数据 
	ID_supply_projectile_action = 0x0102,//场地补给站动作标识数据
	ID_referee_warning					= 0x0104,//裁判警告信息
	ID_dart_remaining_time			= 0x0105,//飞镖发射口倒计时
	ID_game_robot_state    			= 0x0201,//机器人状态数据
	ID_power_heat_data    			= 0x0202,//实时功率热量数据
	ID_game_robot_pos        		= 0x0203,//机器人位置数据
	ID_buff_musk								= 0x0204,//机器人增益数据
	ID_aerial_robot_energy			= 0x0205,//空中机器人能量状态数据
	ID_robot_hurt								= 0x0206,//伤害状态数据
	ID_shoot_data								= 0x0207,//实时射击数据
	ID_bullet_remaining					= 0x0208,//子弹剩余发射数
	ID_rfid_status							= 0x0209,//机器人RFID状态
	ID_dart_client_cmd					= 0x020A,//飞镖机器人客户端指令数据
} CmdID;

//命令码数据段长,根据官方协议来定义长度
typedef enum
{
	LEN_game_state                = 11,  //0x0001
	LEN_game_result               = 1,   //0x0002
	LEN_game_robot_HP             = 32,  //0x0003
	LEN_event_data                = 4,   //0x0101
	LEN_supply_projectile_action  = 4,   //0x0102
	LEN_referee_warning           = 2,   //0x0104
	LEN_dart_remaining_time       = 1,   //0x0105
	LEN_game_robot_state          = 27,  //0x0201
	LEN_power_heat_data           = 16,  //0x0202
	LEN_game_robot_pos            = 16,  //0x0203
	LEN_buff_musk                 = 1,   //0x0204
	LEN_aerial_robot_energy       = 1,   //0x0205
	LEN_robot_hurt                = 1,   //0x0206
	LEN_shoot_data                = 7,   //0x0207
	LEN_bullet_remaining          = 6,   //0x0208
	LEN_rfid_status               = 4,   //0x0209
	LEN_dart_client_cmd           = 6,   //0x020A
} JudgeDataLength;

/* 自定义帧头 */
typedef struct __packed
{
	uint8_t  SOF;
	uint16_t DataLength;
	uint8_t  Seq;
	uint8_t  CRC8;
} xFrameHeader;

/* ID: 0x0001  Byte:  11    比赛状态数据 */
typedef struct __packed 
{
	uint8_t game_type : 4;
	uint8_t game_progress : 4;
	uint16_t stage_remain_time;
	uint64_t SyncTimeStamp;
} ext_game_status_t;

/* ID: 0x0002  Byte:  1    比赛结果数据 */
typedef struct __packed 
{ 
	uint8_t winner;
} ext_game_result_t; 

/* ID: 0x0003  Byte:  32    比赛机器人血量数据 */
typedef struct __packed
{
	uint16_t red_1_robot_HP;
	uint16_t red_2_robot_HP;
	uint16_t red_3_robot_HP;
	uint16_t red_4_robot_HP;
	uint16_t red_5_robot_HP;
	uint16_t red_7_robot_HP;
	uint16_t red_outpost_HP;
	uint16_t red_base_HP;
	uint16_t blue_1_robot_HP;
	uint16_t blue_2_robot_HP;
	uint16_t blue_3_robot_HP;
	uint16_t blue_4_robot_HP;
	uint16_t blue_5_robot_HP;
	uint16_t blue_7_robot_HP;
	uint16_t blue_outpost_HP;
	uint16_t blue_base_HP;
} ext_game_robot_HP_t;

/* ID: 0x0101  Byte:  4    场地事件数据 */
typedef struct __packed 
{ 
	uint32_t event_type;
} ext_event_data_t; 

/* ID: 0x0102  Byte:  4    场地补给站动作标识数据 */
typedef struct __packed
{
	uint8_t supply_projectile_id;
	uint8_t supply_robot_id;
	uint8_t supply_projectile_step;
	uint8_t supply_projectile_num;
} ext_supply_projectile_action_t;

/* ID: 0x104    Byte: 2    裁判警告信息 */
typedef struct __packed
{
	uint8_t level;
	uint8_t foul_robot_id;
} ext_referee_warning_t;

/* ID: 0x105    Byte: 1    飞镖发射口倒计时 */
typedef struct __packed
{
	uint8_t dart_remaining_time;
} ext_dart_remaining_time_t;

/* ID: 0X0201  Byte: 27    机器人状态数据 */
typedef struct __packed
{
	uint8_t robot_id;
	uint8_t robot_level;
	uint16_t remain_HP;
	uint16_t max_HP;
	uint16_t shooter_id1_17mm_cooling_rate;
	uint16_t shooter_id1_17mm_cooling_limit;
	uint16_t shooter_id1_17mm_speed_limit;
	uint16_t shooter_id2_17mm_cooling_rate;
	uint16_t shooter_id2_17mm_cooling_limit;
	uint16_t shooter_id2_17mm_speed_limit;
	uint16_t shooter_id1_42mm_cooling_rate;
	uint16_t shooter_id1_42mm_cooling_limit;
	uint16_t shooter_id1_42mm_speed_limit;
	uint16_t chassis_power_limit;
	uint8_t mains_power_gimbal_output : 1;
	uint8_t mains_power_chassis_output : 1;
	uint8_t mains_power_shooter_output : 1;
} ext_game_robot_status_t;


/* ID: 0X0202  Byte: 16    实时功率热量数据 */
typedef struct __packed
{
	uint16_t chassis_volt;
	uint16_t chassis_current;
	float chassis_power;
	uint16_t chassis_power_buffer;
	uint16_t shooter_id1_17mm_cooling_heat;
	uint16_t shooter_id2_17mm_cooling_heat;
	uint16_t shooter_id1_42mm_cooling_heat;
} ext_power_heat_data_t;


/* ID: 0x0203  Byte: 16    机器人位置数据 */
typedef struct __packed 
{   
	float x;   
	float y;   
	float z;   
	float yaw; 
} ext_game_robot_pos_t; 


/* ID: 0x0204  Byte:  1    机器人增益数据 */
typedef struct __packed 
{ 
	uint8_t power_rune_buff; 
} ext_buff_musk_t; 


/* ID: 0x0205  Byte:  1    空中机器人能量状态数据 */
typedef struct __packed
{
	uint8_t attack_time;
} aerial_robot_energy_t;


/* ID: 0x0206  Byte:  1    伤害状态数据 */
typedef struct __packed 
{ 
	uint8_t armor_id : 4; 
	uint8_t hurt_type : 4; 
} ext_robot_hurt_t; 


/* ID: 0x0207  Byte:  7    实时射击数据 */
typedef struct __packed
{
	uint8_t bullet_type;
	uint8_t shooter_id;
	uint8_t bullet_freq;
	float bullet_speed;
} ext_shoot_data_t;

/* ID: 0x0208  Byte:  6    子弹剩余发射数 */
typedef struct __packed
{
	uint16_t bullet_remaining_num_17mm;
	uint16_t bullet_remaining_num_42mm;
	uint16_t coin_remaining_num;
} ext_bullet_remaining_t;

/* ID: 0x0209  Byte:  4    机器人RFID状态 */
typedef struct __packed
{
	uint32_t rfid_status;
} ext_rfid_status_t;

/* ID: 0x020A  Byte:  6    飞镖机器人客户端指令数据 */
typedef struct __packed
{
	uint8_t dart_launch_opening_status;
	uint8_t dart_attack_target;
	uint16_t target_change_time;
	uint16_t operate_launch_cmd_time;
} ext_dart_client_cmd_t;

/* 
	
	交互数据，包括一个统一的数据段头结构，
	包含了内容 ID，发送者以及接受者的 ID 和内容数据段，
	整个交互数据的包总共长最大为 128 个字节，
	减去 frame_header,cmd_id,frame_tail 以及数据段头结构的 6 个字节，
	故而发送的内容数据段最大为 113。
	整个交互数据 0x0301 的包上行频率为 10Hz。

	机器人 ID：
	1，英雄(红)；
	2，工程(红)；
	3/4/5，步兵(红)；
	6，空中(红)；
	7，哨兵(红)；
	11，英雄(蓝)；
	12，工程(蓝)；
	13/14/15，步兵(蓝)；
	16，空中(蓝)；
	17，哨兵(蓝)。 
	客户端 ID： 
	0x0101 为英雄操作手客户端( 红) ；
	0x0102 ，工程操作手客户端 ((红 )；
	0x0103/0x0104/0x0105，步兵操作手客户端(红)；
	0x0106，空中操作手客户端((红)； 
	0x0111，英雄操作手客户端(蓝)；
	0x0112，工程操作手客户端(蓝)；
	0x0113/0x0114/0x0115，操作手客户端步兵(蓝)；
	0x0116，空中操作手客户端(蓝)。 
*/
/* 交互数据接收信息：0x0301  */
typedef struct __packed 
{ 
	uint16_t data_cmd_id;    
	uint16_t send_ID;    
	uint16_t receiver_ID; 
} ext_student_interactive_header_data_t; 

/* 
	学生机器人间通信 cmd_id 0x0301，内容 ID:0x0200~0x02FF
	交互数据 机器人间通信：0x0301。
	发送频率：上限 10Hz  

	字节偏移量   大小       说明              备注 
	    0        2     数据的内容 ID     0x0200~0x02FF 
										可以在以上 ID 段选取，具体 ID 含义由参赛队自定义 
	
	    2        2      发送者的 ID     需要校验发送者的 ID 正确性， 
	
	    4        2      接收者的 ID     需要校验接收者的 ID 正确性，
										例如不能发送到敌对机器人的ID 
	
	    6        n        数据段        n 需要小于 113 

*/
typedef struct __packed 
{ 
	uint8_t data[100]; //数据段,n需要小于113
} robot_interactive_data_t;


/* 客户端 客户端自定义图形：cmd_id:0x030 */

//图形数据
typedef struct __packed {
	uint8_t graphic_name[3]; 
	uint32_t operate_tpye:3; 
	uint32_t graphic_tpye:3; 
	uint32_t layer:4; 
	uint32_t color:4; 
	uint32_t start_angle:9; 
	uint32_t end_angle:9; 
	uint32_t width:10; 
	uint32_t start_x:11; 
	uint32_t start_y:11;
	uint32_t radius:10; 
	uint32_t end_x:11; 
	uint32_t end_y:11; 
} graphic_data_struct_t;

//删除图形 data_cmd_id=0x0100
typedef struct __packed
{
	uint8_t operate_tpye;
	uint8_t layer;
} ext_client_custom_graphic_delete_t;

//绘制一个图形 data_cmd_id=0x0101
typedef struct __packed
{
	graphic_data_struct_t grapic_data_struct;
} ext_client_custom_graphic_single_t;

//文字数据 data_cmd_id=0x0110
typedef struct __packed {
	graphic_data_struct_t grapic_data_struct; 
	uint8_t data[30];
} ext_client_custom_character_t;

//机器人交互信息
typedef struct __packed
{
	xFrameHeader                           txFrameHeader;//帧头
	uint16_t                               CmdID;//命令码
	ext_student_interactive_header_data_t  dataFrameHeader;//数据段头结构
	robot_interactive_data_t               interactData;//数据段
	uint16_t                               FrameTail;//帧尾
}ext_CommunatianData_t;

//客户端自定义图形信息
typedef struct __packed
{
	xFrameHeader                           txFrameHeader;//帧头
	uint16_t                               CmdID;//命令码
	ext_student_interactive_header_data_t  dataFrameHeader;//数据段头结构
	ext_client_custom_graphic_single_t     graphData;//数据段
	uint16_t                               FrameTail;//帧尾
}ext_GraphData_t;

//客户端自定义文字信息
typedef struct __packed
{
	xFrameHeader                           txFrameHeader;//帧头
	uint16_t                               CmdID;//命令码
	ext_student_interactive_header_data_t  dataFrameHeader;//数据段头结构
	ext_client_custom_character_t          textData;//数据段
	uint16_t                               FrameTail;//帧尾
}ext_TextData_t;

//客户端自定义UI删除形状
typedef struct __packed
{
	xFrameHeader                           txFrameHeader;//帧头
	uint16_t                               CmdID;//命令码
	ext_student_interactive_header_data_t  dataFrameHeader;//数据段头结构
	ext_client_custom_graphic_delete_t     deleteData;//数据段
	uint16_t                               FrameTail;//帧尾
}ext_DeleteData_t;

//裁判系统发送数据帧
typedef struct
{
	uint8_t data[JUDGE_MAX_FRAME_LENGTH];
	uint16_t frameLength;
}JudgeTxFrame;

/*****************系统数据定义**********************/
typedef struct _judge
{
	xFrameHeader                    FrameHeader;            //帧头信息
	ext_game_status_t               GameState;              //0x0001     
	ext_game_result_t               GameResult;             //0x0002
	ext_game_robot_HP_t             GameRobotHP;            //0x0003    
	ext_event_data_t                EventData;              //0x0101
	ext_supply_projectile_action_t  SupplyProjectileAction; //0x0102
	ext_referee_warning_t           RefereeWarning;         //0x0104
	ext_dart_remaining_time_t       DartRemainingTime;      //0x0105
	ext_game_robot_status_t         GameRobotStat;          //0x0201  ***
	ext_power_heat_data_t           PowerHeatData;          //0x0202  ***
	ext_game_robot_pos_t            GameRobotPos;           //0x0203  ***
	ext_buff_musk_t                 BuffMusk;               //0x0204
	aerial_robot_energy_t           AerialRobotEnergy;      //0x0205
	ext_robot_hurt_t                RobotHurt;              //0x0206  ***
	ext_shoot_data_t                ShootData;              //0x0207  ***
	ext_bullet_remaining_t          BulletRemaining;        //0x0208  ***
	ext_rfid_status_t               RfidStatus;             //0x0209
	ext_dart_client_cmd_t           DartClientCmd;          //0x020A
}JudgeRecInfo;

/****************************************************/
//图形操作
typedef enum _GraphOperation
{
	Operation_Null=0,
	Operation_Add,
	Operation_Change,
	Operation_Delete
}GraphOperation;

//图形形状
typedef enum _GraphShape
{
	Shape_Line=0,
	Shape_Rect,
	Shape_Circle,
	Shape_Oval,
	Shape_Arc,
	Shape_Int,
	Shape_Float,
	Shape_Text
}GraphShape;

//图形颜色
typedef enum _GraphColor
{
	Color_Self=0,//己方主色
	Color_Yellow,
	Color_Green,
	Color_Orange,
	Color_Purple,
	Color_Pink,
	Color_Cyan,
	Color_Black,
	Color_White
}GraphColor;



JudgeTxFrame JUDGE_PackGraphData(uint8_t sendID,uint8_t receiveID,graphic_data_struct_t *data);
JudgeTxFrame JUDGE_PackTextData(uint8_t sendID,uint8_t receiveID,graphic_data_struct_t *textConf,uint8_t text[30]);
#endif

