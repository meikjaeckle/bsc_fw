// Copyright (c) 2024 Tobias Himmler
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#include "BmsData.h"
#include "defines.h"

#include "WebSettings.h"
#include <bms/utils/BmsDataUtils.hpp>
#include <inverter/InverterData.hpp>
#include <inverter/SocCtrl.hpp>

namespace inverter
{
  void SocCtrl::calcSoc(inverter::InverterData &inverterData, bool alarmSetSocToFull)
  {
    uint16_t u16_lNewSoc=0;

    if(alarmSetSocToFull)
    {
      #ifdef CAN_DEBUG
      BSC_LOGD(TAG,"SOC aufgrund von Alarm auf 100%");
      #endif
      u16_lNewSoc = 100;
    }
    else
    {
      uint8_t u8_numberOfSocs = 0;
      uint16_t u16_avgSoc = 0;
      u16_lNewSoc = 0;

      if((millis()-getBmsLastDataMillis(inverterData.bmsDatasource))<CAN_BMS_COMMUNICATION_TIMEOUT)
      {
        u16_lNewSoc = u16_avgSoc = getBmsChargePercentage(inverterData.bmsDatasource); // SOC, uint16 1 %
        u8_numberOfSocs++;
      }

      uint8_t u8_lMultiBmsSocHandling = WebSettings::getInt(ID_PARAM_INVERTER_MULTI_BMS_VALUE_SOC,0,DT_ID_PARAM_INVERTER_MULTI_BMS_VALUE_SOC);

      if(inverterData.bmsDatasourceAdd>0 && (u8_lMultiBmsSocHandling==OPTION_MULTI_BMS_SOC_AVG || u8_lMultiBmsSocHandling==OPTION_MULTI_BMS_SOC_MAX))
      {
        for(uint8_t i=0;i<SERIAL_BMS_DEVICES_COUNT;i++)
        {
          if(isBitSet(inverterData.bmsDatasourceAdd,i))
          {
            if((millis()-getBmsLastDataMillis(BMSDATA_FIRST_DEV_SERIAL+i))<CAN_BMS_COMMUNICATION_TIMEOUT) //So lang die letzten 5000ms Daten kamen ist alles gut
            {
              if(u8_lMultiBmsSocHandling==OPTION_MULTI_BMS_SOC_AVG)
              {
                u16_avgSoc+=getBmsChargePercentage(BMSDATA_FIRST_DEV_SERIAL+i);
                u8_numberOfSocs++;
              }
              else if(u8_lMultiBmsSocHandling==OPTION_MULTI_BMS_SOC_MAX)
              {
                if(getBmsChargePercentage(BMSDATA_FIRST_DEV_SERIAL+i)>u16_lNewSoc) u16_lNewSoc=getBmsChargePercentage(BMSDATA_FIRST_DEV_SERIAL+i);
              }
            }
          }
        }

        if ((u8_numberOfSocs > 0) && // Prevents from divide-by-zero
            (u8_lMultiBmsSocHandling==OPTION_MULTI_BMS_SOC_AVG))
        {
          u16_lNewSoc = (u16_avgSoc / u8_numberOfSocs);
        }
      }
      else if(u8_lMultiBmsSocHandling==OPTION_MULTI_BMS_SOC_BMS) // Wenn SoC durch ein bestimmtes BMS geregelt werden soll
      {
        uint8_t u8_lSocBmsNr = WebSettings::getInt(ID_PARAM_INVERTER_BMS_QUELLE_SOC,0,DT_ID_PARAM_INVERTER_BMS_QUELLE_SOC);

        if((millis()-getBmsLastDataMillis(u8_lSocBmsNr))<CAN_BMS_COMMUNICATION_TIMEOUT) //So lang die letzten 5000ms Daten kamen ist alles gut
        {
          u16_lNewSoc = getBmsChargePercentage(u8_lSocBmsNr);
        }
      }

      if(WebSettings::getBool(ID_PARAM_INVERTER_SOC_BELOW_ZELLSPANNUNG_EN,0)==true)
      {
        //Wenn Zellspannung unterschritten wird, dann SoC x an Inverter senden
        u16_lNewSoc = getNewSocByMinCellVoltage(inverterData, u16_lNewSoc);
      }
      else
        inverterData.socZellspannungState = STATE_MINCELLSPG_SOC_WAIT_OF_MIN;
    }

    //inverterData.inverterChargeCurrent = i16_mNewChargeCurrent;
    inverterData.inverterSoc = u16_lNewSoc;
  }


