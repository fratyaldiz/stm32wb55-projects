#ifndef MODBUS_CRC_H
#define MODBUS_CRC_H

#include <stdint.h>

/*
 * CRC-16/MODBUS
 * Polynomial 0xA001 (reflected 0x8005), initial value 0xFFFF.
 * Standart dogrulama: modbus_crc16("123456789", 9) == 0x4B37
 *
 * Modbus RTU: gonderim byte sirasi once CRC low, sonra CRC high.
 */
uint16_t modbus_crc16(const uint8_t *data, uint16_t len);

#endif /* MODBUS_CRC_H */
