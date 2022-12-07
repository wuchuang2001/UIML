#ifndef _SYSCONF_H_
#define _SYSCONF_H_

#include "config.h"

//<<< Use Configuration Wizard in Context Menu >>>
//<h>BSP Config
//<q0>CAN
//<i>Select to include "can.h"
//<q1>UART
//<i>Select to include "usart.h"
//<q1>EXIT
//<i>Select to include "gpio.h"
#define CONF_CAN_ENABLE		1
#define CONF_USART_ENABLE	1
#define CONF_EXIT_ENABLE 1
//</h>
#if CONF_CAN_ENABLE
#include "can.h"
#endif
#if CONF_USART_ENABLE
#include "usart.h"
#endif
#if CONF_EXIT_ENABLE
#include "gpio.h"
#endif
// <<< end of configuration section >>>

//���������б���ÿ���ʽ(������,����������,�������ȼ�,����ջ��С)
#define SERVICE_LIST \
	SERVICE(chassis, Chassis_TaskCallback, osPriorityNormal,256) \
	SERVICE(can, BSP_CAN_TaskCallback, osPriorityRealtime,128) \
	SERVICE(rc, RC_TaskCallback, osPriorityNormal,256)         \
	SERVICE(uart, BSP_UART_TaskCallback, osPriorityNormal,256) \
	SERVICE(exti, BSP_EXTI_TaskCallback, osPriorityNormal,256) 
	
