/*
 * CRC16 Checksum Utility
 * CRC16校验工具（查表法实现）
 */

#ifndef __CRC16_H__
#define __CRC16_H__

#include <stdint.h>

/**
 * @brief 计算CRC16校验值（查表法）
 * @param data 数据指针
 * @param length 数据长度
 * @return CRC16校验值
 */
uint16_t crc16_calculate(const uint8_t *data, uint32_t length);

#endif /* __CRC16_H__ */
