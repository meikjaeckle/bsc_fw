// Copyright (c) 2024 Meik Jaeckle
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#ifndef HEADER_E9A6A30640704E2CBB8C7D7F9A8CE87A
#define HEADER_E9A6A30640704E2CBB8C7D7F9A8CE87A

#include <cstdint>

#include <kernelapi/rtos/RecursiveMutex.hpp>

#include <inverter/DataAdapter.hpp>
#include <inverter/InverterData.hpp>

namespace inverter
{

class InverterDataAdapter :
  public DataAdapter
{
  public:
  void Update(const InverterData& data);

  // DataAdapter interface methods
  public:

  rtos::RecursiveMutex& getMutex() override; //!< @copydoc DataAdapter::getMutex()

  InverterData getInverterData() const override; //!< @copydoc DataAdapter::getInverterData()

  // Ausgewählte BMS Quellen
  uint8_t  getBmsDatasource()    const override; //!< @copydoc DataAdapter::getBmsDatasource()
  uint16_t getBmsDatasourceAdd() const override; //!< @copydoc DataAdapter::getBmsDatasourceAdd()

  // Wechselrichterdaten
  bool     isNoBatteryPackOnline()       const override; //!< @copydoc DataAdapter::isNoBatteryPackOnline()
  int16_t  getBatteryVoltage()           const override; //!< @copydoc DataAdapter::getBatteryVoltage()
  int16_t  getBatteryCurrent()           const override; //!< @copydoc DataAdapter::getBatteryCurrent()
  int16_t  getBatteryTemperatur()        const override; //!< @copydoc DataAdapter::getBatteryTemperatur()
  uint16_t getInverterChargeVoltage()    const override; //!< @copydoc DataAdapter::getInverterChargeVoltage()
  uint16_t getInverterSoc()              const override; //!< @copydoc DataAdapter::getInverterSoc()
  int16_t  getInverterChargeCurrent()    const override; //!< @copydoc DataAdapter::getInverterChargeCurrent()
  int16_t  getInverterDischargeCurrent() const override; //!< @copydoc DataAdapter::getInverterDischargeCurrent()

  // Ströme von der Ladestromregelung
  int16_t getChargeCurrentCellVoltage()  const override; //!< @copydoc DataAdapter::getChargeCurrentCellVoltage()
  int16_t getChargeCurrentSoc()          const override; //!< @copydoc DataAdapter::getChargeCurrentSoc()
  int16_t getChargeCurrentCelldrift()    const override; //!< @copydoc DataAdapter::getChargeCurrentCelldrift()
  int16_t getChargeCurrentCutOff()       const override; //!< @copydoc DataAdapter::getChargeCurrentCutOff()

  // Entladeströme von der Regelung
  int16_t getDischargeCurrentCellVoltage() const override; //!< @copydoc DataAdapter::getDischargeCurrentCellVoltage()

  // Charge current cut off
  uint16_t getChargeCurrentCutOffTimer() const override; //!< @copydoc DataAdapter::getChargeCurrentCutOffTimer()

  // Wenn Zellspannung kleiner x mV wird SoC auf x% setzen
  SocZellspgStates getSocZellspannungState()  const override; //!< @copydoc DataAdapter::getSocZellspannungState()
  uint16_t getSocZellspannungSperrzeitTimer() const override; //!< @copydoc DataAdapter::getSocZellspannungSperrzeitTimer()

  private:
  mutable rtos::RecursiveMutex _mutex;
  InverterData _data;
};

} // namespace inverter

#endif // HEADER_E9A6A30640704E2CBB8C7D7F9A8CE87A
