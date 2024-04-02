// Copyright (c) 2022 Tobias Himmler
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#ifndef I2C_H
#define I2C_H

#include "Arduino.h"
#include "defines.h"

namespace inverter
{
struct DataAdapter; // Forward-Deklaration
}

void i2cInit();
void i2cCyclicRun(const inverter::DataAdapter& dataAdapter);
void i2cSendData(uint8_t i2cAdr, uint8_t data1, uint8_t data2, uint8_t data3, const void *dataAdr, uint8_t dataLen);
void i2cSendData(uint8_t i2cAdr, uint8_t data1, uint8_t data2, uint8_t data3, const String& data);

template<typename T>
void i2cSendData(uint8_t i2cAdr, uint8_t data1, uint8_t data2, uint8_t data3, T data)
{
  if constexpr (std::is_integral_v<T>)
    i2cSendData(i2cAdr, data1, data2, data3, reinterpret_cast<uint8_t*>(&data), sizeof(data));
  else
    static_assert(std::is_integral_v<decltype(data)>, "Type is not supported"); // Helper to catch if the given type is not supported
}

void i2cExtSerialSetEnable(uint8_t u8_serialDevNr, serialRxTxEn_e serialRxTxEn);
bool isSerialExtEnabled();

#endif