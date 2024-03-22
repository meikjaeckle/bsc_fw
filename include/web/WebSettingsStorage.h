// Copyright (c) 2024 Tobias Himmler
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#ifndef WEBSETTINGSMGR_HPP
#define WEBSETTINGSMGR_HPP

#include <cstdint>
#include <type_traits>

#include <freertos/semphr.h>
#include <Preferences.h>
#include <sparsepp/spp.h>
#include <WString.h> // String

namespace fs
{
  class FS; // forward declaration of fs::FS
} // namespace fs

namespace web
{

class WebSettingsStorage
{
public:
  WebSettingsStorage();
  ~WebSettingsStorage();

public:
  bool init(fs::FS &fs, const String &configFilePath);
  bool addDefaultValuesFromNewKeys(const char *parameter, uint32_t jsonStartPos, const String &confName);
  bool isKeyExist(uint16_t key, uint8_t dataType) const;
  bool sync(); // Write the config from RAM to flash
  void   writeValue(uint16_t name, String value, uint8_t datatype, bool toPrefs);
  String readValue(uint16_t name, uint8_t dataType, boolean fromPrefs);

  String   getString(uint16_t name, uint8_t groupNr) const;
  int32_t  getInt(uint16_t name, uint8_t dataType) const;
  int32_t  getInt(uint16_t name, uint8_t groupNr, uint8_t dataType) const;
  float    getFloat(uint16_t name) const;
  float    getFloat(uint16_t name, uint8_t groupNr) const;
  bool     getBool(uint16_t name) const;
  bool     getBool(uint16_t name, uint8_t groupNr) const;

  int32_t getIntFlash(uint16_t name, uint8_t groupNr, uint8_t dataType) const;
  int32_t getIntFlash(uint16_t name, uint8_t groupNr) const;
  float   getFloatFlash(uint16_t name, uint8_t groupNr) const;
  float   getFloatFlash(uint16_t name) const;
  bool    getBoolFlash(uint16_t name, uint8_t groupNr) const;
  bool    getBoolFlash(uint16_t name) const;
  String  getStringFlash(uint16_t name, uint8_t groupNr) const;
  String  getStringFlash(const String &name) const;
  String  getStringFlash(uint16_t name) const;

  void setParameter(uint16_t name, uint8_t group, const String &value, uint8_t dataType);

private:
  template <typename DATA_TYPE>
  using SettingsMapT = spp::sparse_hash_map<uint16_t, DATA_TYPE>;

  bool addDefaultValuesFromNewKeys(const char *parameter, uint32_t jsonStartPos, const String &confName, uint8_t aktOptionGroupNr); // Called recursive
  bool readConfig();
  bool writeConfig();
  void setValue(uint16_t name, const String &value, uint8_t dataType);
  void setInt(uint16_t name, int32_t value);

  template<typename T>
  constexpr auto& getMap();

  template<typename T>
  constexpr const auto& getMap() const
  {
    return const_cast<WebSettingsStorage&>(*this).getMap<T>();
  }

  template<typename T>
  T readValueFromMap(uint16_t name) const;

  template<typename T>
  T readValueFromPrefs(uint16_t name) const;

  template<typename T>
  T readValueFromPrefs(const String &nameStr) const;

  template<typename T>
  String readValueAsString(uint16_t name, bool fromPrefs) const;

  template<typename T>
  void writeValueToPrefs(uint16_t name, const String &value);

private:
  static const String mBackupFilePath;

  SemaphoreHandle_t   mMutex {nullptr};

