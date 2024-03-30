// Copyright (c) 2024 Tobias Himmler
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#pragma once

#include <cstdint>
#include "inverter/Inverter.hpp"


namespace inverters
{
class DisChargeCurrentCtrl
{
  public:
  void calcDisChargCurrent(Inverter &inverter, InverterData &inverterData, bool alarmSetDischargeCurrentToZero);

  private:
  int16_t calcEntladestromZellspanung(InverterData &inverterData, int16_t i16_pMaxDischargeCurrent);
};
} // namespace inverters