#ifndef _JUDGEMENT_H_
#define _JUDGEMENT_H_
#include "string.h"
#include "crc_dji.h"
#include "config.h"
#include "softbus.h"
#include "cmsis_os.h"
#define    JUDGE_DATA_ERROR      0
#define    JUDGE_DATA_CORRECT    1

#define    LEN_HEADER    5        //帧头长
#define    LEN_CMDID     2        //命令码长度
#define    LEN_TAIL      2	      //帧尾CRC16

//起始字节,协议固定为0xA5
#define    JUDGE_FRAME_HEADER         (0xA5)
#define    JUDGE_MAX_TX_LENGTH   64
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

	ID: 0x0001  Byte:  11    比赛状态数据       			发送频率 1Hz      
	ID: 0x0002  Byte:  1    比赛结果数据         		比赛结束后发送      
	ID: 0x0003  Byte:  32    比赛机器人血量数据   		1Hz发送  
	ID: 0x0101  Byte:  4    场地事件数据   				事件改变后发送
	ID: 0x0102  Byte:  4    场地补给站动作标识数据    	动作改变后发送
	ID: 0x0104	Byte: 	2		裁判警告信息
	ID: 0x0105	Byte: 	1		飞镖发射口倒计时
	ID: 0X0201  Byte: 27    机器人状态数据        		10Hz
	ID: 0X0202  Byte: 14    实时功率热量数据   			50Hz       
	ID: 0x0203  Byte: 16    机器人位置数据           	10Hz
	ID: 0x0204  Byte:  1    机器人增益数据           	增益状态改变后发送
	ID: 0x0205  Byte:  1    空中机器人能量状态数据      10Hz
	ID: 0x0206  Byte:  1    伤害状态数据           		伤害发生后发送
	ID: 0x0207  Byte:  7    实时射击数据           		子弹发射后发送
	ID: 0x0208  Byte:  6    子弹剩余发射数
	ID: 0x0209  Byte:  4    机器人RFID状态
	ID: 0x020A  Byte:  6    飞镖机器人客户端指令数据
	ID: 0x0301  Byte:  n    机器人间交互数据           	发送方触发发送,10Hz
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
	LEN_game_state       					= 11,	//0x0001
	LEN_game_result       				= 1,	//0x0002
	LEN_game_robot_HP							= 32,	//0x0003
	LEN_event_data  							= 4,	//0x0101
	LEN_supply_projectile_action  = 4,	//0x0102
	LEN_referee_warning						= 2,	//0x0104
	LEN_dart_remaining_time				= 1,	//0x0105
	LEN_game_robot_state    			= 27,	//0x0201
	LEN_power_heat_data   				= 16,	//0x0202
	LEN_game_robot_pos        		= 16,	//0x0203
	LEN_buff_musk        					= 1,	//0x0204
	LEN_aerial_robot_energy       = 1,	//0x0205
	LEN_robot_hurt        				= 1,	//0x0206
	LEN_shoot_data       					= 7,	//0x0207
	LEN_bullet_remaining					= 6,	//0x0208
	LEN_rfid_status								= 4,	//0x0209
	LEN_dart_client_cmd						= 6,	//0x020A
} JudgeDataLength;

/* 自定义帧头 */
typedef __packed struct
{
	uint8_t  SOF;
	uint16_t DataLength;
	uint8_t  Seq;
	uint8_t  CRC8;
} xFrameHeader;

/* ID: 0x0001  Byte:  11    比赛状态数据 */
typedef __packed struct
{
	uint8_t game_type : 4;
	uint8_t game_progress : 4;
	uint16_t stage_remain_time;
	uint64_t SyncTimeStamp;
} ext_game_status_t;

/* ID: 0x0002  Byte:  1    比赛结果数据 */
typedef __packed struct 
{ 
	uint8_t winner;
} ext_game_result_t; 

/* ID: 0x0003  Byte:  32    比赛机器人血量数据 */
typedef __packed struct
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
typedef __packed struct 
{ 
	uint32_t event_type;
} ext_event_data_t; 

/* ID: 0x0102  Byte:  4    场地补给站动作标识数据 */
typedef __packed struct
{
	uint8_t supply_projectile_id;
	uint8_t supply_robot_id;
	uint8_t supply_projectile_step;
	uint8_t supply_projectile_num;
} ext_supply_projectile_action_t;

