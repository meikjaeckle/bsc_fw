// Copyright (c) 2024 Tobias Himmler
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#pragma once

#include <cstdint>
#include <inverter/InverterData.hpp>


namespace inverter
{
class ChargeVoltageCtrl
{
  public:
  void calcChargeVoltage(InverterData &inverterData);

  private:
  uint16_t calcDynamicReduceChargeVoltage(InverterData &inverterData, uint16_t u16_lChargeVoltage);
};
} // namespace inverter