// Copyright (c) 2024 Tobias Himmler
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#pragma once

#include <cstdint>
#include <inverter/InverterData.hpp>


namespace inverter
{

class SocCtrl
{
  public:
  void calcSoc(InverterData &inverterData, bool alarmSetSocToFull);

  private:
  uint8_t getNewSocByMinCellVoltage(InverterData &inverterData, uint8_t u8_lSoc);
};
} // namespace inverter