// Copyright (c) 2024 Meik Jaeckle
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#ifndef HEADER_1AC1E27ED8674A41AAD7A178FEDF0C34
#define HEADER_1AC1E27ED8674A41AAD7A178FEDF0C34

#include <cstdint>
#include <kernelapi/rtos/RecursiveMutex.hpp>
#include <inverters/InverterData.hpp>
#include <inverters/InverterTypes.hpp>

namespace inverters
{

struct IDataReadAdapter
{
  virtual ~IDataReadAdapter() = default;

  virtual rtos::RecursiveMutex& getMutex() = 0;

  /**
   * @returns a full copy of the InverterData.
   */
  virtual InverterData getInverterData() const = 0;

  // Ausgewählte BMS Quellen
  virtual uint8_t  getBmsDatasource()                 const = 0;
  virtual uint16_t getBmsDatasourceAdd()              const = 0;

  // Wechselrichterdaten
  virtual bool     isNoBatteryPackOnline()            const = 0;
  virtual int16_t  getBatteryVoltage()                const = 0;
  virtual int16_t  getBatteryCurrent()                const = 0;
  virtual int16_t  getBatteryTemperatur()             const = 0; // TODO MEJ not used
  virtual uint16_t getInverterChargeVoltage()         const = 0; // TODO MEJ not used
  virtual uint16_t getInverterSoc()                   const = 0;
  virtual int16_t  getInverterChargeCurrent()         const = 0;
  virtual int16_t  getInverterDischargeCurrent()      const = 0;

  // Ströme von der Ladestromregelung
  virtual int16_t getChargeCurrentCellVoltage()       const = 0;
  virtual int16_t getChargeCurrentSoc()               const = 0;
  virtual int16_t getChargeCurrentCelldrift()         const = 0;
  virtual int16_t getChargeCurrentCutOff()            const = 0;

  // Entladeströme von der Regelung
  virtual int16_t getDischargeCurrentCellVoltage()    const = 0;  // TODO MEJ not used

  // Charge current cut off
  virtual uint16_t getChargeCurrentCutOffTimer()       const = 0;  // TODO MEJ not used

  // Wenn Zellspannung kleiner x mV wird SoC auf x% setzen
  virtual SocZellspgStates getSocZellspannungState()  const = 0;  // TODO MEJ not used
  virtual uint16_t getSocZellspannungSperrzeitTimer() const = 0;  // TODO MEJ not used
};

} // namespace inverters

#endif // HEADER_1AC1E27ED8674A41AAD7A178FEDF0C34