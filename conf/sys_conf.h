#ifndef _SYSCONF_H_
#define _SYSCONF_H_

#include "config.h"
// #include "can.h"

#define SERVICE_LIST \
	SERVICE(chassis, Chassis_TaskCallback, osPriorityNormal) \
	SERVICE(can, BSP_CAN_TaskCallback, osPriorityNormal)

ConfItem* systemConfig = CF_DICT{
	// {"can", CF_DICT{
	// 	{"cans", CF_DICT{
	// 		{"0", CF_DICT{
	// 			{"hcan", &hcan1},
	// 			{"number", IM_PTR(uint8_t, 1)},
	// 			CF_DICT_END
	// 		}},
	// 		{"1", CF_DICT{
	// 			{"hcan", &hcan2},
	// 			{"number", IM_PTR(uint8_t, 2)},
	// 			CF_DICT_END
	// 		}},
	// 		CF_DICT_END
	// 	}},
	// 	{"repeat-buffers", CF_DICT{
	// 		{"0", CF_DICT{
	// 			{"can-x", IM_PTR(uint8_t, 1)},
	// 			{"id", IM_PTR(uint16_t, 0x200)},
	// 			{"interval", IM_PTR(uint16_t, 2)},
	// 			CF_DICT_END
	// 		}},
	// 		{"1", CF_DICT{
	// 			{"can-x", IM_PTR(uint8_t, 2)},
	// 			{"id", IM_PTR(uint16_t, 0x1FF)},
	// 			{"interval", IM_PTR(uint16_t, 2)},
	// 			CF_DICT_END
	// 		}},
	// 		CF_DICT_END
	// 	}},
	// 	CF_DICT_END
	// }},
	CF_DICT_END
};

#endif
