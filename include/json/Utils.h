// Copyright (c) 2022 Tobias Himmler
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#ifndef JSON_UTILS_H
#define JSON_UTILS_H

#include <cstdint>
#include <WString.h> // String

namespace json
{

uint16_t getArraySize(const char *json, long startPos);
bool getValue(const char *json, int idx, const String &name, uint32_t searchStartPos, String& retValue, uint32_t& arrayStart);

} // namespace json

#endif // JSON_UTILS_H