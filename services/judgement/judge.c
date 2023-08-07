#include "judge_drive.h"
#include "config.h"
#include "softbus.h"
#include "cmsis_os.h"
#include "my_queue.h"
#include <string.h>

typedef struct _Judge
{
	JudgeRecInfo judgeRecInfo; //从裁判系统接收到的数据
	bool dataTF; //裁判数据是否可用,辅助函数调用
	Queue txQueue; //发送队列
	JudgeTxFrame *queueBuf;
	uint8_t uartX;	 
	uint16_t taskInterval; //任务执行间隔
}Judge;


void Judge_Init(Judge *judge,ConfItem* dict);
void Judge_Recv_SoftBusCallback(const char* topic, SoftBusFrame* frame, void* bindData);
bool Judge_UiDrawTextCallback(const char* topic, SoftBusFrame* frame, void* bindData); //画文本
bool Judge_UiDrawLineCallback(const char* topic, SoftBusFrame* frame, void* bindData); //画直线
bool Judge_UiDrawRectCallback(const char* topic, SoftBusFrame* frame, void* bindData); //画矩形
bool Judge_UiDrawCircleCallback(const char* topic, SoftBusFrame* frame, void* bindData); //画圆
bool Judge_UiDrawOvalCallback(const char* topic, SoftBusFrame* frame, void* bindData); //画椭圆
bool Judge_UiDrawArcCallback(const char* topic, SoftBusFrame* frame, void* bindData);//画圆弧
bool Judge_UiDrawFloatCallback(const char* topic, SoftBusFrame* frame, void* bindData);
bool Judge_UiDrawIntCallback(const char* topic, SoftBusFrame* frame, void* bindData);
bool JUDGE_Read_Data(Judge *judge,uint8_t *ReadFromUsart);
void Judge_publishData(Judge * judge);
void Judge_TimerCallback(void const *argument);

Judge judge={0}; //大内存 不适合作为任务的局部变量
JudgeTxFrame debug;

void Judge_TaskCallback(void const * argument)
{
	//进入临界区
	portENTER_CRITICAL();
	Judge_Init(&judge, (ConfItem*)argument);
	portEXIT_CRITICAL();
	osDelay(2000);
	while (1)
	{
		if(!Queue_IsEmpty(&judge.txQueue))
		{
			//取队头的消息发送
			JudgeTxFrame *txframe=(JudgeTxFrame*)Queue_Dequeue(&judge.txQueue);
			
			//发送ui
			Bus_RemoteCall("/uart/trans/dma",{{"uart-x",&judge.uartX},
			                                  {"data",(uint8_t *)txframe->data},
			                                  {"trans-size",&txframe->frameLength}});
		}
		osDelay(judge.taskInterval);
	}		
}

//初始化
void Judge_Init(Judge* judge,ConfItem* dict)
{
	//初始化发送队列
	uint16_t maxTxQueueLen = Conf_GetValue(dict, "max-tx-queue-length", uint16_t, 20); //最大发送队列
	judge->queueBuf=(JudgeTxFrame*)pvPortMalloc(maxTxQueueLen*sizeof(JudgeTxFrame));
	Queue_Init(&judge->txQueue,maxTxQueueLen);
	Queue_AttachBuffer(&judge->txQueue,judge->queueBuf,sizeof(JudgeTxFrame));
	judge->taskInterval = Conf_GetValue(dict, "task-interval", uint16_t, 150);  //任务执行间隔
	char name[] = "/uart_/recv";
	judge->uartX = Conf_GetValue(dict, "uart-x", uint8_t, 0);
	name[5] = judge->uartX + '0';

	Bus_RegisterReceiver(judge, Judge_Recv_SoftBusCallback, name);
	Bus_RegisterRemoteFunc(judge, Judge_UiDrawTextCallback, "/judge/send/ui/text");
	Bus_RegisterRemoteFunc(judge, Judge_UiDrawLineCallback, "/judge/send/ui/line");
	Bus_RegisterRemoteFunc(judge, Judge_UiDrawRectCallback, "/judge/send/ui/rect");
	Bus_RegisterRemoteFunc(judge, Judge_UiDrawCircleCallback, "/judge/send/ui/circle");
	Bus_RegisterRemoteFunc(judge, Judge_UiDrawOvalCallback, "/judge/send/ui/oval");
	Bus_RegisterRemoteFunc(judge, Judge_UiDrawArcCallback, "/judge/send/ui/arc");
	Bus_RegisterRemoteFunc(judge, Judge_UiDrawFloatCallback, "/judge/send/ui/float");
	Bus_RegisterRemoteFunc(judge, Judge_UiDrawIntCallback, "/judge/send/ui/int");
	//开启软件定时器 定时广播接收到的数据
	osTimerDef(judge, Judge_TimerCallback);
	osTimerStart(osTimerCreate(osTimer(judge), osTimerPeriodic, judge), 20);
}

