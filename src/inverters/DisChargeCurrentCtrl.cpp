// Copyright (c) 2024 Tobias Himmler
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#include <algorithm>
#include <array>

#include "inverters/DisChargeCurrentCtrl.hpp"
#include <bms/utils/BmsDataUtils.hpp>
#include "defines.h"
#include "WebSettings.h"

namespace inverters
{

void DisChargeCurrentCtrl::calcDisChargCurrent(inverters::InverterData &inverterData, bool alarmSetDischargeCurrentToZero)
{
  std::array<int16_t, 1> maxDischargeCurrentList {};
  int16_t newDisChargeCurrent {0};

  if(alarmSetDischargeCurrentToZero)
  {
    newDisChargeCurrent = 0;
  }
  else
  {
    int16_t maxDischargeCurrent = (int16_t)WebSettings::getInt(ID_PARAM_BMS_MAX_DISCHARGE_CURRENT,0,DT_ID_PARAM_BMS_MAX_DISCHARGE_CURRENT);
    //u8_mModulesCntDischarge=1;

    //Maximalen Entladestrom aus den einzelnen Packs errechnen
    if(inverterData.bmsDatasourceAdd>0)
    {
      int16_t maxCurrent {0};
      for(uint8_t i=0;i<SERIAL_BMS_DEVICES_COUNT;i++)
      {
        if((inverterData.bmsDatasource-BMSDATA_FIRST_DEV_SERIAL)==i || (inverterData.bmsDatasourceAdd>>i)&0x01)
        {
          #ifdef CAN_DEBUG
          BSC_LOGD(TAG,"MaxDischargeCurrent Pack: i=%i, bmsErr=%i, FETdisState=%i, time=%i",
            i,getBmsErrors(BMSDATA_FIRST_DEV_SERIAL+i),getBmsStateFETsDischarge(BMSDATA_FIRST_DEV_SERIAL+i),millis()-getBmsLastDataMillis(BMSDATA_FIRST_DEV_SERIAL+i));
          #endif

          if(getBmsErrors(BMSDATA_FIRST_DEV_SERIAL+i)==0 && (millis()-getBmsLastDataMillis(BMSDATA_FIRST_DEV_SERIAL+i)<CAN_BMS_COMMUNICATION_TIMEOUT) &&
            getBmsStateFETsDischarge(BMSDATA_FIRST_DEV_SERIAL+i))
          {
            maxCurrent+=WebSettings::getInt(ID_PARAM_BATTERY_PACK_DISCHARGE_CURRENT,i,DT_ID_PARAM_BATTERY_PACK_DISCHARGE_CURRENT);
            //u8_mModulesCntDischarge++;
          }
        }
      }

      maxDischargeCurrent = std::min(maxCurrent, maxDischargeCurrent);
    }
    #ifdef CAN_DEBUG
    BSC_LOGI(TAG,"dischargeCurrent Pack:%i",maxDischargeCurrent);
    #endif


    maxDischargeCurrentList[0] = calcEntladestromZellspanung(inverterData, maxDischargeCurrent);

    //Bestimmt kleinsten Entladestrom aller Optionen
    maxDischargeCurrent = *std::min_element(std::begin(maxDischargeCurrentList), std::end(maxDischargeCurrentList));

    newDisChargeCurrent = maxDischargeCurrent;
  }

  inverterData.inverterDischargeCurrent = newDisChargeCurrent;
  inverterData.dischargeCurrentCellVoltage = maxDischargeCurrentList[0];
}



/*******************************************************************************************************
 * Berechnet der Maximalzulässigen Entladestrom anhand der eigestellten Zellspannungsparameter
 *******************************************************************************************************/
int16_t DisChargeCurrentCtrl::calcEntladestromZellspanung(inverters::InverterData &inverterData, int16_t i16_pMaxDischargeCurrent)
{
  uint16_t u16_lStartSpg = WebSettings::getInt(ID_PARAM_INVERTER_ENTLADESTROM_REDUZIEREN_ZELLSPG_STARTSPG,0,DT_ID_PARAM_INVERTER_ENTLADESTROM_REDUZIEREN_ZELLSPG_STARTSPG);

  if(u16_lStartSpg>0) //wenn enabled
  {
    //Kleinste Zellspannung von den aktiven BMSen ermitteln
    uint16_t u16_lAktuelleMinZellspg = bms::utils::getMinCellSpannungFromBms(inverterData.bmsDatasource, inverterData.bmsDatasourceAdd);

    if(u16_lStartSpg>=u16_lAktuelleMinZellspg)
    {
      uint16_t u16_lEndSpg = WebSettings::getInt(ID_PARAM_INVERTER_ENTLADESTROM_REDUZIEREN_ZELLSPG_ENDSPG,0,DT_ID_PARAM_INVERTER_ENTLADESTROM_REDUZIEREN_ZELLSPG_ENDSPG);
      int16_t i16_lMindestChargeCurrent = (WebSettings::getInt(ID_PARAM_INVERTER_ENTLADESTROM_REDUZIEREN_ZELLSPG_MINDEST_STROM,0,DT_ID_PARAM_INVERTER_ENTLADESTROM_REDUZIEREN_ZELLSPG_MINDEST_STROM));

      if(u16_lStartSpg<=u16_lEndSpg) return i16_pMaxDischargeCurrent; //Startspannung <= Endspannung => Fehler
      if(i16_pMaxDischargeCurrent<=i16_lMindestChargeCurrent) return i16_pMaxDischargeCurrent; //Maximaler Entladestrom < Mindest-Entladestrom => Fehler

      if(u16_lAktuelleMinZellspg<u16_lEndSpg)
      {
        //Wenn die aktuelle Zellspannung bereits kleiner als die Endzellspannung ist,
        //dann Ladestrom auf Mindest-Entladestrom einstellen
        return i16_lMindestChargeCurrent;
      }
      else
      {
        uint32_t u32_lAenderungProMv = ((u16_lStartSpg-u16_lEndSpg)*100)/(i16_pMaxDischargeCurrent-i16_lMindestChargeCurrent); //Änderung pro mV
        uint32_t u32_lStromAenderung = ((u16_lStartSpg-u16_lAktuelleMinZellspg)*100)/u32_lAenderungProMv; //Ladestrom um den theoretisch reduziert werden muss
        if(u32_lStromAenderung>(i16_pMaxDischargeCurrent-i16_lMindestChargeCurrent))
        {
          return i16_lMindestChargeCurrent;
        }
        else
        {
          uint16_t u16_lNewChargeCurrent = i16_pMaxDischargeCurrent-u32_lStromAenderung; //neuer Entladestrom
          return u16_lNewChargeCurrent;
        }

      }
    }
  }
  return i16_pMaxDischargeCurrent;
}

} // namespace inverters