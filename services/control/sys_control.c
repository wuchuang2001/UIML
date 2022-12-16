#include "cmsis_os.h"
#include "softbus.h"
#include "config.h"
#include "pid.h"

typedef enum
{
  SYS_FOLLOW_MODE,
  SYS_SPIN_MODE,
  SYS_SEPARATE_MODE
}SysCtrlMode;

typedef struct
{
  struct
  {
    float vx,vy,vw;
    float ax,ay;
    float relativeAngle; 
  }chassisData;

  struct
  {
    float yaw,pitch;
  }gimbalData;
  
  uint8_t mode;
  bool rockerCtrl;
  PID rotatePid;
}SysControl;

SysControl sysCtrl={0};

//��������
void Sys_InitInfo(ConfItem *dict);
void Sys_InitReceiver(void);
void Sys_Broadcast(void);

void Sys_Chassis_StopCallback(const char* name, SoftBusFrame* frame, void* bindData);
void Sys_Chassis_MoveCallback(const char* name, SoftBusFrame* frame, void* bindData);
void Sys_Mode_ChangeCallback(const char* name, SoftBusFrame* frame, void* bindData);
void Sys_Gimbal_RotateCallback(const char* name, SoftBusFrame* frame, void* bindData);
void Sys_Shoot_Callback(const char* name, SoftBusFrame* frame, void* bindData);
void Sys_ErrorHandle(void);

//��ʼ��������Ϣ
void Sys_InitInfo(ConfItem *dict)
{
  sysCtrl.mode=Conf_GetValue(dict,"InitMode",uint8_t,SYS_FOLLOW_MODE); //Ĭ�ϸ���ģʽ
  sysCtrl.rockerCtrl=Conf_GetValue(dict,"rockerCtrl",bool,false);  //Ĭ�ϼ������
  PID_Init(&sysCtrl.rotatePid,Conf_GetPtr(dict, "rotatePID", ConfItem)); 
}

//��ʼ������
void Sys_InitReceiver()
{
	Bus_MultiRegisterReceiver(NULL,Sys_Chassis_MoveCallback,{"/rc/key/on-pressing","rc/left-stick"});
	Bus_RegisterReceiver(NULL,Sys_Chassis_StopCallback,"/rc/key/on-up");
	Bus_MultiRegisterReceiver(NULL,Sys_Gimbal_RotateCallback,{"/rc/mouse-move","rc/right-stick","/gimbal/yaw/relative-angle"});	
  Bus_MultiRegisterReceiver(NULL,Sys_Mode_ChangeCallback,{"/rc/key/on-click","rc/switch"});
	Bus_MultiRegisterReceiver(NULL,Sys_Shoot_Callback,{"/rc/key/on-click","/rc/key/on-pressing","rc/wheel",});
  
}

void SYS_CTRL_TaskCallback(void const * argument)
{
  //�����ٽ���
  portENTER_CRITICAL();
  Sys_InitInfo((ConfItem *)argument);
  Sys_InitReceiver();
  portEXIT_CRITICAL();
	while(1)
	{
    if(sysCtrl.mode==SYS_FOLLOW_MODE)//����ģʽ
    {
      PID_SingleCalc(&sysCtrl.rotatePid,0,sysCtrl.chassisData.relativeAngle);
      sysCtrl.chassisData.vw = sysCtrl.rotatePid.output;
    }
    else if(sysCtrl.mode==SYS_SPIN_MODE)//С����ģʽ
    {
      sysCtrl.chassisData.vw = 2;
    }
    else if(sysCtrl.mode==SYS_SEPARATE_MODE)// ����ģʽ
    {
      sysCtrl.chassisData.vw = 0;
    }
    Sys_Broadcast();
		osDelay(10);
	}
}