//系统定时器回调
void Judge_TimerCallback(void const *argument)
{
	Judge *judge =(Judge*)argument;
	Judge_publishData(judge);  //广播接收到的数据
}


void Judge_Recv_SoftBusCallback(const char* topic, SoftBusFrame* frame, void* bindData)
{
	Judge *judge = (Judge*)bindData;
	uint8_t* data = (uint8_t*)Bus_GetListValue(frame, 0);
	if(data)
		judge->dataTF = JUDGE_Read_Data(judge,data);
}


/*
ui的图形名（name）在操作中，作为客户端的唯一索引，有且只有3个字符
各任务广播ui的频率应小于5hz，应仅在数据更新时广播
立即数对应的图形操作
typedef enum _GraphOperation      
{
	Operation_Null=0,
	Operation_Add,
	Operation_Change,
	Operation_Delete
}GraphOperation;
***添加UI时必须先Operation_Add，才能Operation_Change、Operation_Delete

立即数对应的图形颜色
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

*/
bool Judge_UiDrawTextCallback(const char* topic, SoftBusFrame* frame, void* bindData) //画文本
{
	Judge *judge = (Judge*)bindData;
	//                              名字    文本   颜色    宽度    图层         坐标        字体大小 长度 操作方式
	if(!Bus_CheckMapKeys(frame,{"name","text","color","width","layer","start-x","start-y","size","len","opera"}))
		return false;
	graphic_data_struct_t text;
	uint8_t *value=(uint8_t *)Bus_GetMapValue(frame,"text");
	memcpy(text.graphic_name,(uint8_t*)Bus_GetMapValue(frame,"name"),3);
	text.operate_tpye=*(uint8_t*)Bus_GetMapValue(frame,"opera");
	text.graphic_tpye=Shape_Text;
	text.layer=*(uint8_t*)Bus_GetMapValue(frame,"layer");
	text.color=*(uint8_t*)Bus_GetMapValue(frame,"color");
	text.width=*(uint8_t*)Bus_GetMapValue(frame,"width");;
	text.start_x=*(int16_t*)Bus_GetMapValue(frame,"start-x");
	text.start_y=*(int16_t*)Bus_GetMapValue(frame,"start-y");
	text.start_angle=*(uint16_t*)Bus_GetMapValue(frame,"size");
	text.end_angle=*(uint16_t*)Bus_GetMapValue(frame,"len");    
	JudgeTxFrame txframe = JUDGE_PackTextData(judge->judgeRecInfo.GameRobotStat.robot_id,
	                                          0x100+judge->judgeRecInfo.GameRobotStat.robot_id,
	                                          &text,value);
	debug = txframe;
	Queue_Enqueue(&judge->txQueue,&txframe);
	return true;
}

