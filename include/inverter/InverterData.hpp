// Copyright (c) 2024 Tobias Himmler
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#ifndef HEADER_8747956FAC7243B89F4FE9343AF840A0
#define HEADER_8747956FAC7243B89F4FE9343AF840A0

#include <cstdint>
#include <inverter/InverterTypes.hpp>

namespace inverter
{

struct InverterData
{
  // Ausgewählte BMS Quellen
  uint8_t bmsDatasource            {};
  uint16_t bmsDatasourceAdd       {};

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
  int16_t chargeCurrentCellVoltage{};
  int16_t chargeCurrentSoc        {};
  int16_t chargeCurrentCelldrift  {};
  int16_t chargeCurrentCutOff     {};

  // Entladeströme von der Regelung
  int16_t dischargeCurrentCellVoltage{};

  // Charge current cut off
  uint16_t chargeCurrentCutOffTimer{};

  // Wenn Zellspannung kleiner x mV wird SoC auf x% setzen
  SocZellspgStates socZellspannungState {SocZellspgStates::STATE_MINCELLSPG_SOC_WAIT_OF_MIN};
  uint16_t socZellspannungSperrzeitTimer{};
};

} // namespace inverter

#endif // HEADER_8747956FAC7243B89F4FE9343AF840A0