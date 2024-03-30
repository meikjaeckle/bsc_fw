// Copyright (c) 2024 Tobias Himmler
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#ifndef HEADER_8747956FAC7243B89F4FE9343AF840A0
#define HEADER_8747956FAC7243B89F4FE9343AF840A0

#include <cstdint>

namespace inverters
{

enum class SocZellspgStates {STATE_MINCELLSPG_SOC_WAIT_OF_MIN=0, STATE_MINCELLSPG_SOC_BELOW_MIN, STATE_MINCELLSPG_SOC_LOCKTIMER};

struct InverterData
{
  // Ausgewählte BMS Quellen
  uint8_t u8_bmsDatasource            {};
  uint16_t u16_bmsDatasourceAdd       {};

  // Wechselrichterdaten
  bool       noBatteryPackOnline      {true};
  int16_t    batteryVoltage           {};
  int16_t    batteryCurrent           {};
  int16_t    batteryTemperatur        {};
  uint16_t   inverterChargeVoltage    {};
  uint16_t   inverterSoc              {};
  int16_t    inverterChargeCurrent    {};
  int16_t    inverterDischargeCurrent {};

  // Ströme von der Ladestromregelung
  int16_t calcChargeCurrentCellVoltage{};
  int16_t calcChargeCurrentSoc        {};
  int16_t calcChargeCurrentCelldrift  {};
  int16_t calcChargeCurrentCutOff     {};

  // Entladeströme von der Regelung
  int16_t calcDischargeCurrentCellVoltage{};

  // Charge current cut off
  uint16_t u16_mChargeCurrentCutOfTimer{};

  // Wenn Zellspannung kleiner x mV wird SoC auf x% setzen
  SocZellspgStates socZellspannungState {SocZellspgStates::STATE_MINCELLSPG_SOC_WAIT_OF_MIN};
  uint16_t u16_mSocZellspannungSperrzeitTimer{};
};

} // namespace inverters

#endif // HEADER_8747956FAC7243B89F4FE9343AF840A0