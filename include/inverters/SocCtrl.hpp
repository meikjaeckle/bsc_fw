// Copyright (c) 2024 Tobias Himmler
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#pragma once

#include <cstdint>
#include "inverters/Inverter.hpp"
#include "inverters/InverterData.hpp"


namespace inverters
{

class SocCtrl
{
  public:
  void calcSoc(Inverter &inverter, InverterData &inverterData, bool alarmSetSocToFull);

  private:
  uint8_t getNewSocByMinCellVoltage(InverterData &inverterData, uint8_t u8_lSoc);
};
} // namespace inverters