bool Judge_UiDrawLineCallback(const char* topic, SoftBusFrame* frame, void* bindData) //画直线
{
	Judge *judge = (Judge*)bindData;
	//                              名字     颜色    宽度    图层         开始坐标        结束坐标     操作方式    
	if(!Bus_CheckMapKeys(frame,{"name","color","width","layer","start-x","start-y","end-x","end-y","opera"}))
		return false;
	graphic_data_struct_t line;
	memcpy(line.graphic_name,(uint8_t*)Bus_GetMapValue(frame,"name"),3);
	line.operate_tpye = *(uint8_t*)Bus_GetMapValue(frame,"opera");
	line.graphic_tpye = Shape_Line;
	line.color = *(uint8_t*)Bus_GetMapValue(frame,"color");
	line.width = *(uint8_t*)Bus_GetMapValue(frame,"width");
	line.layer = *(uint8_t*)Bus_GetMapValue(frame,"layer");
	line.start_x = *(int16_t*)Bus_GetMapValue(frame,"start-x");
	line.start_y = *(int16_t*)Bus_GetMapValue(frame,"start-y");
	line.end_x = *(int16_t*)Bus_GetMapValue(frame,"end-x");
	line.end_y = *(int16_t*)Bus_GetMapValue(frame,"end-y");  
	JudgeTxFrame txframe = JUDGE_PackGraphData(judge->judgeRecInfo.GameRobotStat.robot_id, //发送者ID
	                                           0x100+judge->judgeRecInfo.GameRobotStat.robot_id, //接收者ID（客户端）
	                                           &line);
	Queue_Enqueue(&judge->txQueue,&txframe);
	return true;
}

bool Judge_UiDrawRectCallback(const char* topic, SoftBusFrame* frame, void* bindData) //画矩形
{
	Judge *judge = (Judge*)bindData;
	//                              名字     颜色    宽度    图层         开始坐标        对角坐标     操作方式    
	if(!Bus_CheckMapKeys(frame,{"name","color","width","layer","start-x","start-y","end-x","end-y","opera"}))
		return false;
	graphic_data_struct_t rect;
	memcpy(rect.graphic_name,(uint8_t*)Bus_GetMapValue(frame,"name"),3);
	rect.operate_tpye = *(uint8_t*)Bus_GetMapValue(frame,"opera");
	rect.graphic_tpye =Shape_Rect;
	rect.color=*(uint8_t*)Bus_GetMapValue(frame,"color");	
	rect.width=*(uint8_t*)Bus_GetMapValue(frame,"width");
	rect.layer=*(uint8_t*)Bus_GetMapValue(frame,"layer");
	rect.start_x=*(int16_t*)Bus_GetMapValue(frame,"start-x");
	rect.start_y=*(int16_t*)Bus_GetMapValue(frame,"start-y");
	rect.end_x=*(int16_t*)Bus_GetMapValue(frame,"end-x");
	rect.end_y=*(int16_t*)Bus_GetMapValue(frame,"end-y");
	JudgeTxFrame txframe =  JUDGE_PackGraphData(judge->judgeRecInfo.GameRobotStat.robot_id,//发送者ID
	                                            0x100+judge->judgeRecInfo.GameRobotStat.robot_id,//接收者ID（客户端）
	                                            &rect);    
	Queue_Enqueue(&judge->txQueue,&txframe);
	return true;
}

bool Judge_UiDrawCircleCallback(const char* topic, SoftBusFrame* frame, void* bindData) //画圆
{
	Judge *judge = (Judge*)bindData;
	//                              名字     颜色    宽度    图层         中心坐标     半径   操作方式 
	if(!Bus_CheckMapKeys(frame,{"name","color","width","layer","cent-x","cent-y","radius","opera"}))
		return false;
	graphic_data_struct_t circle;
	memcpy(circle.graphic_name,(uint8_t*)Bus_GetMapValue(frame,"name"),3);
	circle.operate_tpye = *(uint8_t*)Bus_GetMapValue(frame,"opera");
	circle.graphic_tpye = Shape_Circle;
	circle.layer=*(uint8_t*)Bus_GetMapValue(frame,"layer");
	circle.color=*(uint8_t*)Bus_GetMapValue(frame,"color");
	circle.width=*(uint8_t*)Bus_GetMapValue(frame,"width");;
	circle.start_x=*(int16_t*)Bus_GetMapValue(frame,"cent-x");
	circle.start_y=*(int16_t*)Bus_GetMapValue(frame,"cent-y");
	circle.radius=*(uint16_t*)Bus_GetMapValue(frame,"radius");   
	JudgeTxFrame txframe =  JUDGE_PackGraphData(judge->judgeRecInfo.GameRobotStat.robot_id,//发送者ID
	                                            0x100+judge->judgeRecInfo.GameRobotStat.robot_id,//接收者ID（客户端）
	                                            &circle);    
	Queue_Enqueue(&judge->txQueue,&txframe); 
	return true;
}

