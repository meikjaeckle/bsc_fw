// Copyright (c) 2024 Tobias Himmler
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#pragma once

#include <cstdint>
#include <inverters/DataReadAdapter.hpp>
#include <inverters/IDataReadAdapter.hpp>
#include <inverters/IInverterControl.hpp>
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

class Inverter :
  public IInverterControl
{
  public:
  Inverter(Canbus& can);
  ~Inverter();

  public:
  void init();
  void loadInverterSettings();
  void cyclicRun();


  // IInverterControl interface methods
  public:
  /** @returns the reference to the IDataReadAdapter */
  inline const IDataReadAdapter& getDataReadAdapter() const override { return _dataReadAdapter; }
  void setChargeCurrentToZero(bool val) override;
  void setDischargeCurrentToZero(bool val) override;
  void setSocToFull(bool val) override;

  private:
  void inverterDataSemaphoreTake();
  void inverterDataSemaphoreGive();
  void updateInverterValues();
  void sendMqttMsg(); // TODO MEJ I would move this method out of the inverter class. This just takes another dependency but mqtt is not the main task of the Inverter.

  private:
  mutable SemaphoreHandle_t _inverterDataMutex; // TODO MEJ still required???
  Canbus& _bmsCan;

  InverterData _inverterData;
  DataReadAdapter _dataReadAdapter;
  uint8_t _mMqttTxTimer;
  bool _alarmSetChargeCurrentToZero;
  bool _alarmSetDischargeCurrentToZero;
  bool _alarmSetSocToFull;
};
} // namespace inverters