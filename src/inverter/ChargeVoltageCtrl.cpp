// Copyright (c) 2024 Tobias Himmler
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#include <inverter/ChargeVoltageCtrl.hpp>
#include <bms/utils/BmsDataUtils.hpp>
#include "defines.h"
#include "WebSettings.h"

namespace inverter
{

void ChargeVoltageCtrl::calcChargeVoltage(inverter::InverterData &inverterData)
{
  uint16_t u16_lChargeVoltage = (uint16_t)WebSettings::getFloat(ID_PARAM_BMS_MAX_CHARGE_SPG,0);
  u16_lChargeVoltage = calcDynamicReduceChargeVoltage(inverterData, u16_lChargeVoltage);

  inverterData.inverterChargeVoltage = u16_lChargeVoltage; //ToDo semaphore
}


/* */
uint16_t ChargeVoltageCtrl::calcDynamicReduceChargeVoltage(inverter::InverterData &inverterData, uint16_t u16_lChargeVoltage)
{
  static uint16_t u16_lDynamicChargeVoltage = u16_lChargeVoltage;

  if(WebSettings::getBool(ID_PARAM_INVERTER_CHARGE_VOLTAGE_DYNAMIC_REDUCE_EN,0)==true) //wenn enabled
  {
    const uint16_t u16_lStartZellVoltage = WebSettings::getInt(ID_PARAM_INVERTER_CHARGE_VOLTAGE_DYNAMIC_REDUCE_ZELLSPG,0,DT_ID_PARAM_INVERTER_CHARGE_VOLTAGE_DYNAMIC_REDUCE_ZELLSPG);
    const uint16_t u16_lDeltaCellVoltage= WebSettings::getInt(ID_PARAM_INVERTER_CHARGE_VOLTAGE_DYNAMIC_REDUCE_DELTA,0,DT_ID_PARAM_INVERTER_CHARGE_VOLTAGE_DYNAMIC_REDUCE_DELTA);

    if(bms::utils::getMaxCellSpannungFromBms(inverterData.bmsDatasource, inverterData.bmsDatasourceAdd)>u16_lStartZellVoltage)
    {
      const uint16_t u16_lMaxCellDiffVoltage = bms::utils::getMaxCellDifferenceFromBms(inverterData.bmsDatasource, inverterData.bmsDatasourceAdd);
      if(u16_lMaxCellDiffVoltage>u16_lDeltaCellVoltage)
      {
        u16_lDynamicChargeVoltage -= 1; //1=100mV
        if(u16_lDynamicChargeVoltage < 0) // TODO MEJ unsigned value can't get negative, may overflow
          u16_lDynamicChargeVoltage=0;

        return u16_lDynamicChargeVoltage;
      }
      else if(u16_lMaxCellDiffVoltage<u16_lDeltaCellVoltage)
      {
        u16_lDynamicChargeVoltage += 1; //1=100mV
        if(u16_lDynamicChargeVoltage > u16_lChargeVoltage)
          u16_lDynamicChargeVoltage = u16_lChargeVoltage;

        return u16_lDynamicChargeVoltage;
      }
    }
  }
  return u16_lChargeVoltage;
}
} // namespace inverter