bool Judge_UiDrawOvalCallback(const char* topic, SoftBusFrame* frame, void* bindData) //画椭圆
{
	Judge *judge = (Judge*)bindData;
	//                           名字     颜色    宽度    图层         中心坐标           xy半轴长            操作方式 
	if(!Bus_CheckMapKeys(frame,{"name","color","width","layer","cent-x","cent-y","semiaxis-x","semiaxis-y","opera"}))
		return false;
	graphic_data_struct_t oval;
	memcpy(oval.graphic_name,(uint8_t*)Bus_GetMapValue(frame,"name"),3);
	oval.operate_tpye = *(uint8_t*)Bus_GetMapValue(frame,"opera");
	oval.graphic_tpye = Shape_Oval;
	oval.layer=*(uint8_t*)Bus_GetMapValue(frame,"layer");	
	oval.color=*(uint8_t*)Bus_GetMapValue(frame,"color");
	oval.width=*(uint8_t*)Bus_GetMapValue(frame,"width");;
	oval.start_x=*(int16_t*)Bus_GetMapValue(frame,"cent-x");
	oval.start_y=*(int16_t*)Bus_GetMapValue(frame,"cent-y");
	oval.end_x=*(int16_t*)Bus_GetMapValue(frame,"semiaxis-x");
	oval.end_y=*(int16_t*)Bus_GetMapValue(frame,"semiaxis-y");  
	JudgeTxFrame txframe =  JUDGE_PackGraphData(judge->judgeRecInfo.GameRobotStat.robot_id,//发送者ID
	                                            0x100+judge->judgeRecInfo.GameRobotStat.robot_id,//接收者ID（客户端）
	                                            &oval);    
	Queue_Enqueue(&judge->txQueue,&txframe);
	return true;
}

bool Judge_UiDrawArcCallback(const char* topic, SoftBusFrame* frame, void* bindData) //画圆弧
{
	Judge *judge = (Judge*)bindData;
	//                           名字     颜色    宽度    图层         中心坐标           xy半轴长                  起始、终止角度       操作方式
	if(!Bus_CheckMapKeys(frame,{"name","color","width","layer","cent-x","cent-y","semiaxis-x","semiaxis-y","start-angle","end-angle","opera"}))
		return false;
	graphic_data_struct_t arc;
	memcpy(arc.graphic_name,(uint8_t*)Bus_GetMapValue(frame,"name"),3);
	arc.operate_tpye = *(uint8_t*)Bus_GetMapValue(frame,"opera");
	arc.graphic_tpye = Shape_Arc;
	arc.layer=*(uint8_t*)Bus_GetMapValue(frame,"layer");
	arc.color=*(uint8_t*)Bus_GetMapValue(frame,"color");
	arc.width=*(uint8_t*)Bus_GetMapValue(frame,"width");;
	arc.start_x=*(int16_t*)Bus_GetMapValue(frame,"cent-x");
	arc.start_y=*(int16_t*)Bus_GetMapValue(frame,"cent-y");
	arc.end_x=*(int16_t*)Bus_GetMapValue(frame,"semiaxis-x");
	arc.end_y=*(int16_t*)Bus_GetMapValue(frame,"semiaxis-y");
	arc.start_angle=*(int16_t*)Bus_GetMapValue(frame,"start-angle");
	arc.end_angle=*(int16_t*)Bus_GetMapValue(frame,"end-angle");    
	JudgeTxFrame txframe =  JUDGE_PackGraphData(judge->judgeRecInfo.GameRobotStat.robot_id,//发送者ID
	                                            0x100+judge->judgeRecInfo.GameRobotStat.robot_id,//接收者ID（客户端）
	                                            &arc);    
	Queue_Enqueue(&judge->txQueue,&txframe);
	return true;
}

