// Copyright (c) 2024 Tobias Himmler
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#include "inverter/InverterBattery.hpp"
#include "inverter/BmsDataUtils.hpp"
#include "defines.h"
#include "WebSettings.h"

namespace inverters
{
  void InverterBattery::getBatteryVoltage(Inverter &inverter, InverterData &inverterData)
  {
    if((millis()-getBmsLastDataMillis(inverterData.u8_bmsDatasource))<CAN_BMS_COMMUNICATION_TIMEOUT)
    {
      inverter.inverterDataSemaphoreTake();
      inverterData.batteryVoltage = (int16_t)(getBmsTotalVoltage(inverterData.u8_bmsDatasource)*100.0f);
      inverter.inverterDataSemaphoreGive();
      return;
    }
    else //Wenn Masterquelle offline, dann nächstes BMS nehmen das online ist
    {
      for(uint8_t i=0;i<SERIAL_BMS_DEVICES_COUNT;i++)
      {
        //Wenn BMS ausgewählt und die letzten 5000ms Daten kamen
        if(((inverterData.u16_bmsDatasourceAdd>>i)&0x01) && ((millis()-getBmsLastDataMillis(BMSDATA_FIRST_DEV_SERIAL+i))<CAN_BMS_COMMUNICATION_TIMEOUT))
        {
          inverter.inverterDataSemaphoreTake();
          inverterData.batteryVoltage = (int16_t)(getBmsTotalVoltage(BT_DEVICES_COUNT+i)*100.0f);
          inverter.inverterDataSemaphoreGive();
          return;
        }
      }
    }

    inverter.inverterDataSemaphoreTake();
    inverterData.batteryVoltage = 0;
    inverter.inverterDataSemaphoreGive();
  }


  void InverterBattery::getBatteryCurrent(Inverter &inverter, InverterData &inverterData)
  {
    bool isOneBatteryPackOnline = false;
    int16_t u16_lBatteryCurrent = (int16_t)(getBmsTotalCurrent(inverterData.u8_bmsDatasource)*10.0f);

    if((millis()-getBmsLastDataMillis(inverterData.u8_bmsDatasource))<CAN_BMS_COMMUNICATION_TIMEOUT) isOneBatteryPackOnline=true;
    #ifdef CAN_DEBUG
    BSC_LOGI(TAG,"Battery current: u8_mBmsDatasource=%i, cur=%i, u8_mBmsDatasourceAdd=%i",u8_mBmsDatasource, msgData.current, u8_mBmsDatasourceAdd);
    #endif

    //Wenn zusätzliche Datenquellen angegeben sind:
    for(uint8_t i=0;i<SERIAL_BMS_DEVICES_COUNT;i++)
    {
      #ifdef CAN_DEBUG
      long lTime = getBmsLastDataMillis(BMSDATA_FIRST_DEV_SERIAL+i);
      #endif
      //Wenn BMS ausgewählt und die letzten 5000ms Daten kamen
      if(((inverterData.u16_bmsDatasourceAdd>>i)&0x01) && ((millis()-getBmsLastDataMillis(BMSDATA_FIRST_DEV_SERIAL+i))<CAN_BMS_COMMUNICATION_TIMEOUT))
      {
        isOneBatteryPackOnline=true;
        u16_lBatteryCurrent += (int16_t)(getBmsTotalCurrent(BT_DEVICES_COUNT+i)*10.0f);
        #ifdef CAN_DEBUG
        BSC_LOGI(TAG,"Battery current (T): dev=%i, time=%i, cur=%i",i,millis()-lTime, msgData.current);
        #endif
      }
      #ifdef CAN_DEBUG
      else
      {
        BSC_LOGI(TAG,"Battery current (F): dev=%i, time1=%i, time2=%i, cur=%i",i,millis()-lTime,lTime,msgData.current);
      }
      #endif
    }

    inverter.inverterDataSemaphoreTake();
    inverterData.noBatteryPackOnline = !isOneBatteryPackOnline;
    inverterData.batteryCurrent = u16_lBatteryCurrent;
    inverter.inverterDataSemaphoreGive();
  }


  int16_t InverterBattery::getBatteryTemp(InverterData &inverterData)
  {
    uint8_t u8_lBmsTempQuelle=WebSettings::getInt(ID_PARAM_INVERTER_BATT_TEMP_QUELLE,0,DT_ID_PARAM_INVERTER_BATT_TEMP_QUELLE);
    uint8_t u8_lBmsTempSensorNr=WebSettings::getInt(ID_PARAM_INVERTER_BATT_TEMP_SENSOR,0,DT_ID_PARAM_INVERTER_BATT_TEMP_SENSOR);
    if(u8_lBmsTempQuelle==1)
    {
      if(u8_lBmsTempSensorNr<3)
      {
        return (int16_t)(getBmsTempature(inverterData.u8_bmsDatasource,u8_lBmsTempSensorNr));
      }
      else
      {
        return (int16_t)(getBmsTempature(inverterData.u8_bmsDatasource,0)*10); //Im Fehlerfall immer Sensor 0 des BMS nehmen
      }
    }
    else if(u8_lBmsTempQuelle==2)
    {
      if(u8_lBmsTempSensorNr<MAX_ANZAHL_OW_SENSOREN)
      {
        return (int16_t)(owGetTemp(u8_lBmsTempSensorNr));
      }
      else
      {
        return (int16_t)(getBmsTempature(inverterData.u8_bmsDatasource,0)); //Im Fehlerfall immer Sensor 0 des BMS nehmen
      }
    }
    else
    {
      return (int16_t)(getBmsTempature(inverterData.u8_bmsDatasource,0));  //Im Fehlerfall immer Sensor 0 des BMS nehmen
    }
  }
} // namespace inverters