//���͹㲥
void Sys_Broadcast()
{
  Bus_BroadcastSend("/chassis/move",{{"vx",&sysCtrl.chassisData.vx},
                                     {"vy",&sysCtrl.chassisData.vy},
                                     {"vw",&sysCtrl.chassisData.vw}});
  Bus_BroadcastSend("/chassis/relativeAngle",{{"angle",&sysCtrl.chassisData.relativeAngle}});
  Bus_BroadcastSend("/gimbal",{{"yaw",&sysCtrl.gimbalData.yaw},{"pitch",&sysCtrl.gimbalData.pitch}});
}

//�����˶���ֹͣ�ص�����
void Sys_Chassis_MoveCallback(const char* name, SoftBusFrame* frame, void* bindData)
{
	float speedRatio=0;
	if(!strcmp(name,"rc/key/on-pressing") && !sysCtrl.rockerCtrl) //�������
	{
		if(!Bus_CheckMapKeys(frame,{"combine-key","key"}))
			return;
		if(!strcmp(Bus_GetMapValue(frame,"combine-key"),"none"))  //����
			speedRatio=1; 
		else if(!strcmp(Bus_GetMapValue(frame,"combine-key"),"shift")) //����
			speedRatio=5; 
		else if(!strcmp(Bus_GetMapValue(frame,"combine-key"),"ctrl")) //����
			speedRatio=0.2;
		switch(*(char*)Bus_GetMapValue(frame,"key"))
		{
			case 'A': 
				sysCtrl.chassisData.vx = 200*speedRatio;
				break;
			case 'D': 
				sysCtrl.chassisData.vx = -200*speedRatio;
				break;
			case 'W': 
				sysCtrl.chassisData.vy = 200*speedRatio;
				break;
			case 'S': 
				sysCtrl.chassisData.vy = -200*speedRatio;
				break;
		}
	}
	else if(!strcmp(name,"rc/left-stick") && sysCtrl.rockerCtrl) //ң��������
	{
		if(!Bus_CheckMapKeys(frame,{"x","y"}))
		  return;
		sysCtrl.chassisData.vx = *(int16_t*)Bus_GetMapValue(frame,"x");
		sysCtrl.chassisData.vy = *(int16_t*)Bus_GetMapValue(frame,"y");
	}
}
void Sys_Chassis_StopCallback(const char* name, SoftBusFrame* frame, void* bindData)
{

	if(!Bus_IsMapKeyExist(frame,"key") && sysCtrl.rockerCtrl)
		return;
	switch(*(char*)Bus_GetMapValue(frame,"key"))
	{
		case 'A': 
		case 'D': 
			sysCtrl.chassisData.vx = 0;
			break;
		case 'W': 
		case 'S': 
			sysCtrl.chassisData.vy = 0;
			break;
	}
}

//��̨��ת�ص�����
void Sys_Gimbal_RotateCallback(const char* name, SoftBusFrame* frame, void* bindData)
{
	if(!strcmp(name,"rc/mouse-move") && !sysCtrl.rockerCtrl)  //�������
	{
		if(!Bus_CheckMapKeys(frame,{"x","y"}))
			return;
		sysCtrl.gimbalData.yaw =*(int16_t*)Bus_GetMapValue(frame,"x");
    sysCtrl.gimbalData.pitch =*(int16_t*)Bus_GetMapValue(frame,"y"); 
	}
	else if(!strcmp(name,"rc/right-stick") && sysCtrl.rockerCtrl)  //ң��������
	{
		if(!Bus_CheckMapKeys(frame,{"x","y"}))
		  return;
		sysCtrl.gimbalData.yaw =*(int16_t*)Bus_GetMapValue(frame,"x");
    sysCtrl.gimbalData.pitch =*(int16_t*)Bus_GetMapValue(frame,"y"); 
	}
  else if(!strcmp(name,"/gimbal/yaw/relative-angle"))
  {
    if(!Bus_IsMapKeyExist(frame,"angle"))
      return;
    sysCtrl.chassisData.relativeAngle = *(float*)Bus_GetMapValue(frame,"angle");
  }
}

