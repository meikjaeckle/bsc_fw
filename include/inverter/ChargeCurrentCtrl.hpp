// Copyright (c) 2024 Tobias Himmler
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#pragma once

#include <cstdint>
#include <inverter/InverterData.hpp>


namespace inverter
{
class ChargeCurrentCtrl
{
  public:
  void calcChargCurrent(InverterData &inverterData, bool alarmSetChargeCurrentToZero);

  #ifdef PIO_UNIT_TESTING
  int16_t calcMaximalenLadestromSprung(int16_t i16_pNewChargeCurrent, int16_t i16_lastChargeCurrent);
  int16_t calcChargeCurrentCutOff(InverterData &inverterData, int16_t u16_lChargeCurrent);

  int16_t calcLadestromZellspanung(InverterData &inverterData, int16_t i16_pMaxChargeCurrent);
  int16_t calcLadestromBeiZelldrift(InverterData &inverterData, int16_t i16_pMaxChargeCurrent);
  int16_t calcLadestromSocAbhaengig(int16_t i16_lMaxChargeCurrent, uint8_t u8_lSoc);
  int16_t calcChargecurrent_MaxCurrentPerPackToHigh(int16_t i16_pMaxChargeCurrent);
  #endif

  private:
  #ifndef PIO_UNIT_TESTING
  int16_t calcMaximalenLadestromSprung(int16_t i16_pNewChargeCurrent, int16_t i16_lastChargeCurrent);
  int16_t calcChargeCurrentCutOff(InverterData &inverterData, int16_t u16_lChargeCurrent);

  int16_t calcLadestromZellspanung(InverterData &inverterData, int16_t i16_pMaxChargeCurrent);
  int16_t calcLadestromBeiZelldrift(InverterData &inverterData, int16_t i16_pMaxChargeCurrent);
  int16_t calcLadestromSocAbhaengig(int16_t i16_lMaxChargeCurrent, uint8_t u8_lSoc);
  int16_t calcChargecurrent_MaxCurrentPerPackToHigh(int16_t i16_pMaxChargeCurrent);
  #endif
};
} // namespace inverter