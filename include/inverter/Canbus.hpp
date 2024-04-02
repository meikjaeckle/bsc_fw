// Copyright (c) 2024 Tobias Himmler
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#pragma once

#include <cstdint>
#include <inverter/InverterData.hpp>

namespace inverter
{
class Canbus
{
  public:
  using CanId = uint32_t;

  public:
  void init();
  void readCanMessages();
  void sendBmsCanMessages(const inverter::InverterData& inverterData);

  private:
  struct data351
  {
    uint16_t chargevoltagelimit;   // CVL
    int16_t  maxchargecurrent;     // CCL
    int16_t  maxDischargeCurrent;  // DCL
    uint16_t dischargevoltage;     // DVL
  };

  struct data355
  {
    uint16_t soc; //state of charge
    uint16_t soh; //state of health
  };

  struct data356
  {
    int16_t voltage;
    int16_t current;
    int16_t temperature;
  };

  struct data35a
  {
    uint8_t u8_b0;
    uint8_t u8_b1;
    uint8_t u8_b2;
    uint8_t u8_b3;
    uint8_t u8_b4;
    uint8_t u8_b5;
    uint8_t u8_b6;
    uint8_t u8_b7;
  };

  struct data373
  {
    uint16_t minCellColtage;
    uint16_t maxCellVoltage;
    uint16_t minCellTemp;
    uint16_t maxCellTemp;
  };

  enum class BmsCanId : CanId
  {
    MIN_CELL_VOLTAGE_TEXT = 0x374,
    MAX_CELL_VOLTAGE_TEXT = 0x375,
  };
  inline CanId FromBmsCanId(BmsCanId bmsCanId) const { return static_cast<CanId>(bmsCanId);}

  void sendCanMsg_ChgVoltCur_DisChgCur_351(const InverterData& inverterData);
  void sendCanMsg_soc_soh_355(const InverterData& inverterData);
  void sendCanMsg_Battery_Voltage_Current_Temp_356(const InverterData& inverterData);
  void sendCanMsg_Alarm_359(const InverterData& inverterData);
  void sendCanMsg_Alarm_35a(const InverterData& inverterData);
  void sendCanMsg_hostname_35e_370_371();
  void sendCanMsg_version_35f();
  void sendCanMsg_battery_modules_372(const InverterData& inverterData);
  void sendCanMsg_min_max_values_373_376_377(const InverterData& inverterData);
  // sends the min or max cell voltage text, depending on the given CanID (min: 374; max: 375)
  void sendCanMsg_min_max_CellVoltage_text(const InverterData& inverterData, BmsCanId canId);
  void sendCanMsg_minCellVoltage_text_374(const InverterData& inverterData) { sendCanMsg_min_max_CellVoltage_text(inverterData, BmsCanId::MIN_CELL_VOLTAGE_TEXT); }
  void sendCanMsg_maxCellVoltage_text_375(const InverterData& inverterData) { sendCanMsg_min_max_CellVoltage_text(inverterData, BmsCanId::MAX_CELL_VOLTAGE_TEXT); }

  void sendCanMsg(uint32_t identifier, const uint8_t* buffer, uint8_t length);
  void sendExtendedCanMsgTemp();
  void sendExtendedCanMsgBmsData();

  void onCanReceive(int packetSize);
};
} // namespace inverter