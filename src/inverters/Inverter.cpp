// Copyright (c) 2024 Tobias Himmler
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#include <freertos/FreeRTOS.h>
#include "inverter/Inverter.hpp"
#include "inverter/BmsDataUtils.hpp"
#include "inverter/ChargeCurrentCtrl.hpp"
#include "inverter/DisChargeCurrentCtrl.hpp"
#include "inverter/ChargeVoltageCtrl.hpp"
#include "inverter/SocCtrl.hpp"
#include "inverter/InverterBattery.hpp"
#include "inverter/Canbus.hpp"

#include "WebSettings.h"
#include "defines.h"
#include "BmsData.h"
#include "Ow.h"
#include "mqtt_t.h"
#include "log.h"
#include "AlarmRules.h"

namespace inverters
{

namespace // anonymous
{
constexpr const char TAG[] = "Inverter";
}

Inverter::Inverter(Canbus& can) :
  _inverterDataMutex {xSemaphoreCreateMutex()},
  _bmsCan {can},
  _mMqttTxTimer{0},
  _alarmSetChargeCurrentToZero{false},
  _alarmSetDischargeCurrentToZero{false},
  _alarmSetSocToFull{false}
{
  assert(_inverterDataMutex);
}

Inverter::~Inverter()
{
  vSemaphoreDelete(_inverterDataMutex);
}

void Inverter::init()
{
  //u8_mBmsDatasource=0;
  _alarmSetChargeCurrentToZero=false;
  _alarmSetDischargeCurrentToZero=false;
  _alarmSetSocToFull=false;

  _inverterData.u16_mSocZellspannungSperrzeitTimer = 0;
  _inverterData.socZellspannungState = SocZellspgStates::STATE_MINCELLSPG_SOC_WAIT_OF_MIN;

  loadInverterSettings();
  _bmsCan.init();
}


void Inverter::inverterDataSemaphoreTake()
{
  xSemaphoreTake(_inverterDataMutex, portMAX_DELAY);
}


void Inverter::inverterDataSemaphoreGive()
{
  xSemaphoreGive(_inverterDataMutex);
}

void Inverter::loadInverterSettings()
{
  _inverterData.noBatteryPackOnline = true;
  const uint8_t u8_bmsDatasource = WebSettings::getInt(ID_PARAM_BMS_CAN_DATASOURCE,0,DT_ID_PARAM_BMS_CAN_DATASOURCE);
  const uint8_t u8_lNumberOfSerial2BMSs = WebSettings::getInt(ID_PARAM_SERIAL2_CONNECT_TO_ID,0,DT_ID_PARAM_SERIAL2_CONNECT_TO_ID);

  uint32_t bmsConnectFilter {0};
  /*for(uint8_t i;i<BT_DEVICES_COUNT;i++)
  {
    if(WebSettings::getInt(ID_PARAM_SS_BTDEV,i,DT_ID_PARAM_SS_BTDEV)!=0)
    {
      bmsConnectFilter |= (1<<i);
    }
  }*/
  for(uint8_t i;i<SERIAL_BMS_DEVICES_COUNT;i++)
  {
    if(WebSettings::getInt(ID_PARAM_SERIAL_CONNECT_DEVICE,i,DT_ID_PARAM_SERIAL_CONNECT_DEVICE)!=0)
      bmsConnectFilter |= (1<<i);
    else if(i>=3 && u8_lNumberOfSerial2BMSs-i+2>0)
      bmsConnectFilter |= (1<<i); //Seplos BMS berücksichtigen
  }

  uint16_t u16_bmsDatasourceAdd =(((uint32_t)WebSettings::getInt(ID_PARAM_BMS_CAN_DATASOURCE_SS1,0,DT_ID_PARAM_BMS_CAN_DATASOURCE_SS1)) & bmsConnectFilter);

  // In den zusätzlichen Datenquellen die Masterquelle entfernen
  if(u8_bmsDatasource>=BT_DEVICES_COUNT)
    bitClear(u16_bmsDatasourceAdd,u8_bmsDatasource-BT_DEVICES_COUNT);

  _inverterData.u8_bmsDatasource = u8_bmsDatasource;
  _inverterData.u16_bmsDatasourceAdd = u16_bmsDatasourceAdd;

  BSC_LOGI(TAG,"loadIverterSettings(): dataSrcAdd=%i, u8_mBmsDatasource=%i, bmsConnectFilter=%i, u8_mBmsDatasourceAdd=%i",WebSettings::getInt(ID_PARAM_BMS_CAN_DATASOURCE_SS1,0,DT_ID_PARAM_BMS_CAN_DATASOURCE_SS1),u8_bmsDatasource,bmsConnectFilter, u16_bmsDatasourceAdd);
}


//Ladeleistung auf 0 einstellen
void Inverter::setChargeCurrentToZero(bool val)
{
  _alarmSetChargeCurrentToZero = val;
}


//Entladeleistung auf 0 einstellen
void Inverter::setDischargeCurrentToZero(bool val)
{
  _alarmSetDischargeCurrentToZero = val;
}


//SOC auf 100 einstellen
void Inverter::setSocToFull(bool val)
{
  _alarmSetSocToFull = val;
}


//Wird vom Task aus der main.c zyklisch aufgerufen
void Inverter::cyclicRun()
{
  if(WebSettings::getBool(ID_PARAM_BMS_CAN_ENABLE,0))
  {
    _mMqttTxTimer++;
    _bmsCan.readCanMessages(_inverterData);

    getInverterValues();
    _bmsCan.sendBmsCanMessages(_inverterData);

    sendMqttMsg();

    if(_mMqttTxTimer >= 15)
      _mMqttTxTimer=0;
  }
  else
    _inverterData.noBatteryPackOnline = true;
}


void Inverter::getInverterValues()
{
  // Ladespannung
  ChargeVoltageCtrl chargeVoltageCtrl;
  chargeVoltageCtrl.calcChargeVoltage(*this, _inverterData);

  // Ladestrom
  ChargeCurrentCtrl chargeCurrentCtl;
  chargeCurrentCtl.calcChargCurrent(*this, _inverterData, _alarmSetChargeCurrentToZero);

  // Entladestrom
  DisChargeCurrentCtrl disChargeCurrentCtl;
  disChargeCurrentCtl.calcDisChargCurrent(*this, _inverterData, _alarmSetDischargeCurrentToZero);

  // SoC
  SocCtrl socCtrl;
  socCtrl.calcSoc(*this, _inverterData, _alarmSetSocToFull);

  // Batteriespannung
  InverterBattery inverterBattery;
  inverterBattery.getBatteryVoltage(*this, _inverterData);

  // Batteriestrom
  inverterBattery.getBatteryCurrent(*this, _inverterData);
}


void Inverter::sendMqttMsg()
{
  if(_mMqttTxTimer == 15)
  {
    mqttPublish(MQTT_TOPIC_INVERTER, -1, MQTT_TOPIC2_INVERTER_CHARGE_VOLTAGE, -1, (float)(_inverterData.inverterChargeVoltage));
    mqttPublish(MQTT_TOPIC_INVERTER, -1, MQTT_TOPIC2_CHARGE_CURRENT_SOLL, -1, _inverterData.inverterChargeCurrent);
    mqttPublish(MQTT_TOPIC_INVERTER, -1, MQTT_TOPIC2_DISCHARGE_CURRENT_SOLL, -1, _inverterData.inverterDischargeCurrent);
    mqttPublish(MQTT_TOPIC_INVERTER, -1, MQTT_TOPIC2_CHARGE_PERCENT, -1, _inverterData.inverterSoc);

    mqttPublish(MQTT_TOPIC_INVERTER, -1, MQTT_TOPIC2_TOTAL_VOLTAGE, -1, (float)(_inverterData.batteryVoltage));
    mqttPublish(MQTT_TOPIC_INVERTER, -1, MQTT_TOPIC2_TOTAL_CURRENT, -1, (float)(_inverterData.batteryCurrent));


    InverterBattery inverterBattery;
    int16_t i16_lBattTemp = inverterBattery.getBatteryTemp(_inverterData);
    mqttPublish(MQTT_TOPIC_INVERTER, -1, MQTT_TOPIC2_TEMPERATURE, -1, (float)(i16_lBattTemp));
  }
}

} // namespace inverters