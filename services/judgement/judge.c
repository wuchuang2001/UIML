#include "judge_drive.h"
#include "vector.h"
#include "config.h"
#include "softbus.h"
#include "cmsis_os.h"


typedef struct _Judge
{
	JudgeRecInfo judgeRecInfo; //�Ӳ���ϵͳ���յ�������
	bool dataTF; //���������Ƿ����,������������
  Vector stableUI;   //����UI
  Vector variableUI;  //�ɱ�UI
	uint8_t uartX;	
  uint16_t taskInterval;
}Judge;


void Judge_Init(Judge *judge,ConfItem* dict);
void Judge_Recv_SoftBusCallback(const char* topic, SoftBusFrame* frame, void* bindData);
void Judge_UI_SoftBusCallback(const char* topic, SoftBusFrame* frame, void* bindData);
bool JUDGE_Read_Data(Judge *judge,uint8_t *ReadFromUsart);
void Judge_publishData(Judge * judge);
void Judge_TimerCallback(void const *argument);

Judge judge={0}; //���ڴ� ���ʺ���Ϊ����ľֲ�����

void Judge_TaskCallback(void const * argument)
{
	//�����ٽ���
	portENTER_CRITICAL();
	Judge_Init(&judge, (ConfItem*)argument);
	portEXIT_CRITICAL();
	osDelay(2000);
	TickType_t tick = xTaskGetTickCount();
	while (1)
	{
		static uint32_t i=0;
    JudgeTxFrame txframe[JUDGE_MAX_TX_LENGTH];
    Bus_BroadcastSend("/uart/trans/dma",{{"uart-x",&judge.uartX},{"data",txframe[i].data},{"transSize",&txframe[i].frameLength}});
    i++;
		osDelayUntil(&tick,judge.taskInterval);
	}		
}

void Judge_Init(Judge* judge,ConfItem* dict)
{
  Vector_Init(judge->stableUI,JudgeTxFrame);
  Vector_Init(judge->variableUI,JudgeTxFrame);
  judge->taskInterval = Conf_GetValue(dict, "taskInterval", uint16_t, 150);
	char name[] = "/uart_/recv";
	judge->uartX = Conf_GetValue(dict, "uart-x", uint8_t, 0);
	name[5] = judge->uartX + '0';
	Bus_RegisterReceiver(judge, Judge_Recv_SoftBusCallback, name);
  Bus_MultiRegisterReceiver(judge,Judge_UI_SoftBusCallback,{"judge/send/ui/text",
                                                            "judge/send/ui/line",
                                                            "judge/send/ui/rect",
                                                            "judge/send/ui/circle",
                                                            "judge/send/ui/oval",
                                                            "judge/send/ui/arc",
                                                            "judge/send/ui/float",
                                                            "judge/send/ui/int"});
	//����������ʱ����ʱ����
	osTimerDef(judge, Judge_TimerCallback);
	osTimerStart(osTimerCreate(osTimer(judge), osTimerPeriodic, judge), 20);
}

