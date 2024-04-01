// Copyright (c) 2024 Meik Jaeckle
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#include <cstdint>

#include <kernelapi/rtos/LockGuard.hpp>
#include <kernelapi/rtos/RecursiveMutex.hpp>

#include <inverters/InverterData.hpp>
#include <inverters/IDataReadAdapter.hpp>
#include <inverters/DataReadAdapter.hpp>

namespace inverters
{

void DataReadAdapter::Update(const InverterData& data)
{
  const rtos::LockGuard lock(_mutex);
  _data = data;
}

// ----------- IDataReadAdapter interface methods ---------------

rtos::RecursiveMutex& DataReadAdapter::getMutex()
{
  return _mutex;
}

InverterData DataReadAdapter::getInverterData() const
{
  const rtos::LockGuard lock(_mutex);
  return _data;
}

uint8_t DataReadAdapter::getBmsDatasource() const
{
  const rtos::LockGuard lock(_mutex);
  return _data.bmsDatasource;
}

uint16_t DataReadAdapter::getBmsDatasourceAdd() const
{
  const rtos::LockGuard lock(_mutex);
  return _data.bmsDatasourceAdd;
}

// Wechselrichterdaten
bool DataReadAdapter::isNoBatteryPackOnline() const
{
  const rtos::LockGuard lock(_mutex);
  return _data.noBatteryPackOnline;
}

int16_t DataReadAdapter::getBatteryVoltage() const
{
  const rtos::LockGuard lock(_mutex);
  return _data.batteryVoltage;
}

int16_t DataReadAdapter::getBatteryCurrent() const
{
  const rtos::LockGuard lock(_mutex);
  return _data.batteryCurrent;
}

int16_t DataReadAdapter::getBatteryTemperatur() const
{
  const rtos::LockGuard lock(_mutex);
  return _data.batteryTemperatur;
}

uint16_t DataReadAdapter::getInverterChargeVoltage() const
{
  const rtos::LockGuard lock(_mutex);
  return _data.inverterChargeVoltage;
}

uint16_t DataReadAdapter::getInverterSoc() const
{
  return _data.inverterSoc;
}

int16_t DataReadAdapter::getInverterChargeCurrent() const
{
  const rtos::LockGuard lock(_mutex);
  return _data.inverterChargeCurrent;
}

int16_t DataReadAdapter::getInverterDischargeCurrent() const
{
  const rtos::LockGuard lock(_mutex);
  return _data.inverterDischargeCurrent;
}

// Ströme von der Ladestromregelung
int16_t DataReadAdapter::getChargeCurrentCellVoltage() const
{
  const rtos::LockGuard lock(_mutex);
  return _data.chargeCurrentCellVoltage;
}

int16_t DataReadAdapter::getChargeCurrentSoc() const
{
  const rtos::LockGuard lock(_mutex);
  return _data.chargeCurrentSoc;
}

int16_t DataReadAdapter::getChargeCurrentCelldrift() const
{
  const rtos::LockGuard lock(_mutex);
  return _data.chargeCurrentCelldrift;
}

int16_t DataReadAdapter::getChargeCurrentCutOff() const
{
  const rtos::LockGuard lock(_mutex);
  return _data.chargeCurrentCutOff;
}

// Entladeströme von der Regelung
int16_t DataReadAdapter::getDischargeCurrentCellVoltage() const
{
  const rtos::LockGuard lock(_mutex);
  return _data.dischargeCurrentCellVoltage;
}

// Charge current cut off
uint16_t DataReadAdapter::getChargeCurrentCutOffTimer() const
{
  const rtos::LockGuard lock(_mutex);
  return _data.chargeCurrentCutOffTimer;
}

// Wenn Zellspannung kleiner x mV wird SoC auf x% setzen
SocZellspgStates DataReadAdapter::getSocZellspannungState()  const
{
  const rtos::LockGuard lock(_mutex);
  return _data.socZellspannungState;
}

uint16_t DataReadAdapter::getSocZellspannungSperrzeitTimer() const
{
  const rtos::LockGuard lock(_mutex);
  return _data.socZellspannungSperrzeitTimer;
}

// ------------------------------------------------------------

} // namespace inverters
