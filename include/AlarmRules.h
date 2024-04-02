// Copyright (c) 2022 tobias
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT


#ifndef ALARMRULES_H
#define ALARMRULES_H

#include <cstdint>

namespace inverter
{
class Inverter;
}

void initAlarmRules(inverter::Inverter& /* inverter */);
void runAlarmRules(inverter::Inverter& inverter);
void changeAlarmSettings();

bool getAlarm(uint8_t alarmNr);
uint16_t getAlarm();
bool isTriggerActive(uint16_t paramId, uint8_t groupNr, uint8_t dataType);

bool setVirtualTrigger(uint8_t triggerNr, bool val);

#endif