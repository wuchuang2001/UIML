#include "judge_drive.h"
#include "string.h"
#include "crc_dji.h"
/**************裁判系统数据辅助****************/

/**
  * @brief  读取裁判数据,中断中读取保证速度
  * @param  缓存数据
  * @retval 是否对正误判断做处理
  * @attention  在此判断帧头和CRC校验,无误再写入数据，不重复判断帧头
  */
bool JUDGE_Read_Data(JudgeRecInfo *judge,uint8_t *ReadFromUsart)
{
	uint16_t judge_length;//统计一帧数据长度 
	
	int CmdID = 0;//数据命令码解析
	
	/***------------------*****/
	//无数据包，则不作任何处理
	if (ReadFromUsart == NULL)
	{
		return false;
	}
	
	//写入帧头数据,用于判断是否开始存储裁判数据
	memcpy(&judge->FrameHeader, ReadFromUsart, LEN_HEADER);
	
	//判断帧头数据是否为0xA5
	if(ReadFromUsart[ SOF ] == JUDGE_FRAME_HEADER)
	{
		//帧头CRC8校验
		if (!Verify_CRC8_Check_Sum( ReadFromUsart, LEN_HEADER ))
			return false;
		//统计一帧数据长度,用于CR16校验
		judge_length = ReadFromUsart[ DATA_LENGTH ] + LEN_HEADER + LEN_CMDID + LEN_TAIL;

		//帧尾CRC16校验
		if(!Verify_CRC16_Check_Sum(ReadFromUsart,judge_length))
			return false;
		
		CmdID = (ReadFromUsart[6] << 8 | ReadFromUsart[5]);
		//解析数据命令码,将数据拷贝到相应结构体中(注意拷贝数据的长度)
		switch(CmdID)
		{
			case ID_game_state:                 //0x0001
				memcpy(&judge->GameState, (ReadFromUsart + DATA), LEN_game_state);
			break;
			
			case ID_game_result:                //0x0002
				memcpy(&judge->GameResult, (ReadFromUsart + DATA), LEN_game_result);
			break;
			
			case ID_game_robot_HP:              //0x0003
				memcpy(&judge->GameRobotHP, (ReadFromUsart + DATA), LEN_game_robot_HP);
			break;
			
			case ID_event_data:                 //0x0101
				memcpy(&judge->EventData, (ReadFromUsart + DATA), LEN_event_data);
			break;
			
			case ID_supply_projectile_action:   //0x0102
				memcpy(&judge->SupplyProjectileAction, (ReadFromUsart + DATA), LEN_supply_projectile_action);
			break;
			
			case ID_referee_warning:            //0x0104
				memcpy(&judge->RefereeWarning, (ReadFromUsart + DATA), LEN_referee_warning);
			break;
			
			case ID_dart_remaining_time:        //0x0105
				memcpy(&judge->DartRemainingTime, (ReadFromUsart + DATA), LEN_dart_remaining_time);
			break;
			
			case ID_game_robot_state:           //0x0201
				memcpy(&judge->GameRobotStat, (ReadFromUsart + DATA), LEN_game_robot_state);
			break;
			
			case ID_power_heat_data:            //0x0202
				memcpy(&judge->PowerHeatData, (ReadFromUsart + DATA), LEN_power_heat_data);
			break;
			
			case ID_game_robot_pos:             //0x0203
				memcpy(&judge->GameRobotPos, (ReadFromUsart + DATA), LEN_game_robot_pos);
			break;
			
			case ID_buff_musk:                  //0x0204
				memcpy(&judge->BuffMusk, (ReadFromUsart + DATA), LEN_buff_musk);
			break;
			
			case ID_aerial_robot_energy:      	//0x0205
				memcpy(&judge->AerialRobotEnergy, (ReadFromUsart + DATA), LEN_aerial_robot_energy);
			break;
			
			case ID_robot_hurt:                 //0x0206
				memcpy(&judge->RobotHurt, (ReadFromUsart + DATA), LEN_robot_hurt);
			break;
			
			case ID_shoot_data:                 //0x0207
				memcpy(&judge->ShootData, (ReadFromUsart + DATA), LEN_shoot_data);
				//Vision_SendShootSpeed(ShootData.bullet_speed);
			break;
			
			case ID_bullet_remaining:           //0x0208
				memcpy(&judge->BulletRemaining, (ReadFromUsart + DATA), LEN_bullet_remaining);
			break;
			
			case ID_rfid_status:                //0x0209
				memcpy(&judge->RfidStatus, (ReadFromUsart + DATA), LEN_rfid_status);
			break;
			
			case ID_dart_client_cmd:            //0x020A
				memcpy(&judge->DartClientCmd, (ReadFromUsart + DATA), LEN_dart_client_cmd);
			break;
		}
		//首地址加帧长度,指向CRC16下一字节,用来判断是否为0xA5,用来判断一个数据包是否有多帧数据
		if(*(ReadFromUsart + sizeof(xFrameHeader) + LEN_CMDID + judge->FrameHeader.DataLength + LEN_TAIL) == 0xA5)
		{
			//如果一个数据包出现了多帧数据,则再次读取
			JUDGE_Read_Data(judge,ReadFromUsart + sizeof(xFrameHeader) + LEN_CMDID + judge->FrameHeader.DataLength + LEN_TAIL);
		}
		return true;//都校验过了则说明数据可用
	}	
	return false;//帧头有问题
}

//打包文本数据，为文本数据添加帧头，校验码等信息，形成一个完整的帧
JudgeTxFrame JUDGE_PackTextData(uint8_t sendID,uint8_t receiveID,graphic_data_struct_t *textConf,uint8_t text[30])
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
	textData.dataFrameHeader.send_ID 	 = sendID;//发送者的ID
	textData.dataFrameHeader.receiver_ID = receiveID; //接收者的ID
	textData.textData.grapic_data_struct=*textConf;
	memcpy(textData.textData.data,text,30);
	
	memcpy(
		txFrame.data+sizeof(xFrameHeader),
		(uint8_t*)&textData.CmdID,
		sizeof(textData.CmdID)+sizeof(textData.dataFrameHeader)+sizeof(textData.textData));
	Append_CRC16_Check_Sum(txFrame.data,sizeof(textData));
		
	txFrame.frameLength=sizeof(textData);
  return txFrame;
}
//打包图像数据，为图像数据添加帧头，校验码等信息，形成一个完整的帧
JudgeTxFrame JUDGE_PackGraphData(uint8_t sendID,uint8_t receiveID,graphic_data_struct_t *data)
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
	graphData.dataFrameHeader.send_ID 	 = sendID;//发送者的ID
	graphData.dataFrameHeader.receiver_ID = receiveID;//客户端的ID，只能为发送者机器人对应的客户端
	
	graphData.graphData.grapic_data_struct=*data;
	
	memcpy(
		txFrame.data+sizeof(xFrameHeader),
		(uint8_t*)&graphData.CmdID,
		sizeof(graphData.CmdID)+sizeof(graphData.dataFrameHeader)+sizeof(graphData.graphData));
	Append_CRC16_Check_Sum(txFrame.data,sizeof(graphData));
		
	txFrame.frameLength=sizeof(graphData);
  return txFrame;
}


