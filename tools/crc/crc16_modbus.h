#ifndef _CRC16_MODBUS_H_
#define _CRC16_MODBUS_H_

#include <stdint.h>

uint16_t CRC_Calculate(uint8_t *pdata, uint8_t num);

#endif
