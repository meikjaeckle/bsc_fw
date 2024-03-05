// Copyright (c) 2023 Tobias Himmler
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#ifndef INC_CRC_H_
#define INC_CRC_H_

#include <cstdint>

uint16_t crc16 (const uint8_t* nData, uint16_t wLength);
uint32_t calcCrc32(const uint8_t* pData, std::size_t dataLength);
uint32_t calcCrc32(uint32_t crcIn, const uint8_t* pData, std::size_t dataLength);

#endif
