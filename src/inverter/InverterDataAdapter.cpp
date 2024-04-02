// Copyright (c) 2024 Meik Jaeckle
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#include <cstdint>

#include <kernelapi/rtos/LockGuard.hpp>
#include <kernelapi/rtos/RecursiveMutex.hpp>

#include <inverter/InverterData.hpp>
#include <inverter/InverterDataAdapter.hpp>

namespace inverter
{

void InverterDataAdapter::Update(const InverterData& data)
{
  const rtos::LockGuard lock(_mutex);
  _data = data;
}

// ----------- DataAdapter interface methods ---------------

rtos::RecursiveMutex& InverterDataAdapter::getMutex()
{
  return _mutex;
}

InverterData InverterDataAdapter::getInverterData() const
{
  const rtos::LockGuard lock(_mutex);
  return _data;
}

uint8_t InverterDataAdapter::getBmsDatasource() const
{
  const rtos::LockGuard lock(_mutex);
  return _data.bmsDatasource;
}

uint16_t InverterDataAdapter::getBmsDatasourceAdd() const
{
  const rtos::LockGuard lock(_mutex);
  return _data.bmsDatasourceAdd;
}

// Wechselrichterdaten
bool InverterDataAdapter::isNoBatteryPackOnline() const
{
  const rtos::LockGuard lock(_mutex);
  return _data.noBatteryPackOnline;
}

int16_t InverterDataAdapter::getBatteryVoltage() const
{
  const rtos::LockGuard lock(_mutex);
  return _data.batteryVoltage;
}

int16_t InverterDataAdapter::getBatteryCurrent() const
{
  const rtos::LockGuard lock(_mutex);
  return _data.batteryCurrent;
}

int16_t InverterDataAdapter::getBatteryTemperatur() const
{
  const rtos::LockGuard lock(_mutex);
  return _data.batteryTemperatur;
}

uint16_t InverterDataAdapter::getInverterChargeVoltage() const
{
  const rtos::LockGuard lock(_mutex);
  return _data.inverterChargeVoltage;
}

uint16_t InverterDataAdapter::getInverterSoc() const
{
  return _data.inverterSoc;
}

int16_t InverterDataAdapter::getInverterChargeCurrent() const
{
  const rtos::LockGuard lock(_mutex);
  return _data.inverterChargeCurrent;
}

int16_t InverterDataAdapter::getInverterDischargeCurrent() const
{
  const rtos::LockGuard lock(_mutex);
  return _data.inverterDischargeCurrent;
}

// Ströme von der Ladestromregelung
int16_t InverterDataAdapter::getChargeCurrentCellVoltage() const
{
  const rtos::LockGuard lock(_mutex);
  return _data.chargeCurrentCellVoltage;
}

int16_t InverterDataAdapter::getChargeCurrentSoc() const
{
  const rtos::LockGuard lock(_mutex);
  return _data.chargeCurrentSoc;
}

int16_t InverterDataAdapter::getChargeCurrentCelldrift() const
{
  const rtos::LockGuard lock(_mutex);
  return _data.chargeCurrentCelldrift;
}

int16_t InverterDataAdapter::getChargeCurrentCutOff() const
{
  const rtos::LockGuard lock(_mutex);
  return _data.chargeCurrentCutOff;
}

// Entladeströme von der Regelung
int16_t InverterDataAdapter::getDischargeCurrentCellVoltage() const
{
  const rtos::LockGuard lock(_mutex);
  return _data.dischargeCurrentCellVoltage;
}

// Charge current cut off
uint16_t InverterDataAdapter::getChargeCurrentCutOffTimer() const
{
  const rtos::LockGuard lock(_mutex);
  return _data.chargeCurrentCutOffTimer;
}

// Wenn Zellspannung kleiner x mV wird SoC auf x% setzen
SocZellspgStates InverterDataAdapter::getSocZellspannungState()  const
{
  const rtos::LockGuard lock(_mutex);
  return _data.socZellspannungState;
}

uint16_t InverterDataAdapter::getSocZellspannungSperrzeitTimer() const
{
  const rtos::LockGuard lock(_mutex);
  return _data.socZellspannungSperrzeitTimer;
}

// ------------------------------------------------------------

} // namespace inverter