//ģʽ�л��ص�
void Sys_Mode_ChangeCallback(const char* name, SoftBusFrame* frame, void* bindData)
{
  if(!strcmp(name,"/rc/key/on-click") && !sysCtrl.rockerCtrl)  //�������
	{
    if(!Bus_IsMapKeyExist(frame,"key"))
      return;
    switch(*(char*)Bus_GetMapValue(frame,"key"))
    {
      case 'Q':  
        sysCtrl.mode = SYS_SPIN_MODE;  //С����ģʽ
        break;
      case 'E':  
        sysCtrl.mode = SYS_FOLLOW_MODE;  //����ģʽ
        break;
      case 'R':
        sysCtrl.mode = SYS_SEPARATE_MODE; //����ģʽ
        break;
    }
  }
  else if(!strcmp(name,"rc/switch") && sysCtrl.rockerCtrl)  //ң��������
  {
    if(!Bus_IsMapKeyExist(frame,"right"))
      return;
    switch(*(uint8_t*)Bus_GetMapValue(frame,"right"))
    {
      case 1:
        sysCtrl.mode = SYS_SPIN_MODE; //С����ģʽ
        break;                        
      case 2:                         
        sysCtrl.mode = SYS_FOLLOW_MODE;  //����ģʽ
        break;                        
      case 3:                         
        sysCtrl.mode = SYS_SEPARATE_MODE; //����ģʽ
        break;
    }
  }
	else if(!strcmp(name,"rc/switch"))
	{
		if(!Bus_IsMapKeyExist(frame,"left"))
			return;
    switch(*(uint8_t*)Bus_GetMapValue(frame,"left"))
    {
      case 1:
        sysCtrl.rockerCtrl = true; //�л���ң��������
        break;
      case 2:
        sysCtrl.rockerCtrl = false; //�л����������
        break;
      case 3:   
        Sys_ErrorHandle();  //��ͣ
        break;
    }
  }
}

//����ص�����
void Sys_Shoot_Callback(const char* name, SoftBusFrame* frame, void* bindData)
{
  if(!strcmp(name,"/rc/key/on-click") && !sysCtrl.rockerCtrl)//�������
  {
    if(!Bus_IsMapKeyExist(frame,"left"))
      return;
    Bus_BroadcastSend("/shooter",{{"onec",IM_PTR(uint8_t,1)}});  //����
  }
  else if(!strcmp(name,"/rc/key/on-pressing") && !sysCtrl.rockerCtrl)
  {
    if(!Bus_IsMapKeyExist(frame,"left"))
      return;
    Bus_BroadcastSend("/shooter",{{"continue",IM_PTR(uint8_t,1)},{"num",IM_PTR(uint8_t,1)}}); //����
  }
  
  else if(!strcmp(name,"rc/wheel") && sysCtrl.rockerCtrl)//ң��������
  {
    if(!Bus_IsMapKeyExist(frame,"value"))
      return;
    int16_t wheel = *(int16_t*)Bus_GetMapValue(frame,"value");
    
    if(wheel > 600)
      Bus_BroadcastSend("/shooter",{{"once",IM_PTR(uint8_t,1)}}); //����
    else if(wheel < -600)
      Bus_BroadcastSend("/shooter",{{"continue",IM_PTR(uint8_t,1)},{"num",IM_PTR(uint8_t,1)}}); //����
  }
}

//��ͣ����
void Sys_ErrorHandle()
{
  uint8_t data[8]={0};
  Bus_BroadcastSend("/can/send-once",{{"can-x",&((uint8_t){1})},{"id",&((uint16_t){0x200})}, {"data", data}});
  Bus_BroadcastSend("/can/send-once",{{"can-x",&((uint8_t){1})},{"id",&((uint16_t){0x1FF})}, {"data", data}});
  Bus_BroadcastSend("/can/send-once",{{"can-x",&((uint8_t){2})},{"id",&((uint16_t){0x200})}, {"data", data}});
  Bus_BroadcastSend("/can/send-once",{{"can-x",&((uint8_t){2})},{"id",&((uint16_t){0x1FF})}, {"data", data}});
  while(1);
}
