#ifndef _SYSCONF_H_
#define _SYSCONF_H_
//<<< Use Configuration Wizard in Context Menu >>>
//<h>BSP Enable
//<q0>CAN Enable
//<i>Use CAN Please Click CheckBox ON
//<q1>UART Enable
//<i>Use UART Please Click CheckBox ON
#define CONF_CAN_ENABLE		1
#define CONF_UART_ENABLE	0
//</h>
#include "config.h"

#if CONF_CAN_ENABLE
#include "can.h"
#endif

#if CONF_UART_ENABLE
#include "uart.h"
#endif

// <<< end of configuration section >>>

#define SERVICE_LIST \
	SERVICE(chassis, Chassis_TaskCallback, osPriorityNormal) \
	SERVICE(can, BSP_CAN_TaskCallback, osPriorityNormal)

ConfItem* systemConfig = CF_DICT{
	{
		"chassis", CF_DICT{
			{"taskInterval", IM_PTR(uint8_t, 2)},
			{"info", CF_DICT{
				{"wheelbase", IM_PTR(float, 100)},
				{"wheeltrack", IM_PTR(float, 100)},
				{"wheelRadius", IM_PTR(float, 76)},
				{"offsetX", IM_PTR(float, 0)},
				{"offsetY", IM_PTR(float, 0)},
				CF_DICT_END
			}},
			{"move", CF_DICT{
				{"maxVx", IM_PTR(float, 2000)},
				{"maxVy", IM_PTR(float, 2000)},
				{"maxVw", IM_PTR(float, 2)},
				{"xAcc", IM_PTR(float, 1000)},
				{"yAcc", IM_PTR(float, 1000)},
				CF_DICT_END
			}},
			{"motorFL", CF_DICT{
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
	{"can", CF_DICT{
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
//			CF_DICT_END
		}},
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
