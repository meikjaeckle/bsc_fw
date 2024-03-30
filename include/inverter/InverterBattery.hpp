// Copyright (c) 2024 Tobias Himmler
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#pragma once

#include <cstdint>
#include "inverter/Inverter.hpp"


namespace inverters
{
class InverterBattery
{
  public:
  void getBatteryVoltage(Inverter &inverter, InverterData &inverterData);
  void getBatteryCurrent(Inverter &inverter, InverterData &inverterData);
  int16_t getBatteryTemp(InverterData &inverterData);

  private:
};
} // namespace inverters