bool Judge_UiDrawFloatCallback(const char* topic, SoftBusFrame* frame, void* bindData)
{
	Judge *judge = (Judge*)bindData;
	//                           名字     颜色    宽度    图层    值          坐标            大小 有效位数 操作方式
	if(!Bus_CheckMapKeys(frame,{"name","color","width","layer","value","start-x","start-y","size","digit","opera"}))
		return false;
	graphic_data_struct_t float_num;
	float value = *(float*)Bus_GetMapValue(frame,"value");
	memcpy(float_num.graphic_name,(uint8_t*)Bus_GetMapValue(frame,"name"),3);
	float_num.operate_tpye=*(uint8_t*)Bus_GetMapValue(frame,"opera");
	float_num.graphic_tpye=Shape_Float;
	float_num.layer=*(uint8_t*)Bus_GetMapValue(frame,"layer");
	float_num.color=*(uint8_t*)Bus_GetMapValue(frame,"color");
	float_num.width=*(uint8_t*)Bus_GetMapValue(frame,"width");;
	float_num.start_x=*(int16_t*)Bus_GetMapValue(frame,"start-x");
	float_num.start_y=*(int16_t*)Bus_GetMapValue(frame,"start-y");
	float_num.start_angle=*(uint16_t*)Bus_GetMapValue(frame,"size");
	float_num.end_angle=*(uint8_t*)Bus_GetMapValue(frame,"digit");
	float_num.radius=((int32_t)(value*1000))&0x3FF;
	float_num.end_x=((int32_t)(value*1000)>>10)&0x7FF;
	float_num.end_y=((int32_t)(value*1000)>>21)&0x7FF;    
	JudgeTxFrame txframe =  JUDGE_PackGraphData(judge->judgeRecInfo.GameRobotStat.robot_id,//发送者ID
	                                            0x100+judge->judgeRecInfo.GameRobotStat.robot_id,//接收者ID（客户端）
	                                            &float_num);    
	Queue_Enqueue(&judge->txQueue,&txframe);
	return true;
}

bool Judge_UiDrawIntCallback(const char* topic, SoftBusFrame* frame, void* bindData)
{
	Judge *judge = (Judge*)bindData;
	//                           名字     颜色    宽度    图层    值          坐标           大小   操作方式
	if(!Bus_CheckMapKeys(frame,{"name","color","width","layer","value","start-x","start-y","size","opera"}))
		return false;
	graphic_data_struct_t int_num;
	int32_t value = *(int32_t*)Bus_GetMapValue(frame,"value");
	memcpy(int_num.graphic_name, (uint8_t*)Bus_GetMapValue(frame,"name"),3);
	int_num.operate_tpye=*(uint8_t*)Bus_GetMapValue(frame,"opera");
	int_num.graphic_tpye=Shape_Int;
	int_num.layer= *(uint8_t*)Bus_GetMapValue(frame,"layer");
	int_num.color= *(uint8_t*)Bus_GetMapValue(frame,"color");
	int_num.width= *(uint8_t*)Bus_GetMapValue(frame,"width");;
	int_num.start_x=*(int16_t*)Bus_GetMapValue(frame,"start-x");
	int_num.start_y=*(int16_t*)Bus_GetMapValue(frame,"start-y");
	int_num.start_angle= *(uint16_t*)Bus_GetMapValue(frame,"size");
	int_num.radius= value&0x3FF;
	int_num.end_x=( value>>10)&0x7FF;
	int_num.end_y=( value>>21)&0x7FF;    
	JudgeTxFrame txframe =  JUDGE_PackGraphData(judge->judgeRecInfo.GameRobotStat.robot_id,//发送者ID
	                                            0x100+judge->judgeRecInfo.GameRobotStat.robot_id,//接收者ID（客户端）
	                                            &int_num);    
	Queue_Enqueue(&judge->txQueue,&txframe);
	return true;
}