  /* *******************************************************************************************
  * getNewSocByMinCellVoltage()
  * Wenn die eingestellte mindest-Zellspannung unterschritten wird, dann kann ein belibiger SoC
  * an den Wechselrichter gesendet werden. Hiermit kann ein Nachladen erzwungen werden.
  * *******************************************************************************************/
  uint8_t SocCtrl::getNewSocByMinCellVoltage(inverter::InverterData &inverterData, uint8_t u8_lSoc)
  {
    //Wenn Zellspannung unterschritten wird, dann SoC x% an Inverter senden
    switch(inverterData.socZellspannungState)
    {
      //Warte bis Zellspannung kleiner Mindestspannung
      case SocZellspgStates::STATE_MINCELLSPG_SOC_WAIT_OF_MIN:
      {
        if(bms::utils::getMinCellSpannungFromBms(inverterData.bmsDatasource, inverterData.bmsDatasourceAdd)<=WebSettings::getInt(ID_PARAM_INVERTER_SOC_BELOW_ZELLSPANNUNG_SPG,0,DT_ID_PARAM_INVERTER_SOC_BELOW_ZELLSPANNUNG_SPG))
        {
          inverterData.socZellspannungState = SocZellspgStates::STATE_MINCELLSPG_SOC_BELOW_MIN;
        }
      } break;

      //Spannung war kleiner als Mindestspannung
      case SocZellspgStates::STATE_MINCELLSPG_SOC_BELOW_MIN:
      {
        uint16_t u16_lZellspgChargeEnd;
        u16_lZellspgChargeEnd = WebSettings::getInt(ID_PARAM_INVERTER_SOC_BELOW_ZELLSPANNUNG_SPG_END,0,DT_ID_PARAM_INVERTER_SOC_BELOW_ZELLSPANNUNG_SPG_END);
        //Wenn Parameter ID_PARAM_INVERTER_SOC_BELOW_ZELLSPANNUNG_SPG_END 0 ist, dann Ladestartspannung nehmen
        if(u16_lZellspgChargeEnd == 0)
        {
          u16_lZellspgChargeEnd=WebSettings::getInt(ID_PARAM_INVERTER_SOC_BELOW_ZELLSPANNUNG_SPG,0,DT_ID_PARAM_INVERTER_SOC_BELOW_ZELLSPANNUNG_SPG);
        }

        if(bms::utils::getMinCellSpannungFromBms(inverterData.bmsDatasource, inverterData.bmsDatasourceAdd)>u16_lZellspgChargeEnd)
        {
          inverterData.socZellspannungSperrzeitTimer=WebSettings::getInt(ID_PARAM_INVERTER_SOC_BELOW_ZELLSPANNUNG_TIME,0,DT_ID_PARAM_INVERTER_SOC_BELOW_ZELLSPANNUNG_TIME);
          inverterData.socZellspannungState = SocZellspgStates::STATE_MINCELLSPG_SOC_LOCKTIMER;
        }
        u8_lSoc=WebSettings::getInt(ID_PARAM_INVERTER_SOC_BELOW_ZELLSPANNUNG_SOC,0,DT_ID_PARAM_INVERTER_SOC_BELOW_ZELLSPANNUNG_SOC);
      } break;

      //Sperrzeit läuft, warte auf ablauf der Sperrezit
      case SocZellspgStates::STATE_MINCELLSPG_SOC_LOCKTIMER:
      {
        inverterData.socZellspannungSperrzeitTimer--; // TODO MEJ Check for overflow
        if(inverterData.socZellspannungSperrzeitTimer==0)
        {
          inverterData.socZellspannungState = SocZellspgStates::STATE_MINCELLSPG_SOC_WAIT_OF_MIN;
        }
      } break;

      //default:
      //  break;
    }

    return u8_lSoc;
  }
} // namespace inverter