//������������
ConfItem* systemConfig = CF_DICT{
	//���̷�������
	{"chassis", CF_DICT{
		//����ѭ������
		{"taskInterval", IM_PTR(uint8_t, 2)},
		//���̳ߴ���Ϣ
		{"info", CF_DICT{
			{"wheelbase", IM_PTR(float, 100)},
			{"wheeltrack", IM_PTR(float, 100)},
			{"wheelRadius", IM_PTR(float, 76)},
			{"offsetX", IM_PTR(float, 0)},
			{"offsetY", IM_PTR(float, 0)},
			CF_DICT_END
		}},
		//�����ƶ��ٶ�/���ٶ�����
		{"move", CF_DICT{
			{"maxVx", IM_PTR(float, 2000)},
			{"maxVy", IM_PTR(float, 2000)},
			{"maxVw", IM_PTR(float, 2)},
			{"xAcc", IM_PTR(float, 1000)},
			{"yAcc", IM_PTR(float, 1000)},
			CF_DICT_END
		}},
		//�ĸ��������
		{"motorFL", CF_DICT{
			{"type", "M3508"},
			{"id", IM_PTR(uint16_t, 1)},
			{"canX", IM_PTR(uint8_t, 1)},
//			{"reductionRatio", IM_PTR(float, 1)},   //��ʹ�ø�װ��������߲��������ĵ�����޸Ĵ˲�������ʹ��ԭװ������������ô˲���
			{"speedPID", CF_DICT{
				{"p", IM_PTR(float, 10)},
				{"i", IM_PTR(float, 1)},
				{"d", IM_PTR(float, 0)},
				{"maxI", IM_PTR(float, 10000)},
				{"maxOut", IM_PTR(float, 20000)},
				CF_DICT_END
			}},
//			{"anglePID", CF_DICT{                  //����pidʾ��������ʹ�ô���pid�մ�ģ�����ü���
//				{"inner", CF_DICT{
//					{"p", IM_PTR(float, 10)},
//					{"i", IM_PTR(float, 1)},
//					{"d", IM_PTR(float, 0)},
//					{"maxI", IM_PTR(float, 10000)},
//					{"maxOut", IM_PTR(float, 20000)},
//					CF_DICT_END
//				}},
//				{"outer", CF_DICT{
//					{"p", IM_PTR(float, 0.5)},
//					{"i", IM_PTR(float, 0)},
//					{"d", IM_PTR(float, 0)},
//					{"maxI", IM_PTR(float, 25)},
//					{"maxOut", IM_PTR(float, 50)},
//					CF_DICT_END
//				}},
//				CF_DICT_END
//			}},
			CF_DICT_END
		}},
		{"motorFR", CF_DICT{
			{"type", "M3508"},
			{"id", IM_PTR(uint16_t, 2)},
			{"canX", IM_PTR(uint8_t, 1)},
			{"speedPID", CF_DICT{
				{"p", IM_PTR(float, 10)},
				{"i", IM_PTR(float, 1)},
				{"d", IM_PTR(float, 0)},
				{"maxI", IM_PTR(float, 10000)},
				{"maxOut", IM_PTR(float, 20000)},
				CF_DICT_END
			}},
			CF_DICT_END
		}},
		{"motorBL", CF_DICT{
			{"type", "M3508"},
			{"id", IM_PTR(uint16_t, 3)},
			{"canX", IM_PTR(uint8_t, 1)},
			{"speedPID", CF_DICT{
				{"p", IM_PTR(float, 10)},
				{"i", IM_PTR(float, 1)},
				{"d", IM_PTR(float, 0)},
				{"maxI", IM_PTR(float, 10000)},
				{"maxOut", IM_PTR(float, 20000)},
				CF_DICT_END
			}},
			CF_DICT_END
		}},
		{"motorBR", CF_DICT{
			{"type", "M3508"},
			{"id", IM_PTR(uint16_t, 4)},
			{"canX", IM_PTR(uint8_t, 1)},
			{"speedPID", CF_DICT{
				{"p", IM_PTR(float, 10)},
				{"i", IM_PTR(float, 1)},
				{"d", IM_PTR(float, 0)},
				{"maxI", IM_PTR(float, 10000)},
				{"maxOut", IM_PTR(float, 20000)},
				CF_DICT_END
			}},
			CF_DICT_END
		}},
		CF_DICT_END
	}},
	//CAN��������
	{"can", CF_DICT{
		//CAN��������Ϣ
		{"cans", CF_DICT{
			{"0", CF_DICT{
				{"hcan", &hcan1},
				{"number", IM_PTR(uint8_t, 1)},
				CF_DICT_END
			}},
			{"1", CF_DICT{
				{"hcan", &hcan2},
				{"number", IM_PTR(uint8_t, 2)},
				CF_DICT_END
			}},
			CF_DICT_END
		}},
		//��ʱ֡����
		{"repeat-buffers", CF_DICT{
			{"0", CF_DICT{
				{"can-x", IM_PTR(uint8_t, 1)},
				{"id", IM_PTR(uint16_t, 0x200)},
				{"interval", IM_PTR(uint16_t, 2)},
				CF_DICT_END
			}},
      		{"1",CF_DICT{
				{"can-x", IM_PTR(uint8_t, 1)},
				{"id", IM_PTR(uint16_t, 0x1FF)},
				{"interval", IM_PTR(uint16_t, 2)},          
				CF_DICT_END
				}},
			{"2",CF_DICT{
				{"can-x", IM_PTR(uint8_t, 2)},
				{"id", IM_PTR(uint16_t, 0x200)},
				{"interval", IM_PTR(uint16_t, 2)},          
				CF_DICT_END
				}},		
			{"3", CF_DICT{
				{"can-x", IM_PTR(uint8_t, 2)},
				{"id", IM_PTR(uint16_t, 0x1FF)},
				{"interval", IM_PTR(uint16_t, 2)},
				CF_DICT_END
			}},
			CF_DICT_END
		}},
		CF_DICT_END
	}},
	//ң�ط�������
	{"rc",CF_DICT{
		{"uart-x",IM_PTR(uint8_t, 3)},
	   CF_DICT_END
	}},
	//���ڷ�������
	{"uart",CF_DICT{
		{"uarts",CF_DICT{
			{"0",CF_DICT{
				{"huart",&huart1},
				{"uart-x",IM_PTR(uint8_t,1)},
				{"maxRecvSize",IM_PTR(uint16_t,100)},
				CF_DICT_END
				}},
			{"1",CF_DICT{
				{"huart",&huart3},
				{"uart-x",IM_PTR(uint8_t,3)},
				{"maxRecvSize",IM_PTR(uint16_t,18)},
				CF_DICT_END
				}},
			CF_DICT_END
			}},	
		CF_DICT_END
		}},
	//�ⲿ�жϷ�������
	{"exti",CF_DICT{
		{"extis",CF_DICT{
			{"0",CF_DICT{
				{"gpio-x", GPIOA},
				{"pin-x",IM_PTR(uint8_t,0)},
				CF_DICT_END
				}},
			{"1",CF_DICT{
				{"gpio-x", GPIOA},
				{"pin-x",IM_PTR(uint8_t,4)},
				CF_DICT_END
				}},
			CF_DICT_END
			}},
		CF_DICT_END
		}},
	CF_DICT_END
};

#endif