//广播接收到的数据
void Judge_publishData(Judge* judge)
{
	// if(!judge->dataTF)
	// 	return;
	//准备带发布的数据
	uint8_t robot_id = judge->judgeRecInfo.GameRobotStat.robot_id; 
	uint8_t robot_color = robot_id<10?RobotColor_Blue:RobotColor_Red;//机器人颜色
	uint16_t chassis_power_limit = judge->judgeRecInfo.GameRobotStat.chassis_power_limit; //底盘功率限制	
	bool isShooterPowerOutput = judge->judgeRecInfo.GameRobotStat.mains_power_shooter_output; //电管发射机构是否断电
	bool isChassisPowerOutput = judge->judgeRecInfo.GameRobotStat.mains_power_chassis_output; //电管底盘是否断电
	float chassis_power = judge->judgeRecInfo.PowerHeatData.chassis_power;	 //底盘功率
	uint16_t chassis_power_buffer = judge->judgeRecInfo.PowerHeatData.chassis_power_buffer; //底盘缓冲
	float bullet_speed = judge->judgeRecInfo.ShootData.bullet_speed; //发射弹丸速度
	//数据发布
	if(robot_id == 1|| robot_id == 101)   //英雄
	{
		uint16_t shooter_id1_42mm_speed_limit = judge->judgeRecInfo.GameRobotStat.shooter_id1_42mm_speed_limit; //42mm弹速上限	
		uint16_t shooter_id1_42mm_cooling_heat = judge->judgeRecInfo.PowerHeatData.shooter_id1_42mm_cooling_heat;	//42mm枪口热量
		uint16_t bullet_remaining_num_42mm = judge->judgeRecInfo.BulletRemaining.bullet_remaining_num_42mm; //42mm剩余弹丸数量
		Bus_BroadcastSend("/judge/recv/robot-state",{{"color",&robot_color},
		                                             {"42mm-speed-limit",&shooter_id1_42mm_speed_limit},
		                                             {"chassis-power-limit",&chassis_power_limit},
		                                             {"is-shooter-power-output",&isShooterPowerOutput},
		                                             {"is-chassis-power-output",&isChassisPowerOutput}
		                                            });
		Bus_BroadcastSend("/judge/recv/power-Heat",{{"chassis-power",&chassis_power},
		                                            {"chassis-power-buffer",&chassis_power_buffer},
		                                            {"42mm-cooling-heat",&shooter_id1_42mm_cooling_heat}
		                                            }); 
		Bus_BroadcastSend("/judge/recv/shoot",{{"bullet-speed",&bullet_speed}});
		Bus_BroadcastSend("/judge/recv/bullet",{ {"42mm-bullet-remain",&bullet_remaining_num_42mm}});
	}
	else //非英雄单位
	{
		uint16_t shooter_id1_17mm_speed_limit = judge->judgeRecInfo.GameRobotStat.shooter_id1_17mm_speed_limit;	//17mm弹速上限	
		uint16_t shooter_id1_17mm_cooling_heat = judge->judgeRecInfo.PowerHeatData.shooter_id1_17mm_cooling_heat; //17mm枪口热量
		uint16_t bullet_remaining_num_17mm = judge->judgeRecInfo.BulletRemaining.bullet_remaining_num_17mm; //17mm剩余弹丸数量
		bool isGimbalPowerOutput = judge->judgeRecInfo.GameRobotStat.mains_power_gimbal_output; //电管云台是否断电
		Bus_BroadcastSend("/judge/recv/robot-state",{{"color",&robot_color},
		                                            {"17mm-speed-limit",&shooter_id1_17mm_speed_limit},
		                                            {"chassis-power-limit",&chassis_power_limit},
		                                            {"is-shooter-power-output",&isShooterPowerOutput},
		                                            {"is-chassis-power-output",&isChassisPowerOutput},
		                                            {"is-gimabal-power-output",&isGimbalPowerOutput}
		                                            });
		Bus_BroadcastSend("/judge/recv/power-Heat",{{"chassis-power",&chassis_power},
		                                            {"chassis-power-buffer",&chassis_power_buffer},
		                                            {"17mm-cooling-heat",&shooter_id1_17mm_cooling_heat}
		                                            }); 
		Bus_BroadcastSend("/judge/recv/shoot",{{"bullet-speed",&bullet_speed}});
		Bus_BroadcastSend("/judge/recv/bullet",{{"17mm-bullet-remain",&bullet_remaining_num_17mm}});
	}	
}
