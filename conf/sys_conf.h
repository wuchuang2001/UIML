#ifndef _SYSCONF_H_
#define _SYSCONF_H_

#include "config.h"

//<<< Use Configuration Wizard in Context Menu >>>
//<h>BSP Config
//<q0>CAN
//<i>Select to include "can.h"
//<q1>UART
//<i>Select to include "usart.h"
//<q2>EXIT
//<i>Select to include "gpio.h"
//<q3>TIM
//<i>Select to include "tim.h"
#define CONF_CAN_ENABLE		1
#define CONF_USART_ENABLE	1
#define CONF_EXIT_ENABLE 1
#define CONF_TIM_ENABLE 1
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
#if CONF_TIM_ENABLE
#include "tim.h"
#endif
// <<< end of configuration section >>>

//服务配置列表，每项格式(服务名,服务任务函数,任务优先级,任务栈大小)
#define SERVICE_LIST \
	SERVICE(chassis, Chassis_TaskCallback, osPriorityNormal,256) \
	SERVICE(can, BSP_CAN_TaskCallback, osPriorityRealtime,128) \
	SERVICE(rc, RC_TaskCallback, osPriorityNormal,256)         \
	SERVICE(uart, BSP_UART_TaskCallback, osPriorityNormal,256) \
	SERVICE(exti, BSP_EXTI_TaskCallback, osPriorityNormal,256) \
	SERVICE(tim, BSP_TIM_TaskCallback, osPriorityNormal,256)
	
//各服务配置项
ConfItem* systemConfig = CF_DICT{
	//底盘服务配置
	{"chassis", CF_DICT{
		//任务循环周期
		{"taskInterval", IM_PTR(uint8_t, 2)},
		//底盘尺寸信息
		{"info", CF_DICT{
			{"wheelbase", IM_PTR(float, 100)},
			{"wheeltrack", IM_PTR(float, 100)},
			{"wheelRadius", IM_PTR(float, 76)},
			{"offsetX", IM_PTR(float, 0)},
			{"offsetY", IM_PTR(float, 0)},
			CF_DICT_END
		}},
		//底盘移动速度/加速度配置
		{"move", CF_DICT{
			{"maxVx", IM_PTR(float, 2000)},
			{"maxVy", IM_PTR(float, 2000)},
			{"maxVw", IM_PTR(float, 2)},
			{"xAcc", IM_PTR(float, 1000)},
			{"yAcc", IM_PTR(float, 1000)},
			CF_DICT_END
		}},
		//四个电机配置
		{"motorFL", CF_DICT{
			{"type", "M3508"},
			{"id", IM_PTR(uint16_t, 1)},
			{"canX", IM_PTR(uint8_t, 1)},
//			{"reductionRatio", IM_PTR(float, 1)},   //若使用改装减速箱或者拆掉减速箱的电机则修改此参数，若使用原装电机则无需配置此参数
			{"speedPID", CF_DICT{
				{"p", IM_PTR(float, 10)},
				{"i", IM_PTR(float, 1)},
				{"d", IM_PTR(float, 0)},
				{"maxI", IM_PTR(float, 10000)},
				{"maxOut", IM_PTR(float, 20000)},
				CF_DICT_END
			}},
//			{"anglePID", CF_DICT{                  //串级pid示例，如需使用串级pid照此模板配置即可
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
	//CAN服务配置
	{"can", CF_DICT{
		//CAN控制器信息
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
		//定时帧配置
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
	//遥控服务配置
	{"rc",CF_DICT{
		{"uart-x",IM_PTR(uint8_t, 3)},
	   CF_DICT_END
	}},
	//串口服务配置
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
	//外部中断服务配置
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
	{"tim",CF_DICT{
		{"tims",CF_DICT{
			{"0",CF_DICT{
				{"htim",&htim1},
				{"number",IM_PTR(uint8_t,1)},
				{"mode","encode"},
				CF_DICT_END
				}},
			{"1",CF_DICT{
				{"htim",&htim14},
				{"number",IM_PTR(uint8_t,14)},
				{"mode","pwm"},
				CF_DICT_END
                }}, 
			CF_DICT_END
			}},
		CF_DICT_END
		}},	
	CF_DICT_END
};

#endif
