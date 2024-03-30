// Copyright (c) 2024 Tobias Himmler
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#pragma once

#include <cstdint>
#include <inverters/InverterData.hpp>
#ifndef PIO_UNIT_TESTING
#include "WebSettings.h"
#endif
#include "defines.h"
#include "BmsData.h"
#ifndef PIO_UNIT_TESTING
#include "Ow.h"
#include "mqtt_t.h"
#include "log.h"
#include "AlarmRules.h"
#endif


namespace inverters
{

class Canbus; // forward declaration

class Inverter
{
  public:
  Inverter(Canbus& can);
  ~Inverter();

  void init();
  void loadInverterSettings();
  void cyclicRun();

  void inverterDataSemaphoreTake();
  void inverterDataSemaphoreGive();
  inline const InverterData& getInverterData() const { return _inverterData; }
  InverterData& getInverterData() { return _inverterData; }

  void setChargeCurrentToZero(bool val);
  void setDischargeCurrentToZero(bool val);
  void setSocToFull(bool val);

  uint16_t getAktualChargeCurrentSoll() const;

  private:
  void getInverterValues();
  void sendMqttMsg();

  private:
  mutable SemaphoreHandle_t _inverterDataMutex;
  Canbus& _bmsCan;

  InverterData _inverterData;
  uint8_t _mMqttTxTimer;
  bool _alarmSetChargeCurrentToZero;
  bool _alarmSetDischargeCurrentToZero;
  bool _alarmSetSocToFull;
};
} // namespace inverters