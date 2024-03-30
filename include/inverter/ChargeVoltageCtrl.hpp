// Copyright (c) 2024 Tobias Himmler
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#pragma once

#include <cstdint>
#include "inverter/Inverter.hpp"


namespace inverters
{
class ChargeVoltageCtrl
{
  public:
  void calcChargeVoltage(Inverter &inverter, InverterData &inverterData);

  private:
  uint16_t calcDynamicReduzeChargeVolltage(InverterData &inverterData, uint16_t u16_lChargeVoltage);
};
} // namespace inverters