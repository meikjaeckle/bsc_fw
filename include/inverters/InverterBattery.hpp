// Copyright (c) 2024 Tobias Himmler
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#pragma once

#include <cstdint>
#include "inverters/Inverter.hpp"


namespace inverters
{
class InverterBattery
{
  public:
  void getBatteryVoltage(InverterData &inverterData);
  void getBatteryCurrent(InverterData &inverterData);
  int16_t getBatteryTemp(const InverterData& inverterData);

  private:
  int16_t _batteryVoltage; // TODO MEJ
  bool _isOneBatteryPackOnline;

};
} // namespace inverters