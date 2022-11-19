#ifndef _SYSCONF_H_
#define _SYSCONF_H_

#include "config.h"

//<<< Use Configuration Wizard in Context Menu >>>
//<h>BSP Config
//<q0>CAN
//<i>Select to include "can.h"
//<q1>UART
//<i>Select to include "uart.h"
#define CONF_CAN_ENABLE		1
#define CONF_UART_ENABLE	0
//</h>
#if CONF_CAN_ENABLE
#include "can.h"
#endif
#if CONF_UART_ENABLE
#include "uart.h"
#endif
// <<< end of configuration section >>>

//服务配置列表，每项格式(服务名,服务任务函数,任务优先级)
#define SERVICE_LIST \
	SERVICE(chassis, Chassis_TaskCallback, osPriorityNormal) \
	SERVICE(can, BSP_CAN_TaskCallback, osPriorityNormal)

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
//			{"1", CF_DICT{
//				{"hcan", &hcan2},
//				{"number", IM_PTR(uint8_t, 2)},
//				CF_DICT_END
//			}},
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
			{"1", CF_DICT{
				{"can-x", IM_PTR(uint8_t, 2)},
				{"id", IM_PTR(uint16_t, 0x1FF)},
				{"interval", IM_PTR(uint16_t, 2)},
				CF_DICT_END
			}},
			CF_DICT_END
		}},
		CF_DICT_END
	}},
	CF_DICT_END
};

#endif
