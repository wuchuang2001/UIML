#ifndef _SYSCONF_H_
#define _SYSCONF_H_

#include "config.h"

#define SERVICE_LIST SERVICE(chassis, Chassis_TaskCallback, osPriorityNormal) 
//                     SERVICE(uart, Uart_TaskCallback, osPriorityNormal)

ConfItem *conf = CF_DICT
{
	// {"chassis", CF_DICT{
    //                     {"", IM_PTR(int[],1,2,3,4,5)},
	// 	                {"n2", CF_DICT{
    //                                     {"nn1","nn1-value"},
    //                                     CF_DICT_END
    //                                   }
    //                     },
    //                     CF_DICT_END
    //                    }
    // },
	CF_DICT_END
};

#endif
