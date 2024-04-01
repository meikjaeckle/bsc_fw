// Copyright (c) 2024 Meik Jaeckle
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#ifndef HEADER_E9A6A30640704E2CBB8C7D7F9A8CE87A
#define HEADER_E9A6A30640704E2CBB8C7D7F9A8CE87A

#include <cstdint>

#include <kernelapi/rtos/RecursiveMutex.hpp>

#include <inverters/IDataReadAdapter.hpp>
#include <inverters/InverterData.hpp>

namespace inverters
{

class DataReadAdapter :
  public IDataReadAdapter
{
  public:
  void Update(const InverterData& data);

  // IDataReadAdapter interface methods
  public:

  rtos::RecursiveMutex& getMutex() override; //!< @copydoc IDataReadAdapter::getMutex()

  InverterData getInverterData() const override; //!< @copydoc IDataReadAdapter::getInverterData()

  // Ausgewählte BMS Quellen
  uint8_t  getBmsDatasource()    const override; //!< @copydoc IDataReadAdapter::getBmsDatasource()
  uint16_t getBmsDatasourceAdd() const override; //!< @copydoc IDataReadAdapter::getBmsDatasourceAdd()

  // Wechselrichterdaten
  bool     isNoBatteryPackOnline()       const override; //!< @copydoc IDataReadAdapter::isNoBatteryPackOnline()
  int16_t  getBatteryVoltage()           const override; //!< @copydoc IDataReadAdapter::getBatteryVoltage()
  int16_t  getBatteryCurrent()           const override; //!< @copydoc IDataReadAdapter::getBatteryCurrent()
  int16_t  getBatteryTemperatur()        const override; //!< @copydoc IDataReadAdapter::getBatteryTemperatur()
  uint16_t getInverterChargeVoltage()    const override; //!< @copydoc IDataReadAdapter::getInverterChargeVoltage()
  uint16_t getInverterSoc()              const override; //!< @copydoc IDataReadAdapter::getInverterSoc()
  int16_t  getInverterChargeCurrent()    const override; //!< @copydoc IDataReadAdapter::getInverterChargeCurrent()
  int16_t  getInverterDischargeCurrent() const override; //!< @copydoc IDataReadAdapter::getInverterDischargeCurrent()

  // Ströme von der Ladestromregelung
  int16_t getChargeCurrentCellVoltage()  const override; //!< @copydoc IDataReadAdapter::getChargeCurrentCellVoltage()
  int16_t getChargeCurrentSoc()          const override; //!< @copydoc IDataReadAdapter::getChargeCurrentSoc()
  int16_t getChargeCurrentCelldrift()    const override; //!< @copydoc IDataReadAdapter::getChargeCurrentCelldrift()
  int16_t getChargeCurrentCutOff()       const override; //!< @copydoc IDataReadAdapter::getChargeCurrentCutOff()

  // Entladeströme von der Regelung
  int16_t getDischargeCurrentCellVoltage() const override; //!< @copydoc IDataReadAdapter::getDischargeCurrentCellVoltage()

  // Charge current cut off
  uint16_t getChargeCurrentCutOffTimer() const override; //!< @copydoc IDataReadAdapter::getChargeCurrentCutOffTimer()

  // Wenn Zellspannung kleiner x mV wird SoC auf x% setzen
  SocZellspgStates getSocZellspannungState()  const override; //!< @copydoc IDataReadAdapter::getSocZellspannungState()
  uint16_t getSocZellspannungSperrzeitTimer() const override; //!< @copydoc IDataReadAdapter::getSocZellspannungSperrzeitTimer()

  private:
  mutable rtos::RecursiveMutex _mutex;
  InverterData _data;
};

} // namespace inverters

#endif // HEADER_E9A6A30640704E2CBB8C7D7F9A8CE87A
