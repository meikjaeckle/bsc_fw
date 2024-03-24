// Copyright (c) 2024 Tobias Himmler
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#include <cctype> // isdigit
#include <cstdint>
#include <WString.h> // String

#include <json/Utils.h>
#include <web/WebDefinitions.h>
#include <web/Utils.h>

namespace web
{

bool isNumber(const String& str)
{
  for (char const &c : str) {
    if ((std::isdigit(static_cast<unsigned char>(c)) == 0) && (c != '-'))
      return false;
  }
  return true;
}

//Universal
String getJson_Key(const char *parameter, const String &key, uint8_t idx, uint32_t startPos, const String &defaultValue)
{
  String retStr;
  uint32_t retArrayStart {0};
  const bool ret = json::getValue(parameter, idx, key, startPos, retStr, retArrayStart);
  if (ret)
    return retStr;
  else
    return defaultValue;
}

uint8_t getJsonSize(const char *parameter, uint8_t idx, uint32_t startPos)
{
  String retStr;
  uint32_t retArrayStart {0};
  constexpr uint32_t DEFAULT_SIZE {1};

  const bool ret = json::getValue(parameter, idx, "size", startPos, retStr, retArrayStart);
  if (ret)
  {
    if (isNumber(retStr))
    {
      return atoi(retStr.c_str()); // TODO MEJ Is it garanteed, that retStr number is < 256
    }
    else
    {
      //Error; Sollte nicht vorkommen
      return DEFAULT_SIZE;
    }
  }
  else
  {
    return DEFAULT_SIZE;
  }
}

uint8_t getJsonGroupsize(const char *parameter, uint8_t idx, uint32_t startPos)
{
  String retStr;
  uint32_t retArrayStart {0};

  const bool ret = json::getValue(parameter, idx, "groupsize", startPos, retStr, retArrayStart);
  if (ret)
  {
    if (isNumber(retStr))
    {
      return atoi(retStr.c_str());
    }
    else
    { //Error; Sollte nicht vorkommen
      return 0;
    }
  }
  else
  {
    return 0;
  }
}

uint32_t getJsonName(const char *parameter, uint8_t idx, uint32_t startPos)
{
  String retStr;
  uint32_t retArrayStart {0};

  const bool ret = json::getValue(parameter, idx, "name", startPos, retStr, retArrayStart);
  if (ret)
    return retStr.toInt();
  else
    return 0;
}

String getJsonLabel(const char *parameter, uint8_t idx, uint32_t startPos)
{
  String retStr;
  uint32_t retArrayStart {0};

  const bool ret = json::getValue(parameter, idx, "label", startPos, retStr, retArrayStart);
  if (ret)
    return retStr;
  else
    return emptyString;
}

String getJsonLabelEntry(const char *parameter, uint8_t idx, uint32_t startPos)
{
  String retStr;
  uint32_t retArrayStart {0};

  const bool ret = json::getValue(parameter, idx, "label_entry", startPos, retStr, retArrayStart);
  if (ret)
    return retStr;
  else
    return emptyString;
}

String getJsonHelp(const char *parameter, uint8_t idx, uint32_t startPos)
{
  String retStr;
  uint32_t retArrayStart {0};

  const bool ret = json::getValue(parameter, idx, "help", startPos, retStr, retArrayStart);
  if (ret == true)
    return retStr;
  else
    return emptyString;
}

uint8_t getJsonType(const char *parameter, uint8_t idx, uint32_t startPos)
{
  String retStr;
  uint32_t retArrayStart {0};

  const bool ret = json::getValue(parameter, idx, "type", startPos, retStr, retArrayStart);
  if (ret)
  {
    if (isNumber(retStr))
    {
      return atoi(retStr.c_str());
    }
    else
    { //Error; Sollte nicht vorkommen
      return HTML_INPUTTEXT;
    }
  }
  else
  {
    return HTML_INPUTTEXT;
  }
}

String getJsonDefault(const char *parameter, uint8_t idx, uint32_t startPos)
{
  String retStr;
  uint32_t retArrayStart {0};

  const bool ret = json::getValue(parameter, idx, "default", startPos, retStr, retArrayStart);
  if (ret)
    return retStr;
  else
    return String("0");
}

std::vector<String> getJsonOptionValues(const char *parameter, uint8_t idx, uint32_t startPos)
{
  std::vector<String> options;
  String retStr;
  uint32_t retArrayStart {0};

  bool ret = json::getValue(parameter, idx, "options", startPos, retStr, retArrayStart);
  if (ret)
  {
    for (uint8_t i {0}; i < json::getArraySize(parameter, retArrayStart); ++i)
    {
      uint32_t retArrayStart2 {0};
      String option;
      ret = json::getValue(parameter, i, "v", retArrayStart, option, retArrayStart2);
      if (ret)
        options.push_back(std::move(option));
    }
  }

  return options;
}

std::vector<String> getJsonOptionLabels(const char *parameter, uint8_t idx, uint32_t startPos)
{
  std::vector<String> labels;
  String retStr;
  uint32_t retArrayStart {0};

  bool ret = json::getValue(parameter, idx, "options", startPos, retStr, retArrayStart);
  if (ret)
  {
    for (uint8_t i {0}; i< json::getArraySize(parameter, retArrayStart); ++i)
    {
      uint32_t retArrayStart2 {0};
      String label;
      ret = json::getValue(parameter, i, "l", retArrayStart, label, retArrayStart2);
      if (ret)
        labels.push_back(std::move(label));
    }
  }
  return labels;
}

String getJsonArrValue(const char *parameter, const String &str_key1, const String &str_key2, const uint8_t u8_eCnt, const uint8_t idx, const uint32_t startPos)
{
  String retStr;
  uint32_t retArrayStart {0};

  bool ret = json::getValue(parameter, idx, str_key1, startPos, retStr, retArrayStart);
  if (ret)
  { // TODO MEJ Check this method => why is retArrayStart2 not used
    for (uint8_t i {0}; i < json::getArraySize(parameter, retArrayStart); ++i)
    {
      retStr.clear();
      uint32_t retArrayStart2 {0};
      ret = json::getValue(parameter, i, str_key2, retArrayStart, retStr, retArrayStart2);
      if (u8_eCnt == i)
        return retStr;
    }
  }

  return retStr;
}

uint8_t getJsonOptionsCnt(const char *parameter, const uint8_t idx, const uint32_t startPos)
{
  String retStr;
  uint32_t retArrayStart {0};

  const bool ret = json::getValue(parameter, idx, "options", startPos, retStr, retArrayStart);
  if (ret == true)
    return static_cast<uint8_t>(json::getArraySize(parameter, retArrayStart));
  else
    return 0;
}

// TODO MEJ Check why return is uint32_t here and uint8_t at getJsonOptionsCnt()
uint32_t getJsonOptionsMin(const char *parameter, const uint8_t idx, const uint32_t startPos)
{
  String retStr;
  uint32_t retArrayStart {0};
  constexpr uint32_t DEFAULT_VALUE_MIN {0};

  const bool ret = json::getValue(parameter, idx, "min", startPos, retStr, retArrayStart);
  if (ret)
  {
    if (isNumber(retStr))
    {
      return atoi(retStr.c_str());
    }
    else
    { //Error; Sollte nicht vorkommen
      return DEFAULT_VALUE_MIN;
    }
  }
  else
  {
    return DEFAULT_VALUE_MIN;
  }
}

// TODO MEJ Check why return is uint32_t here and uint8_t at getJsonOptionsCnt()
uint32_t getJsonOptionsMax(const char *parameter, const uint8_t idx, const uint32_t startPos)
{
  String retStr;
  uint32_t retArrayStart {0};
  constexpr uint32_t DEFAULT_VALUE_MAX {100};

  const bool ret = json::getValue(parameter, idx, "max", startPos, retStr, retArrayStart);
  if (ret)
  {
    if (isNumber(retStr))
    {
      return atoi(retStr.c_str());
    }
    else //Error; Sollte nicht vorkommen
    {
      return DEFAULT_VALUE_MAX;
    }
  }
  else
  {
    return DEFAULT_VALUE_MAX;
  }
}

} // namespace web

