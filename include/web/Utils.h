// Copyright (c) 2024 Tobias Himmler
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#ifndef WEB_UTILS_H
#define WEB_UTILS_H

#include <cstdint>
#include <vector>
#include <WString.h> // String

namespace web
{

bool isNumber(const String& str);
uint16_t getParmId(uint16_t id, uint8_t groupIdx);

// Json helper methods for WebSettings

  String   getJson_Key(const char *parameter, const String &key, uint8_t idx, uint32_t startPos, const String &defaultValue); //Universal über parameter key
  uint8_t  getJsonSize(const char *parameter, uint8_t idx, uint32_t startPos);
  uint8_t  getJsonGroupsize(const char *parameter, uint8_t idx, uint32_t startPos);
  uint32_t getJsonName(const char *parameter, uint8_t idx, uint32_t startPos);
  String   getJsonLabel(const char *parameter, uint8_t idx, uint32_t startPos);
  String   getJsonLabelEntry(const char *parameter, uint8_t idx, uint32_t startPos);
  String   getJsonHelp(const char *parameter, uint8_t idx, uint32_t startPos);
  uint8_t  getJsonType(const char *parameter, uint8_t idx, uint32_t startPos);
  String   getJsonDefault(const char *parameter, uint8_t idx, uint32_t startPos);
  uint8_t  getJsonOptionsCnt(const char *parameter, uint8_t idx, uint32_t startPos);
  uint32_t getJsonOptionsMin(const char *parameter, uint8_t idx, uint32_t startPos);
  uint32_t getJsonOptionsMax(const char *parameter, uint8_t idx, uint32_t startPos);
  std::vector<String> getJsonOptionValues(const char *parameter, uint8_t idx, uint32_t startPos);
  std::vector<String> getJsonOptionLabels(const char *parameter, uint8_t idx, uint32_t startPos);
  String   getJsonArrValue(const char *parameter, const String &str_key1, const String &str_key2, uint8_t u8_eCnt, uint8_t idx, uint32_t startPos);

} // namespace web

#endif // WEB_UTILS_H