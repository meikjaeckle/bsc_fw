// Copyright (c) 2024 Tobias Himmler
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#pragma once

#include <cstdint>
#include <inverter/InverterData.hpp>


namespace inverter
{
class DisChargeCurrentCtrl
{
  public:
  void calcDisChargCurrent(InverterData &inverterData, bool alarmSetDischargeCurrentToZero);

  private:
  int16_t calcEntladestromZellspanung(InverterData &inverterData, int16_t i16_pMaxDischargeCurrent);
};
} // namespace inverter