  SettingsMapT<int8_t>  mSettingValues_i8;
  SettingsMapT<int16_t> mSettingValues_i16;
  SettingsMapT<int32_t> mSettingValues_i32;
  SettingsMapT<float>   mSettingValues_fl;
  SettingsMapT<bool>    mSettingValues_bo;
  SettingsMapT<String>  mSettingValues_str;
  fs::FS                *mFileSystem {nullptr};
  Preferences           mPrefs;
  String                mConfigFilePath;
};


template<typename T>
constexpr auto& WebSettingsStorage::getMap()
{
  if constexpr (std::is_same<T, int8_t>::value || std::is_same<T, uint8_t>::value)
    return mSettingValues_i8;
  else if constexpr (std::is_same<T, int16_t>::value || std::is_same<T, uint16_t>::value)
    return mSettingValues_i16;
  else if constexpr (std::is_same<T, int32_t>::value || std::is_same<T, uint32_t>::value)
    return mSettingValues_i32;
  else if constexpr (std::is_same<T, float>::value)
    return mSettingValues_fl;
  else if constexpr (std::is_same<T, String>::value)
    return mSettingValues_str;
  else if constexpr (std::is_same<T, bool>::value)
    return mSettingValues_bo;
  else
    static_assert(!std::is_same_v<T, T>, "Type is not supported"); // Helper to catch if the given type is not supported
}

template<typename T>
T WebSettingsStorage::readValueFromMap(uint16_t name) const
{
  const auto& map = getMap<T>();
  return (map.contains(name) ? map.at(name) : T{});
}

template<typename T>
T WebSettingsStorage::readValueFromPrefs(uint16_t name) const
{
  const String nameStr(name);
  return readValueFromPrefs<T>(nameStr);
}

template<typename T>
T WebSettingsStorage::readValueFromPrefs(const String &nameStr) const
{
  Preferences &prefs = const_cast<Preferences&>(mPrefs);

  if constexpr (std::is_same<T, int8_t>::value || std::is_same<T, uint8_t>::value)
    return static_cast<T>(prefs.getChar(nameStr.c_str()));
  else if constexpr (std::is_same<T, int16_t>::value || std::is_same<T, uint16_t>::value)
    return static_cast<T>(prefs.getInt(nameStr.c_str()));
  else if constexpr (std::is_same<T, int32_t>::value || std::is_same<T, uint32_t>::value)
    return static_cast<T>(prefs.getLong(nameStr.c_str()));
  else if constexpr (std::is_same<T, float>::value)
    return prefs.getFloat(nameStr.c_str());
  else if constexpr (std::is_same<T, String>::value)
    return prefs.getString(nameStr.c_str());
  else if constexpr (std::is_same<T, bool>::value)
    return prefs.getBool(nameStr.c_str());
  else
    static_assert(!std::is_same_v<T, T>, "Type is not supported"); // Helper to catch if the given type is not supported
}

template<typename T>
String WebSettingsStorage::readValueAsString(uint16_t name, bool fromPrefs) const
{
  const T value = (fromPrefs) ? readValueFromPrefs<T>(name) : readValueFromMap<T>(name);
  if constexpr (std::is_same<T, String>::value)
    return value;
  else
    return String(value); // convert value to string
}

template<typename T>
void WebSettingsStorage::writeValueToPrefs(uint16_t name, const String &valueToWrite)
{
  const String nameStr(name);

  // NOTE: Casting return to void to signal the compiler it is intended to ignore the return value.

  if constexpr (std::is_same<T, int8_t>::value || std::is_same<T, uint8_t>::value)
    (void) mPrefs.putChar(nameStr.c_str(), static_cast<T>(valueToWrite.toInt()));
  else if constexpr (std::is_same<T, int16_t>::value || std::is_same<T, uint16_t>::value)
    (void) mPrefs.putInt(nameStr.c_str(), static_cast<T>(valueToWrite.toInt()));
  else if constexpr (std::is_same<T, int32_t>::value || std::is_same<T, uint32_t>::value)
    (void) mPrefs.putLong(nameStr.c_str(), static_cast<T>(valueToWrite.toInt()));
  else if constexpr (std::is_same<T, float>::value)
    (void) mPrefs.putFloat(nameStr.c_str(), valueToWrite.toFloat());
  else if constexpr (std::is_same<T, String>::value)
    (void) mPrefs.putString(nameStr.c_str(), valueToWrite.c_str());
  else if constexpr (std::is_same<T, bool>::value)
    (void) mPrefs.putBool(nameStr.c_str(), static_cast<T>(valueToWrite.toInt()));
  else
    static_assert(!std::is_same_v<T, T>, "Type is not supported"); // Helper to catch if the given type is not supported
}

// TODO MEJ Upate doc
/**
 * A simple singleton wrapper for the ESP32TWAI class to support legacy code,
 * where global access to the ESP32TWAI class is required.
 * @note If possible, don´t use the singleton, use some kind of dependency injection instead.
 * The #define "CAN" can be used as an "alias" to access the singleton object of the ESP32TWAI.
 */
class WebSettingsMgrSingleton :
  public WebSettingsStorage
{
  private:
    // don't allow public construct/destruction
    WebSettingsMgrSingleton()  = default;
    ~WebSettingsMgrSingleton() = default;

  // Make this class non-copyable, it`s a singleton
  public:
    WebSettingsMgrSingleton(const WebSettingsMgrSingleton& other) = delete;
    WebSettingsMgrSingleton& operator=(const WebSettingsMgrSingleton& other) = delete;

    /** Returns the singleton instance (static) of the ESP32TWAISingleton.
     *  The instance is created on the first call to getInstance.
    */
    static WebSettingsMgrSingleton& getInstance()
    {
      static WebSettingsMgrSingleton instance;
      return instance;
    }
};

} // namespace web

#define WEBSETTINGS (web::WebSettingsMgrSingleton::getInstance())

#endif // WEBSETTINGSMGR_HPP