void Judge_Recv_SoftBusCallback(const char* topic, SoftBusFrame* frame, void* bindData)
{
	Judge *judge = (Judge*)bindData;
	uint8_t* data = (uint8_t*)Bus_GetListValue(frame, 0);
	if(data)
		judge->dataTF = JUDGE_Read_Data(judge,data);
}


  
void Judge_UI_SoftBusCallback(const char* topic, SoftBusFrame* frame, void* bindData)
{
  Judge *judge = (Judge*)bindData;
  
  if(!strcmp(topic, "judge/send/ui/text"))
	{
    if(!Bus_CheckMapKeys(frame,{"name","color","width","layer","start_x","start_y","size","len"}))
      return;
    graphic_data_struct_t text;
    uint8_t *value=(uint8_t *)Bus_GetMapValue(frame,"text");
    memcpy(text.graphic_name,(uint8_t*)Bus_GetMapValue(frame,"name"),3);
    text.operate_tpye=*(uint8_t*)Bus_GetMapValue(frame,"opera");
    text.graphic_tpye=Shape_Text;
    text.layer=*(uint8_t*)Bus_GetMapValue(frame,"layer");
    text.color=*(uint8_t*)Bus_GetMapValue(frame,"color");
    text.width=*(uint8_t*)Bus_GetMapValue(frame,"width");;
    text.start_x=*(uint8_t*)Bus_GetMapValue(frame,"start_x");
    text.start_y=*(uint8_t*)Bus_GetMapValue(frame,"start_y");
    text.start_angle=*(uint8_t*)Bus_GetMapValue(frame,"size");
    text.end_angle=*(uint8_t*)Bus_GetMapValue(frame,"len");    
    JUDGE_SendTextStruct(judge->judgeRecInfo.GameRobotStat.robot_id,
                          0x100+judge->judgeRecInfo.GameRobotStat.robot_id,
                          &text,value);
  }
  else if(!strcmp(topic, "judge/send/ui/line"))
  {
    if(!Bus_CheckMapKeys(frame,{"name","color","width","layer","start_x","start_y","end_x","end_y"}))
      return;
    graphic_data_struct_t line;
    memcpy(line.graphic_name,(uint8_t*)Bus_GetMapValue(frame,"name"),3);
    line.operate_tpye = *(uint8_t*)Bus_GetMapValue(frame,"opera");
    line.graphic_tpye = Shape_Line;
    line.color = *(uint8_t*)Bus_GetMapValue(frame,"color");
    line.width = *(uint8_t*)Bus_GetMapValue(frame,"width");
    line.layer = *(uint8_t*)Bus_GetMapValue(frame,"layer");
    line.start_x = *(uint8_t*)Bus_GetMapValue(frame,"start_x");
    line.start_y = *(uint8_t*)Bus_GetMapValue(frame,"start_y");
    line.end_x = *(uint8_t*)Bus_GetMapValue(frame,"end_x");
    line.end_y = *(uint8_t*)Bus_GetMapValue(frame,"end_y");  
    JUDGE_SendGraphStruct(judge->judgeRecInfo.GameRobotStat.robot_id,
                          0x100+judge->judgeRecInfo.GameRobotStat.robot_id,
                          &line);
  }
  else if(!strcmp(topic, "judge/send/ui/rect"))
  {
    if(!Bus_CheckMapKeys(frame,{"name","color","width","layer","start_x","start_y","end_x","end_y"}))
      return;
    graphic_data_struct_t rect;
    memcpy(rect.graphic_name,(uint8_t*)Bus_GetMapValue(frame,"name"),3);
    rect.operate_tpye = *(uint8_t*)Bus_GetMapValue(frame,"opera");
    rect.graphic_tpye =Shape_Rect;
    rect.color=*(uint8_t*)Bus_GetMapValue(frame,"color");	
    rect.width=*(uint8_t*)Bus_GetMapValue(frame,"width");
    rect.layer=*(uint8_t*)Bus_GetMapValue(frame,"layer");
    rect.start_x=*(uint8_t*)Bus_GetMapValue(frame,"start_x");
    rect.start_y=*(uint8_t*)Bus_GetMapValue(frame,"start_y");
    rect.end_x=*(uint8_t*)Bus_GetMapValue(frame,"end_x");
    rect.end_y=*(uint8_t*)Bus_GetMapValue(frame,"end_y");
    JUDGE_SendGraphStruct(judge->judgeRecInfo.GameRobotStat.robot_id,
                          0x100+judge->judgeRecInfo.GameRobotStat.robot_id,
                          &rect);    
      
  }
  else if(!strcmp(topic, "judge/send/ui/circle"))
  {
    if(!Bus_CheckMapKeys(frame,{"name","color","width","layer","cent_x","cent_y","radius"}))
      return;
    graphic_data_struct_t circle;
    memcpy(circle.graphic_name,(uint8_t*)Bus_GetMapValue(frame,"name"),3);
    circle.operate_tpye = *(uint8_t*)Bus_GetMapValue(frame,"opera");
    circle.graphic_tpye = Shape_Circle;
    circle.layer=*(uint8_t*)Bus_GetMapValue(frame,"layer");
    circle.color=*(uint8_t*)Bus_GetMapValue(frame,"color");
    circle.width=*(uint8_t*)Bus_GetMapValue(frame,"width");;
    circle.start_x=*(uint8_t*)Bus_GetMapValue(frame,"cent_x");
    circle.start_y=*(uint8_t*)Bus_GetMapValue(frame,"cent_y");
    circle.radius=*(uint8_t*)Bus_GetMapValue(frame,"radius");    
  }
  else if(!strcmp(topic, "judge/send/ui/oval"))
  {
    if(!Bus_CheckMapKeys(frame,{"name","color","width","layer","cent_x","cent_y","semiaxis_x","semiaxis_y"}))
      return;
    graphic_data_struct_t oval;
    memcpy(oval.graphic_name,(uint8_t*)Bus_GetMapValue(frame,"name"),3);
    oval.operate_tpye = *(uint8_t*)Bus_GetMapValue(frame,"opera");
    oval.graphic_tpye = Shape_Oval;
    oval.layer=*(uint8_t*)Bus_GetMapValue(frame,"layer");	
    oval.color=*(uint8_t*)Bus_GetMapValue(frame,"color");
    oval.width=*(uint8_t*)Bus_GetMapValue(frame,"width");;
    oval.start_x=*(uint8_t*)Bus_GetMapValue(frame,"cent_x");
    oval.start_y=*(uint8_t*)Bus_GetMapValue(frame,"cent_y");
    oval.end_x=*(uint8_t*)Bus_GetMapValue(frame,"semiaxis_x");
    oval.end_y=*(uint8_t*)Bus_GetMapValue(frame,"semiaxis_y");    
  }
  else if(!strcmp(topic, "judge/send/ui/arc"))
  {
    if(!Bus_CheckMapKeys(frame,{"name","color","width","layer","cent_x","cent_y","semiaxis_x","semiaxis_y","start_angle","end_angle"}))
      return;
    graphic_data_struct_t arc;
    memcpy(arc.graphic_name,(uint8_t*)Bus_GetMapValue(frame,"name"),3);
    arc.operate_tpye = *(uint8_t*)Bus_GetMapValue(frame,"opera");
    arc.graphic_tpye = Shape_Arc;
    arc.layer=*(uint8_t*)Bus_GetMapValue(frame,"layer");
    arc.color=*(uint8_t*)Bus_GetMapValue(frame,"color");
    arc.width=*(uint8_t*)Bus_GetMapValue(frame,"width");;
    arc.start_x=*(uint8_t*)Bus_GetMapValue(frame,"cent_x");
    arc.start_y=*(uint8_t*)Bus_GetMapValue(frame,"cent_y");
    arc.end_x=*(uint8_t*)Bus_GetMapValue(frame,"semiaxis_x");
    arc.end_y=*(uint8_t*)Bus_GetMapValue(frame,"semiaxis_y");
    arc.start_angle=*(uint8_t*)Bus_GetMapValue(frame,"start_angle");
    arc.end_angle=*(uint8_t*)Bus_GetMapValue(frame,"end_angle");    
  }
  else if(!strcmp(topic, "judge/send/ui/float"))
  {
    if(!Bus_CheckMapKeys(frame,{"name","color","width","layer","value","start_x","start_y","size","digit"}))
      return;
    graphic_data_struct_t float_num;
    float value = *(float*)Bus_GetMapValue(frame,"value");
    memcpy(float_num.graphic_name,(uint8_t*)Bus_GetMapValue(frame,"name"),3);
    float_num.operate_tpye=*(uint8_t*)Bus_GetMapValue(frame,"opera");
    float_num.graphic_tpye=Shape_Float;
    float_num.layer=*(uint8_t*)Bus_GetMapValue(frame,"layer");
    float_num.color=*(uint8_t*)Bus_GetMapValue(frame,"color");
    float_num.width=*(uint8_t*)Bus_GetMapValue(frame,"width");;
    float_num.start_x=*(uint8_t*)Bus_GetMapValue(frame,"start_x");
    float_num.start_y=*(uint8_t*)Bus_GetMapValue(frame,"start_y");
    float_num.start_angle=*(uint8_t*)Bus_GetMapValue(frame,"size");
    float_num.end_angle=*(uint8_t*)Bus_GetMapValue(frame,"digit");
    float_num.radius=((int32_t)(value*1000))&0x3FF;
    float_num.end_x=((int32_t)(value*1000)>>10)&0x7FF;
    float_num.end_y=((int32_t)(value*1000)>>21)&0x7FF;    
  }
  else if(!strcmp(topic, "judge/send/ui/int"))
  {
    if(!Bus_CheckMapKeys(frame,{"name","color","width","layer","value","start_x","start_y","size"}))
      return;
    graphic_data_struct_t int_num;
    int32_t value = *(int32_t*)Bus_GetMapValue(frame,"value");
    memcpy(int_num.graphic_name, (uint8_t*)Bus_GetMapValue(frame,"name"),3);
    int_num.operate_tpye=*(uint8_t*)Bus_GetMapValue(frame,"opera");
    int_num.graphic_tpye=Shape_Int;
    int_num.layer= *(uint8_t*)Bus_GetMapValue(frame,"layer");
    int_num.color= *(uint8_t*)Bus_GetMapValue(frame,"color");
    int_num.width= *(uint8_t*)Bus_GetMapValue(frame,"width");;
    int_num.start_x=*(uint8_t*)Bus_GetMapValue(frame,"start_x");
    int_num.start_y=*(uint8_t*)Bus_GetMapValue(frame,"start_y");
    int_num.start_angle= *(uint8_t*)Bus_GetMapValue(frame,"size");
    int_num.radius= value&0x3FF;
    int_num.end_x=( value>>10)&0x7FF;
    int_num.end_y=( value>>21)&0x7FF;    
  }
}
void Judge_publishData(Judge* judge)
{
	// if(!judge->dataTF)
	// 	return;
	//׼��������������
	uint8_t robot_id = judge->judgeRecInfo.GameRobotStat.robot_id; 
	uint8_t robot_color = robot_id<10?RobotColor_Blue:RobotColor_Red;//��������ɫ
	uint16_t chassis_power_limit = judge->judgeRecInfo.GameRobotStat.chassis_power_limit; //���̹�������	
	bool isShooterPowerOutput = judge->judgeRecInfo.GameRobotStat.mains_power_shooter_output; //��ܷ�������Ƿ�ϵ�
	bool isChassisPowerOutput = judge->judgeRecInfo.GameRobotStat.mains_power_chassis_output; //��ܵ����Ƿ�ϵ�
	float chassis_power = judge->judgeRecInfo.PowerHeatData.chassis_power;	 //���̹���
	uint16_t chassis_power_buffer = judge->judgeRecInfo.PowerHeatData.chassis_power_buffer; //���̻���
	float bullet_speed = judge->judgeRecInfo.ShootData.bullet_speed; //���䵯���ٶ�
	//���ݷ���
	if(robot_id == 1|| robot_id == 101)   //Ӣ��
	{
		uint16_t shooter_id1_42mm_speed_limit = judge->judgeRecInfo.GameRobotStat.shooter_id1_42mm_speed_limit; //42mm��������	
		uint16_t shooter_id1_42mm_cooling_heat = judge->judgeRecInfo.PowerHeatData.shooter_id1_42mm_cooling_heat;	//42mmǹ������
		uint16_t bullet_remaining_num_42mm = judge->judgeRecInfo.BulletRemaining.bullet_remaining_num_42mm; //42mmʣ�൯������
		Bus_BroadcastSend("/judge/recv/robot-state",{{"color",&robot_color},
		                                             {"42mm-speed-limit",&shooter_id1_42mm_speed_limit},
		                                             {"chassis-power-limit",&chassis_power_limit},
		                                             {"is-shooter-power-output",&isShooterPowerOutput},
		                                             {"is-chassis-power-output",&isChassisPowerOutput}
		                                            });
		Bus_BroadcastSend("/judge/recv/power-Heat",{{"chassis-power",&chassis_power},
		                                            {"chassis-power_buffer",&chassis_power_buffer},
		                                            {"42mm-cooling-heat",&shooter_id1_42mm_cooling_heat}
		                                            }); 
		Bus_BroadcastSend("/judge/recv/shoot",{{"bullet-speed",&bullet_speed}});
		Bus_BroadcastSend("/judge/recv/bullet",{ {"42mm-bullet-remain",&bullet_remaining_num_42mm}});
	}
	else //��Ӣ�۵�λ
	{
		uint16_t shooter_id1_17mm_speed_limit = judge->judgeRecInfo.GameRobotStat.shooter_id1_17mm_speed_limit;	//17mm��������	
		uint16_t shooter_id1_17mm_cooling_heat = judge->judgeRecInfo.PowerHeatData.shooter_id1_17mm_cooling_heat; //17mmǹ������
		uint16_t bullet_remaining_num_17mm = judge->judgeRecInfo.BulletRemaining.bullet_remaining_num_17mm; //17mmʣ�൯������
		bool isGimbalPowerOutput = judge->judgeRecInfo.GameRobotStat.mains_power_gimbal_output; //�����̨�Ƿ�ϵ�
		Bus_BroadcastSend("/judge/recv/robot-state",{{"color",&robot_color},
		                                            {"17mm-speed-limit",&shooter_id1_17mm_speed_limit},
		                                            {"chassis-power-limit",&chassis_power_limit},
		                                            {"is-shooter-power-output",&isShooterPowerOutput},
		                                            {"is-chassis-power-output",&isChassisPowerOutput},
		                                            {"is-gimabal-power-output",&isGimbalPowerOutput}
		                                            });
		Bus_BroadcastSend("/judge/recv/power-Heat",{{"chassis-power",&chassis_power},
		                                            {"chassis-power_buffer",&chassis_power_buffer},
		                                            {"17mm-cooling-heat",&shooter_id1_17mm_cooling_heat}
		                                            }); 
		Bus_BroadcastSend("/judge/recv/shoot",{{"bullet-speed",&bullet_speed}});
		Bus_BroadcastSend("/judge/recv/bullet",{{"17mm-bullet-remain",&bullet_remaining_num_17mm}});
	}	
}

//ϵͳ��ʱ���ص�
/************����UI***************/
void Judge_TimerCallback(void const *argument)
{
  Judge *judge =(Judge*)argument;
  Judge_publishData(judge);

}






