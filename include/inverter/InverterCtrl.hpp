// Copyright (c) 2024 Tobias Himmler
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#pragma once

#include <cstdint>
#include <inverter/DataAdapter.hpp>
#include <inverter/Inverter.hpp>
#include <inverter/InverterData.hpp>
#include <inverter/InverterDataAdapter.hpp>
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


namespace inverter
{

class Canbus; // forward declaration

class InverterCtrl :
  public Inverter
{
  public:
  InverterCtrl(Canbus& can);
  ~InverterCtrl();

  public:
  void init();
  void loadInverterSettings();
  void cyclicRun();


  // Inverter interface methods
  public:
  /** @returns the reference to the DataAdapter */
  inline const DataAdapter& getDataAdapter() const override { return _dataReadAdapter; }
  void setChargeCurrentToZero(bool val) override;
  void setDischargeCurrentToZero(bool val) override;
  void setSocToFull(bool val) override;

  private:
  void inverterDataSemaphoreTake();
  void inverterDataSemaphoreGive();
  void updateInverterValues();
  void sendMqttMsg(); // TODO MEJ I would move this method out of the inverter class. This just takes another dependency but mqtt is not the main task of the InverterCtrl.

  private:
  mutable SemaphoreHandle_t _inverterDataMutex; // TODO MEJ still required???
  Canbus& _bmsCan;

  InverterData _inverterData;
  InverterDataAdapter _dataReadAdapter;
  uint8_t _mMqttTxTimer;
  bool _alarmSetChargeCurrentToZero;
  bool _alarmSetDischargeCurrentToZero;
  bool _alarmSetSocToFull;
};
} // namespace inverter