/* ID: 0x104    Byte: 2    裁判警告信息 */
typedef __packed struct
{
	uint8_t level;
	uint8_t foul_robot_id;
} ext_referee_warning_t;

/* ID: 0x105    Byte: 1    飞镖发射口倒计时 */
typedef __packed struct
{
	uint8_t dart_remaining_time;
} ext_dart_remaining_time_t;

/* ID: 0X0201  Byte: 27    机器人状态数据 */
typedef __packed struct
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
typedef __packed struct
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
typedef __packed struct 
{   
	float x;   
	float y;   
	float z;   
	float yaw; 
} ext_game_robot_pos_t; 


/* ID: 0x0204  Byte:  1    机器人增益数据 */
typedef __packed struct 
{ 
	uint8_t power_rune_buff; 
} ext_buff_musk_t; 


/* ID: 0x0205  Byte:  1    空中机器人能量状态数据 */
typedef __packed struct
{
	uint8_t attack_time;
} aerial_robot_energy_t;


/* ID: 0x0206  Byte:  1    伤害状态数据 */
typedef __packed struct 
{ 
	uint8_t armor_id : 4; 
	uint8_t hurt_type : 4; 
} ext_robot_hurt_t; 


/* ID: 0x0207  Byte:  7    实时射击数据 */
typedef __packed struct
{
	uint8_t bullet_type;
	uint8_t shooter_id;
	uint8_t bullet_freq;
	float bullet_speed;
} ext_shoot_data_t;

/* ID: 0x0208  Byte:  6    子弹剩余发射数 */
typedef __packed struct
{
	uint16_t bullet_remaining_num_17mm;
	uint16_t bullet_remaining_num_42mm;
	uint16_t coin_remaining_num;
} ext_bullet_remaining_t;

/* ID: 0x0209  Byte:  4    机器人RFID状态 */
typedef __packed struct
{
	uint32_t rfid_status;
} ext_rfid_status_t;

/* ID: 0x020A  Byte:  6    飞镖机器人客户端指令数据 */
typedef __packed struct
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
typedef __packed struct 
{ 
	uint16_t data_cmd_id;    
	uint16_t send_ID;    
	uint16_t receiver_ID; 
} ext_student_interactive_header_data_t; 

/* 
	学生机器人间通信 cmd_id 0x0301，内容 ID:0x0200~0x02FF
	交互数据 机器人间通信：0x0301。
	发送频率：上限 10Hz  

	字节偏移量 	大小 	说明 			备注 
	0 			2 		数据的内容 ID 	0x0200~0x02FF 
										可以在以上 ID 段选取，具体 ID 含义由参赛队自定义 
	
	2 			2 		发送者的 ID 	需要校验发送者的 ID 正确性， 
	
	4 			2 		接收者的 ID 	需要校验接收者的 ID 正确性，
										例如不能发送到敌对机器人的ID 
	
	6 			n 		数据段 			n 需要小于 113 

*/
typedef __packed struct 
{ 
	uint8_t data[100]; //数据段,n需要小于113
} robot_interactive_data_t;


/* 客户端 客户端自定义图形：cmd_id:0x030 */

