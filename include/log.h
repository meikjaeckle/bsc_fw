// Copyright (c) 2022 Tobias Himmler
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#ifndef LOG_H
#define LOG_H

#include <cstdint>

namespace inverters
{
class IDataReadAdapter;
}

void debugInit();

#ifdef DEBUG_ON_FS
void writeLogToFS();
#endif
void deleteLogfile();
void logTrigger(uint8_t triggerNr, uint8_t cause, bool trigger);
void logValues(const inverters::IDataReadAdapter& dataAdapter);

void fsLock();
void fsUnlock();
bool isFsLock();

#endif