//图形数据
typedef __packed struct {
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
typedef __packed struct
{
	uint8_t operate_tpye;
	uint8_t layer;
} ext_client_custom_graphic_delete_t;

//绘制一个图形 data_cmd_id=0x0101
typedef __packed struct
{
	graphic_data_struct_t grapic_data_struct;
} ext_client_custom_graphic_single_t;

//文字数据 data_cmd_id=0x0110
typedef __packed struct {
	graphic_data_struct_t grapic_data_struct; 
	uint8_t data[30];
} ext_client_custom_character_t;

//机器人交互信息
typedef __packed struct
{
	xFrameHeader   							txFrameHeader;//帧头
	uint16_t								CmdID;//命令码
	ext_student_interactive_header_data_t   dataFrameHeader;//数据段头结构
	robot_interactive_data_t  	 			interactData;//数据段
	uint16_t		 						FrameTail;//帧尾
}ext_CommunatianData_t;

//客户端自定义图形信息
typedef __packed struct
{
	xFrameHeader   							txFrameHeader;//帧头
	uint16_t								CmdID;//命令码
	ext_student_interactive_header_data_t   dataFrameHeader;//数据段头结构
	ext_client_custom_graphic_single_t  	 			graphData;//数据段
	uint16_t		 						FrameTail;//帧尾
}ext_GraphData_t;

//客户端自定义文字信息
typedef __packed struct
{
	xFrameHeader   							txFrameHeader;//帧头
	uint16_t								CmdID;//命令码
	ext_student_interactive_header_data_t   dataFrameHeader;//数据段头结构
	ext_client_custom_character_t  	 			textData;//数据段
	uint16_t		 						FrameTail;//帧尾
}ext_TextData_t;

//客户端自定义UI删除形状
typedef __packed struct
{
	xFrameHeader   							txFrameHeader;//帧头
	uint16_t								CmdID;//命令码
	ext_student_interactive_header_data_t   dataFrameHeader;//数据段头结构
	ext_client_custom_graphic_delete_t  	 			deleteData;//数据段
	uint16_t		 						FrameTail;//帧尾
}ext_DeleteData_t;

//裁判系统发送数据帧
typedef struct
{
	uint8_t data[JUDGE_MAX_TX_LENGTH];
	uint16_t frameLength;
}JudgeTxFrame;




/*****************系统数据定义**********************/
ext_game_status_t       				GameState;							//0x0001
ext_game_result_t            		GameResult;							//0x0002
ext_game_robot_HP_t          		GameRobotHP;						//0x0003
ext_event_data_t        				EventData;							//0x0101
ext_supply_projectile_action_t	SupplyProjectileAction;	//0x0102
ext_referee_warning_t						RefereeWarning;					//0x0104
ext_dart_remaining_time_t				DartRemainingTime;			//0x0105
ext_game_robot_status_t			  	GameRobotStat;					//0x0201
ext_power_heat_data_t		  			PowerHeatData;					//0x0202
ext_game_robot_pos_t						GameRobotPos;						//0x0203
ext_buff_musk_t									BuffMusk;								//0x0204
aerial_robot_energy_t						AerialRobotEnergy;			//0x0205
ext_robot_hurt_t								RobotHurt;							//0x0206
ext_shoot_data_t								ShootData;							//0x0207
ext_bullet_remaining_t					BulletRemaining;				//0x0208
ext_rfid_status_t								RfidStatus;							//0x0209
ext_dart_client_cmd_t						DartClientCmd;					//0x020A

xFrameHeader              			FrameHeader;						//发送帧头信息
/****************************************************/
bool Judge_Data_TF = FALSE;//裁判数据是否可用,辅助函数调用

typedef struct _Judge
{

	uint8_t uartX;
	uint8_t taskInterval;	
	
}Judge;

void Judge_Init(Judge *judge,ConfItem* dict);
void Judge_SoftBusCallback(const char* topic, SoftBusFrame* frame, void* bindData);
bool JUDGE_Read_Data(uint8_t *ReadFromUsart);

void Judge_TaskCallback(void const * argument)
{
	//进入临界区
	portENTER_CRITICAL();
	Judge judge={0};
	Judge_Init(&judge, (ConfItem*)argument);
	portEXIT_CRITICAL();
	osDelay(2000);
	TickType_t tick = xTaskGetTickCount();
	while (1)
	{
		
		osDelayUntil(&tick,judge.taskInterval);
	}		
}

void Judge_Init(Judge *judge,ConfItem* dict)
{
	char name[] = "/uart_/recv";
	judge->uartX = Conf_GetValue(dict, "uart-x", uint8_t, 0);
	name[5] = judge->uartX + '0';
	Bus_RegisterReceiver(NULL, Judge_SoftBusCallback, name);
	judge->taskInterval = Conf_GetValue(dict, "taskInterval", uint8_t, 150);
}
void Judge_SoftBusCallback(const char* topic, SoftBusFrame* frame, void* bindData)
{
	
	uint8_t* data = (uint8_t*)Bus_GetListValue(frame, 0);
	if(data)
		JUDGE_Read_Data(data);
}

/**************裁判系统数据辅助****************/

/**
  * @brief  读取裁判数据,中断中读取保证速度
  * @param  缓存数据
  * @retval 是否对正误判断做处理
  * @attention  在此判断帧头和CRC校验,无误再写入数据，不重复判断帧头
  */
bool JUDGE_Read_Data(uint8_t *ReadFromUsart)
{
	bool retval_tf = FALSE;//数据正确与否标志,每次调用读取裁判系统数据函数都先默认为错误
	
	uint16_t judge_length;//统计一帧数据长度 
	
	int CmdID = 0;//数据命令码解析
	
	/***------------------*****/
	//无数据包，则不作任何处理
	if (ReadFromUsart == NULL)
	{
		return -1;
	}
	
	//写入帧头数据,用于判断是否开始存储裁判数据
	memcpy(&FrameHeader, ReadFromUsart, LEN_HEADER);
	
	//判断帧头数据是否为0xA5
	if(ReadFromUsart[ SOF ] == JUDGE_FRAME_HEADER)
	{
		//帧头CRC8校验
		if (Verify_CRC8_Check_Sum( ReadFromUsart, LEN_HEADER ) == TRUE)
		{
			//统计一帧数据长度,用于CR16校验
			judge_length = ReadFromUsart[ DATA_LENGTH ] + LEN_HEADER + LEN_CMDID + LEN_TAIL;;

			//帧尾CRC16校验
			if(Verify_CRC16_Check_Sum(ReadFromUsart,judge_length) == TRUE)
			{
				retval_tf = TRUE;//都校验过了则说明数据可用
				
				CmdID = (ReadFromUsart[6] << 8 | ReadFromUsart[5]);
				//解析数据命令码,将数据拷贝到相应结构体中(注意拷贝数据的长度)
				switch(CmdID)
				{
					case ID_game_state:        			//0x0001
						memcpy(&GameState, (ReadFromUsart + DATA), LEN_game_state);
					break;
					
					case ID_game_result:          		//0x0002
						memcpy(&GameResult, (ReadFromUsart + DATA), LEN_game_result);
					break;
					
					case ID_game_robot_HP:       //0x0003
						memcpy(&GameRobotHP, (ReadFromUsart + DATA), LEN_game_robot_HP);
					break;
					
					case ID_event_data:    				//0x0101
						memcpy(&EventData, (ReadFromUsart + DATA), LEN_event_data);
					break;
					
					case ID_supply_projectile_action:   //0x0102
						memcpy(&SupplyProjectileAction, (ReadFromUsart + DATA), LEN_supply_projectile_action);
					break;
					
					case ID_referee_warning:  //0x0104
						memcpy(&RefereeWarning, (ReadFromUsart + DATA), LEN_referee_warning);
					break;
					
					case ID_dart_remaining_time:  //0x0105
						memcpy(&DartRemainingTime, (ReadFromUsart + DATA), LEN_dart_remaining_time);
					break;
					
					case ID_game_robot_state:      		//0x0201
						memcpy(&GameRobotStat, (ReadFromUsart + DATA), LEN_game_robot_state);
					break;
					
					case ID_power_heat_data:      		//0x0202
						memcpy(&PowerHeatData, (ReadFromUsart + DATA), LEN_power_heat_data);
					break;
					
					case ID_game_robot_pos:      		//0x0203
						memcpy(&GameRobotPos, (ReadFromUsart + DATA), LEN_game_robot_pos);
					break;
					
					case ID_buff_musk:      			//0x0204
						memcpy(&BuffMusk, (ReadFromUsart + DATA), LEN_buff_musk);
					break;
					
					case ID_aerial_robot_energy:      	//0x0205
						memcpy(&AerialRobotEnergy, (ReadFromUsart + DATA), LEN_aerial_robot_energy);
					break;
					
					case ID_robot_hurt:      			//0x0206
						memcpy(&RobotHurt, (ReadFromUsart + DATA), LEN_robot_hurt);
					break;
					
					case ID_shoot_data:      			//0x0207
						memcpy(&ShootData, (ReadFromUsart + DATA), LEN_shoot_data);
						//Vision_SendShootSpeed(ShootData.bullet_speed);
					break;
					
					case ID_bullet_remaining:      			//0x0208
						memcpy(&BulletRemaining, (ReadFromUsart + DATA), LEN_bullet_remaining);
					break;
					
					case ID_rfid_status:      			//0x0209
						memcpy(&RfidStatus, (ReadFromUsart + DATA), LEN_rfid_status);
					break;
					
					case ID_dart_client_cmd:      			//0x020A
						memcpy(&DartClientCmd, (ReadFromUsart + DATA), LEN_dart_client_cmd);
					break;
				}
				//首地址加帧长度,指向CRC16下一字节,用来判断是否为0xA5,用来判断一个数据包是否有多帧数据
				if(*(ReadFromUsart + sizeof(xFrameHeader) + LEN_CMDID + FrameHeader.DataLength + LEN_TAIL) == 0xA5)
				{
					//如果一个数据包出现了多帧数据,则再次读取
					JUDGE_Read_Data(ReadFromUsart + sizeof(xFrameHeader) + LEN_CMDID + FrameHeader.DataLength + LEN_TAIL);
				}
			}
		}
		//首地址加帧长度,指向CRC16下一字节,用来判断是否为0xA5,用来判断一个数据包是否有多帧数据
		if(*(ReadFromUsart + sizeof(xFrameHeader) + LEN_CMDID + FrameHeader.DataLength + LEN_TAIL) == 0xA5)
		{
			//如果一个数据包出现了多帧数据,则再次读取
			JUDGE_Read_Data(ReadFromUsart + sizeof(xFrameHeader) + LEN_CMDID + FrameHeader.DataLength + LEN_TAIL);
		}
	}
	
	if (retval_tf == TRUE)
	{
		Judge_Data_TF = TRUE;//辅助函数用
	}
	else		//只要CRC16校验不通过就为FALSE
	{
		Judge_Data_TF = FALSE;//辅助函数用
	}
	
	return retval_tf;//对数据正误做处理
}

/*
void JUDGE_SendTextStruct(graphic_data_struct_t *textConf,uint8_t text[30],uint8_t len)
{
	JudgeTxFrame txFrame;
	ext_TextData_t textData;
	textData.txFrameHeader.SOF=0xA5;
	textData.txFrameHeader.DataLength=sizeof(ext_student_interactive_header_data_t)+sizeof(ext_client_custom_character_t);
	textData.txFrameHeader.Seq=0;
	memcpy(txFrame.data, &textData.txFrameHeader, sizeof(xFrameHeader));//写入帧头数据
	Append_CRC8_Check_Sum(txFrame.data, sizeof(xFrameHeader));//写入帧头CRC8校验码
	
	textData.CmdID=0x301;//数据帧ID
	textData.dataFrameHeader.data_cmd_id=0x0110;//数据段ID
	textData.dataFrameHeader.send_ID 	 = JUDGE_GetSelfID();//发送者的ID
	textData.dataFrameHeader.receiver_ID = JUDGE_GetClientID();//客户端的ID，只能为发送者机器人对应的客户端
	
	textData.textData.grapic_data_struct=*textConf;
	memcpy(textData.textData.data,text,len);
	
	memcpy(
		txFrame.data+sizeof(xFrameHeader),
		(uint8_t*)&textData.CmdID,
		sizeof(textData.CmdID)+sizeof(textData.dataFrameHeader)+sizeof(textData.textData));
	Append_CRC16_Check_Sum(txFrame.data,sizeof(textData));
		
	txFrame.frameLength=sizeof(textData);
  //Queue_Enqueue(&judgeQueue,&txFrame);
 
}

void JUDGE_SendGraphStruct(graphic_data_struct_t *data)
{
	JudgeTxFrame txFrame;
	ext_GraphData_t graphData;
	graphData.txFrameHeader.SOF=0xA5;
	graphData.txFrameHeader.DataLength=sizeof(ext_student_interactive_header_data_t)+sizeof(ext_client_custom_graphic_single_t);
	graphData.txFrameHeader.Seq=0;
	memcpy(txFrame.data, &graphData.txFrameHeader, sizeof(xFrameHeader));//写入帧头数据
	Append_CRC8_Check_Sum(txFrame.data, sizeof(xFrameHeader));//写入帧头CRC8校验码
	
	graphData.CmdID=0x301;//数据帧ID
	graphData.dataFrameHeader.data_cmd_id=0x0101;//数据段ID
	graphData.dataFrameHeader.send_ID 	 = JUDGE_GetSelfID();//发送者的ID
	graphData.dataFrameHeader.receiver_ID = JUDGE_GetClientID();//客户端的ID，只能为发送者机器人对应的客户端
	
	graphData.graphData.grapic_data_struct=*data;
	
	memcpy(
		txFrame.data+sizeof(xFrameHeader),
		(uint8_t*)&graphData.CmdID,
		sizeof(graphData.CmdID)+sizeof(graphData.dataFrameHeader)+sizeof(graphData.graphData));
	Append_CRC16_Check_Sum(txFrame.data,sizeof(graphData));
		
	txFrame.frameLength=sizeof(graphData);
 // Queue_Enqueue(&judgeQueue,&txFrame);
}

*/
#endif


