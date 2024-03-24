// Copyright (c) 2024 Tobias Himmler
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#include <algorithm>
#include <array>
#include <cstdint>
#include <type_traits>
#include <utility> // std::pair
#include <vector>

#include <FS.h>
#include <kernelapi/rtos/LockGuard.hpp>
#include <sparsepp/spp.h>
#include <WString.h>


#include <crc.h>
#include <defines.h>
#include <Preferences.h>
#include <json/Utils.h>
#include <web/Utils.h>
#include <web/WebDefinitions.h>
#include <web/WebSettingsStorage.h>

namespace web
{

namespace // anonymous namespace. only visible within file scope
{

// local helper methods

// returns "uint16_t id" and "uint8_t groupIdx" as std::pair<uint16_t, uint8_t>
// example usage:
//    const auto [u16_lParamId, u8_lParamGroup] = getIdFromParamId(name);
constexpr auto getIdFromParamId(uint16_t paramId)
{
  const uint16_t id = ((paramId>>6) & 0x3FF);
  const uint8_t groupIdx = (paramId & 0x3F);
  return std::pair<uint16_t, uint8_t> {id, groupIdx};
}

uint32_t copyFile(fs::FS &fs, const String &fileSrc, const String &fileDst)
{
  std::array<char, 64> buf {};
  uint32_t crc {};

  if (fs.exists(fileDst) == true)
    fs.remove(fileDst);

  File fSrc = fs.open(fileSrc, "r");
  File fDst = fs.open(fileDst, "w");

  while (fSrc.available() > 0)
  {
    const uint8_t readBytes = fSrc.readBytes(buf.data(), buf.size());
    crc = calcCrc32(crc, reinterpret_cast<uint8_t*>(buf.data()), readBytes);
    fDst.write(reinterpret_cast<uint8_t*>(buf.data()), readBytes);
  }

  fDst.close();
  fSrc.flush();
  fSrc.close();

  return crc;
}

uint32_t calcCrc(fs::FS &fs, const String &fileSrc)
{
  std::array<char, 64> buf {};
  uint32_t crc {0};
  File fSrc = fs.open(fileSrc, "r");
  while (fSrc.available() > 0)
  {
    const uint8_t readBytes = fSrc.readBytes(buf.data(), buf.size());
    crc = calcCrc32(crc, reinterpret_cast<uint8_t*>(buf.data()), readBytes);
  }
  return crc;
}

} // anonymous namespace

static const char * TAG = "WEB_SETTINGS_MGR";

/* static */ const String WebSettingsStorage::mBackupFilePath {"/WebSettings.sich"};

bool WebSettingsStorage::init(fs::FS &fs, const String &configFilePath)
{
  const rtos::LockGuard lock(mMutex);

  mFileSystem = &fs;
  mConfigFilePath = configFilePath;

  // TODO MEJ Wouldn`t it be better to handle this already outside of this class, while initializing the filesystem?
  // if (!mFileSystem.begin())
  // {
  //   BSC_LOGE(TAG, "Mount Failed");
  //   mFileSystem.format();
  //   mFileSystem.begin();
  // }

  bool initSuccessful {true}; // Init with true and lets use &= to update the flag

  if (!mPrefs.begin("prefs"))
  {
    initSuccessful &= false;
    #ifdef WEBSET_DEBUG
    BSC_LOGE(TAG,"Fehler beim Oeffnen des NVS-Namespace");
    #endif
  }
  else
  {
    BSC_LOGI(TAG,"Free flash entries: %d", mPrefs.freeEntries());
  }

  initSuccessful &= readConfig();

  #ifdef WEBSET_DEBUG
  BSC_LOGI(TAG, "WebSettingsStorage::init() end");
  #endif

  return initSuccessful;
}

bool WebSettingsStorage::addDefaultValuesFromNewKeys(const char *parameter, uint32_t jsonStartPos, const String &confName)
{
  assert(parameter);
  constexpr uint8_t optionGroupNr {0};

  const rtos::LockGuard lock(mMutex);
  return doAddDefaultValuesFromNewKeys(parameter, jsonStartPos, confName, optionGroupNr); // Called recursive
}

bool WebSettingsStorage::isKeyExist(uint16_t key, uint8_t dataType) const
{
  const rtos::LockGuard lock(mMutex);
  return doCheckKeyExist(key, dataType);
}

bool WebSettingsStorage::sync()
{
  const rtos::LockGuard lock(mMutex);
  return writeConfig();
}

// Pass value by value, as we may have to modify it
void WebSettingsStorage::writeValue(const uint16_t name, String value, const uint8_t datatype, const bool toPrefs)
{
  const rtos::LockGuard lock(mMutex);

  if (toPrefs) //Store  in Flash
  {
    #ifdef WEBSET_DEBUG
    BSC_LOGD(TAG,"Store in Flash; u16_argName=%i, dt=%i", name, datatype);
    #endif

    //Spezielle Parameter vor dem Speichern ändern
    const auto [u16_lParamId, u8_lParamGroup] = getIdFromParamId(name);

    if(u16_lParamId==ID_PARAM_SS_BTDEVMAC)
      value.toLowerCase();

    switch(datatype)
    {
      case PARAM_DT_U8:
        writeValueToPrefs<uint8_t>(name, value);
        break;
      case PARAM_DT_I8:
        writeValueToPrefs<int8_t>(name, value);
        break;
      case PARAM_DT_U16:
        writeValueToPrefs<uint16_t>(name, value);
        break;
      case PARAM_DT_I16:
        writeValueToPrefs<int16_t>(name, value);
        break;
      case PARAM_DT_U32:
        writeValueToPrefs<uint32_t>(name, value);
        break;
      case PARAM_DT_I32:
        writeValueToPrefs<int32_t>(name, value);
        break;
      case PARAM_DT_FL:
        writeValueToPrefs<float>(name, value);
        break;
      case PARAM_DT_ST:
        writeValueToPrefs<String>(name, value);
        break;
      case PARAM_DT_BO:
        writeValueToPrefs<bool>(name, value);
        break;
    }
  }
  else //Store in RAM
  {
    #ifdef WEBSET_DEBUG
    BSC_LOGD(TAG,"Store in RAM; name=%i, dt=%i", name, datatype);
    #endif

    setValue(name, value, datatype);
  }
}

String WebSettingsStorage::readValue(uint16_t name, uint8_t dataType, boolean fromFlash)
{
  const rtos::LockGuard lock(mMutex);

  String ret;

  switch(dataType)
  {
    case PARAM_DT_U8:
      ret = readValueAsString<uint8_t>(name, fromFlash);
      break;
    case PARAM_DT_I8:
      ret = readValueAsString<int8_t>(name, fromFlash);
      break;
    case PARAM_DT_U16:
      ret = readValueAsString<uint16_t>(name, fromFlash);
      break;
    case PARAM_DT_I16:
      ret = readValueAsString<int16_t>(name, fromFlash);
      break;
    case PARAM_DT_U32:
      ret = readValueAsString<uint32_t>(name, fromFlash);
      break;
    case PARAM_DT_I32:
      ret = readValueAsString<int32_t>(name, fromFlash);
      break;
    case PARAM_DT_FL:
      ret = readValueAsString<float>(name, fromFlash);
      break;
    case PARAM_DT_ST:
      ret = readValueAsString<String>(name, fromFlash);
      break;
    case PARAM_DT_BO:
      ret = readValueAsString<bool>(name, fromFlash);
      break;
  }

  #ifdef WEBSET_DEBUG
  BSC_LOGI(TAG,"readValue(): name=%i, fromFlash=%d, dataType=%i, string: %s", name, fromFlash, dataType, ret.c_str());
  #endif

  return ret;
}

String WebSettingsStorage::getString(uint16_t name, uint8_t groupNr) const
{
  const rtos::LockGuard lock(mMutex);
  return readValueFromMap<String>(getParmId(name,groupNr));
}

int32_t WebSettingsStorage::getInt(uint16_t name, uint8_t u8_dataType) const
{
  /*#ifdef WEBSET_DEBUG
  BSC_LOGI(TAG,"getInt(); name=%i",name);
  #endif*/

  const rtos::LockGuard lock(mMutex);

  switch(u8_dataType)
  {
    case PARAM_DT_U8:
      return readValueFromMap<uint8_t>(name);
    case PARAM_DT_I8:
      return readValueFromMap<int8_t>(name);
    case PARAM_DT_U16:
      return readValueFromMap<uint16_t>(name);
    case PARAM_DT_I16:
      return readValueFromMap<int16_t>(name);
    case PARAM_DT_U32:
      return readValueFromMap<uint32_t>(name);
    case PARAM_DT_I32:
      return readValueFromMap<int32_t>(name);
    default:
      return {};
  }
}

int32_t WebSettingsStorage::getInt(uint16_t name, uint8_t groupNr, uint8_t u8_dataType) const
{
  return getInt(getParmId(name,groupNr),u8_dataType);
}

float WebSettingsStorage::getFloat(uint16_t name) const
{
  const rtos::LockGuard lock(mMutex);
  return readValueFromMap<float>(name);
}
float WebSettingsStorage::getFloat(uint16_t name, uint8_t groupNr) const
{
  return getFloat(getParmId(name, groupNr));
}

bool WebSettingsStorage::getBool(uint16_t name) const
{
  const rtos::LockGuard lock(mMutex);
  return readValueFromMap<bool>(name);
}

bool WebSettingsStorage::getBool(uint16_t name, uint8_t groupNr) const
{
  return getBool(getParmId(name, groupNr));
}

//Load Data from Flash
int32_t WebSettingsStorage::getIntFlash(uint16_t name, uint8_t groupNr, uint8_t u8_dataType) const
{
  name = getParmId(name, groupNr);
  return getIntFlash(getParmId(name, groupNr), u8_dataType);
}

int32_t WebSettingsStorage::getIntFlash(uint16_t name, uint8_t u8_dataType) const
{
  const rtos::LockGuard lock(mMutex);

  switch(u8_dataType)
  {
    case PARAM_DT_U8:
      return readValueFromPrefs<uint8_t>(name);
    case PARAM_DT_I8:
      return readValueFromPrefs<int8_t>(name);
    case PARAM_DT_U16:
      return readValueFromPrefs<uint16_t>(name);
    case PARAM_DT_I16:
      return readValueFromPrefs<int16_t>(name);
    case PARAM_DT_U32:
      return readValueFromPrefs<uint32_t>(name);
    case PARAM_DT_I32:
      return readValueFromPrefs<int32_t>(name);
    default:
      return static_cast<int32_t>(0); // TODO: Shall we handle the unknown data type somehow???
  }
}

float WebSettingsStorage::getFloatFlash(uint16_t name, uint8_t groupNr) const
{
  return getFloatFlash(getParmId(name, groupNr));
}

float WebSettingsStorage::getFloatFlash(uint16_t name) const
{
  const rtos::LockGuard lock(mMutex);
  return readValueFromPrefs<float>(name);
}

bool WebSettingsStorage::getBoolFlash(uint16_t name, uint8_t groupNr) const
{
  return getBoolFlash(getParmId(name, groupNr));
}

bool WebSettingsStorage::getBoolFlash(uint16_t name) const
{
  const rtos::LockGuard lock(mMutex);
  return readValueFromPrefs<bool>(name);
}

String WebSettingsStorage::getStringFlash(uint16_t name, uint8_t groupNr) const
{
  return getStringFlash(getParmId(name, groupNr));
}

String WebSettingsStorage::getStringFlash(const String &name) const
{
  const rtos::LockGuard lock(mMutex);

  if(name.isEmpty())
    return emptyString;
  else
    return readValueFromPrefs<String>(name);
}

String WebSettingsStorage::getStringFlash(uint16_t name) const
{
  const rtos::LockGuard lock(mMutex);

  return readValueFromPrefs<String>(name);
}

void WebSettingsStorage::setParameter(uint16_t name, uint8_t group, const String &value, uint8_t u8_dataType)
{
  const rtos::LockGuard lock(mMutex);

  setValue(getParmId(name, group), value, u8_dataType);
}

// ----------------- private methods ----------------------------------------

bool WebSettingsStorage::doCheckKeyExist(uint16_t key, uint8_t u8_dataType) const
{
  switch(u8_dataType)
  {
    case PARAM_DT_U8:
      return getMap<uint8_t>().contains(key);
    case PARAM_DT_I8:
      return getMap<int8_t>().contains(key);
    case PARAM_DT_U16:
      return getMap<uint16_t>().contains(key);
    case PARAM_DT_I16:
      return getMap<int16_t>().contains(key);
    case PARAM_DT_U32:
      return getMap<uint32_t>().contains(key);
    case PARAM_DT_I32:
      return getMap<int32_t>().contains(key);
    case PARAM_DT_FL:
      return getMap<float>().contains(key);
    case PARAM_DT_ST:
      return getMap<String>().contains(key);
    case PARAM_DT_BO:
      return getMap<bool>().contains(key);
    default:
      return false;
  }
}

bool WebSettingsStorage::doAddDefaultValuesFromNewKeys(const char *parameter, uint32_t jsonStartPos, const String &confName, uint8_t aktOptionGroupNr)
{
  bool hasNewKeys {false};

  #ifdef WEBSET_DEBUG
  BSC_LOGI(TAG,"doAddDefaultValuesFromNewKeys: confName=%s, arraySize=%i", confName.c_str(), json::getArraySize(parameter, jsonStartPos));
  #endif
  for (uint8_t a {0}; a < json::getArraySize(parameter, jsonStartPos); a++)
  {
    const uint8_t type = web::getJsonType(parameter, a, jsonStartPos);
    //const uint8_t optionCnt = web::getJsonOptionsCnt(parameter, a, jsonStartPos);

    if (type==HTML_OPTIONGROUP || type==HTML_OPTIONGROUP_COLLAPSIBLE)
    {
      const uint8_t optionGroupSize = web::getJsonGroupsize(parameter, a, jsonStartPos);
      for(uint8_t g {0}; g < optionGroupSize; ++g)
      {
        String retStr;
        uint32_t jsonArrayGroupStart {0};
        json::getValue(parameter, a, "group", jsonStartPos, retStr, jsonArrayGroupStart);  // TODO MEJ Handle return value
        hasNewKeys |= doAddDefaultValuesFromNewKeys(parameter, jsonArrayGroupStart, confName, g); // Recursive call of this method!
      }
    }
    else
    {
      uint32_t u32_tmp {};
      String retStr_default;
      json::getValue(parameter, a, "dt", jsonStartPos, retStr_default, u32_tmp); // TODO MEJ Handle return value
      const uint8_t u8_dataType   = static_cast<uint8_t>(retStr_default.toInt());
      const uint16_t jsonNameBase = web::getJsonName(parameter, a, jsonStartPos);
      const uint16_t jsonName     = web::getParmId(jsonNameBase, aktOptionGroupNr);
      //Store in RAM
      if(web::getJson_Key(parameter, "flash", a, jsonStartPos, "").equals("1")==false)
      {
        retStr_default.clear();
        json::getValue(parameter, a, "default", jsonStartPos, retStr_default, u32_tmp);

        if((jsonNameBase!= 0) && (doCheckKeyExist(jsonName, u8_dataType)==false))
        {
          hasNewKeys = true;
          //const auto [id, group] = getIdFromParamId(jsonName);
          //BSC_LOGI(TAG,"newDefKeyInRam: key=%i, val=%s, dt=%i, id=%i, group=%i",jsonName,retStr_default.c_str(),u8_dataType,id,group);
          setValue(jsonName, retStr_default, u8_dataType);
        }
      }
      else
      {
        if((jsonNameBase != 0) && (mPrefs.isKey(String(jsonName).c_str())==false))
        {
          hasNewKeys = true;
          retStr_default.clear();
          json::getValue(parameter, a, "default", jsonStartPos, retStr_default, u32_tmp);

          //const auto [id, group] = getIdFromParamId(jsonName);
          //BSC_LOGI(TAG,"newDefKeyInFlash: key=%i, val=%s, dt=%i, id=%i, group=%i",jsonName,retStr_default.c_str(),u8_dataType,id,group);

          switch(u8_dataType)
          {
            case PARAM_DT_U8:
              writeValueToPrefs<uint8_t>(jsonName, retStr_default);
              break;
            case PARAM_DT_I8:
              writeValueToPrefs<int8_t>(jsonName, retStr_default);
              break;
            case PARAM_DT_U16:
              writeValueToPrefs<uint16_t>(jsonName, retStr_default);
              break;
            case PARAM_DT_I16:
              writeValueToPrefs<int16_t>(jsonName, retStr_default);
              break;
            case PARAM_DT_U32:
              writeValueToPrefs<uint32_t>(jsonName, retStr_default);
              break;
            case PARAM_DT_I32:
              writeValueToPrefs<int32_t>(jsonName, retStr_default);
              break;
            case PARAM_DT_FL:
              writeValueToPrefs<float>(jsonName, retStr_default);
              break;
            case PARAM_DT_ST:
              writeValueToPrefs<String>(jsonName, retStr_default);
              break;
            case PARAM_DT_BO:
              writeValueToPrefs<bool>(jsonName, retStr_default);
              break;
          }
        }
      }
    }
  }
  #ifdef WEBSET_DEBUG
  BSC_LOGI(TAG,"doAddDefaultValuesFromNewKeys: finish");
  #endif

  return hasNewKeys;
}

//Lese Parameter aus Datei
bool WebSettingsStorage::readConfig()
{
  assert(mFileSystem);


  #ifdef WEBSET_DEBUG
  BSC_LOGI(TAG,"readConfig()");
  #endif

  String  confFilePath{mConfigFilePath};

  if (mFileSystem->exists(confFilePath.c_str()))
  {
    const uint32_t crc = calcCrc(*mFileSystem, confFilePath.c_str());

    if(mPrefs.isKey("confCrc") && mPrefs.getULong("confCrc")!=crc) // Wrong CRC
    {
      if(mFileSystem->exists(mBackupFilePath))
      {
        BSC_LOGI(TAG,"Fehler beim lesen der Parameter (falsche CRC)! Lade Backup.");
        confFilePath = mBackupFilePath;
      }
      else
      {
        BSC_LOGI(TAG,"Fehler beim lesen der Parameter (falsche CRC)!");
        writeConfig();
      }
    }
    else
    {
      if(!mPrefs.isKey("confCrc"))
        BSC_LOGI(TAG,"Noch keine CRC der Settings vorhanden");
      else
        BSC_LOGI(TAG,"Settings ok");

      //Wenn die CRC der Config ok ist, aber noch kein Backup-File exisitiert
      const uint32_t crcBackup = copyFile(*mFileSystem, mConfigFilePath, mBackupFilePath);
      if(crc!=crcBackup) //CRC des Backups falsch -> Bakup wieder löschen
      {
        BSC_LOGI(TAG,"Fehler beim erstellen des Backups (falsche CRC)");
        mFileSystem->remove(mBackupFilePath);
      }
    }
  }
  else
  {
    //wenn settingfile nicht vorhanden, dann schreibe default Werte
    if(mFileSystem->exists(mBackupFilePath))
    {
      BSC_LOGI(TAG,"Kein Parameterfile vorhanden. Lade Backup.");
      confFilePath = mBackupFilePath;
    }
    else
    {
      BSC_LOGI(TAG,"Kein Parameterfile vorhanden");
      writeConfig();
    }
  }

  File f = mFileSystem->open(confFilePath.c_str(), "r");
  if (f)
  {
    #ifdef WEBSET_DEBUG
    BSC_LOGI(TAG,"file open");
    #endif

    const uint32_t size = f.size();
    while (f.position() < size)
    {
      #ifdef WEBSET_DEBUG
      BSC_LOGI(TAG,"readConfig size=%i, pos=%i", size, (uint32_t)f.position());
      #endif
      const String str_line {f.readStringUntil(10)};
      const uint8_t ui8_pos {static_cast<uint8_t>(str_line.indexOf('='))};
      const uint32_t str_name {static_cast<uint32_t>(str_line.substring(0, ui8_pos).toInt())};
      const String str_dataType {str_line.substring(ui8_pos+1, ui8_pos+1+3).c_str()};
      String str_value {str_line.substring(ui8_pos+1+3).c_str()};
      str_value.replace("~","\n");

      if(str_dataType.equals("U8_"))      setValue(str_name, str_value, PARAM_DT_U8);
      else if(str_dataType.equals("I8_")) setValue(str_name, str_value, PARAM_DT_I8);
      else if(str_dataType.equals("U16")) setValue(str_name, str_value, PARAM_DT_U16);
      else if(str_dataType.equals("I16")) setValue(str_name, str_value, PARAM_DT_I16);
      else if(str_dataType.equals("U32")) setValue(str_name, str_value, PARAM_DT_U32);
      else if(str_dataType.equals("I32")) setValue(str_name, str_value, PARAM_DT_I32);
      else if(str_dataType.equals("FL_")) setValue(str_name, str_value, PARAM_DT_FL);
      else if(str_dataType.equals("BO_")) setValue(str_name, str_value, PARAM_DT_BO);
      else if(str_dataType.equals("STR")) setValue(str_name, str_value, PARAM_DT_ST);

      #ifdef WEBSET_DEBUG
      BSC_LOGI(TAG,"readConfig key:%lu, val:%s",str_name, str_value.c_str());
      #endif

      constexpr uint32_t fPosOld = 0xFFFFFFFF;
      if (fPosOld == f.position())
      {
        BSC_LOGE(TAG,"Read config break: pos=%i", static_cast<uint32_t>(f.position()));
        break;
      }
    }

    f.close();

    #ifdef WEBSET_DEBUG
    BSC_LOGI(TAG,"readConfig() end");
    #endif

    return true;
  }
  else
  {
    BSC_LOGI(TAG,"Cannot read configuration");
    return false;
  }
}

//Schreiben der Parameter in Datei
bool WebSettingsStorage::writeConfig()
{
  assert(mFileSystem);

  #ifdef WEBSET_DEBUG
  BSC_LOGI(TAG, "writeConfig()");
  #endif

  if (!mFileSystem->exists(mConfigFilePath))
  {
    BSC_LOGI(TAG,"writeConfig(): file not exist");
  }

  File f = mFileSystem->open(mConfigFilePath,"w");
  if (f)
  {
    // lambda to print the parameter to file, takes care about the type
    auto PrintToFile = [&f](const auto& element)
    {
        // lambda to get the type dependend format string
        auto FormatStr = []<typename T>(T x) constexpr
        {
            if constexpr (std::is_same_v<T, int8_t>)
               return static_cast<const char*>("%lu=I8_%i\n");
            else if constexpr (std::is_same_v<T, int16_t>)
              return static_cast<const char*>("%lu=I16%i\n");
            else if constexpr (std::is_same_v<T, int32_t>)
              return static_cast<const char*>("%lu=I32%i\n");
            else if constexpr (std::is_same_v<T, bool>)
              return static_cast<const char*>("%lu=BO_%d\n");
            else if constexpr (std::is_same_v<T, float>)
              return static_cast<const char*>("%lu=FL_%f\n");
            else if constexpr (std::is_same_v<T, String>)
              return static_cast<const char*>("%lu=STR%s\n");
            else
              static_assert(!std::is_same_v<T, T>, "Type is not supported"); // Helper to catch if the given type is not supported
        };

        if constexpr ((std::is_integral_v<decltype(element.second)>) or
                      (std::is_floating_point_v<decltype(element.second)>))

        {
            f.printf(FormatStr(element.second), element.first, element.second);
        }
        else if constexpr (std::is_same_v<decltype(element.second), String>)
        {
            String stringValue = element.second;
            std::replace(stringValue.begin(), stringValue.end(), '\n', '~');
            f.printf(FormatStr(element.second), element.first, stringValue.c_str());
        }
    };

    std::for_each(mSettingValues_str.begin(), mSettingValues_str.end(), PrintToFile);
    std::for_each(mSettingValues_i8.begin(),  mSettingValues_i8.end(),  PrintToFile);
    std::for_each(mSettingValues_i16.begin(), mSettingValues_i16.end(), PrintToFile);
    std::for_each(mSettingValues_i32.begin(), mSettingValues_i32.end(), PrintToFile);
    std::for_each(mSettingValues_bo.begin(),  mSettingValues_bo.end(),  PrintToFile);
    std::for_each(mSettingValues_fl.begin(),  mSettingValues_fl.end(),  PrintToFile);

    f.flush();
    f.close();

    //CRC speichern und Backup der Config erstellen
    const uint32_t crc = calcCrc(*mFileSystem, mConfigFilePath);
    mPrefs.putULong("confCrc",crc);

    const uint32_t crcBackup = copyFile(*mFileSystem, mConfigFilePath, mBackupFilePath);
    if(crc != crcBackup)
    {
      BSC_LOGI(TAG,"Fehler beim erstellen des Backup (falsche CRC)");
      mFileSystem->remove(mBackupFilePath);
    }

    return true;
  }
  else
  {
    BSC_LOGI(TAG,"Cannot write configuration");
    return false;
  }
  #ifdef WEBSET_DEBUG
  BSC_LOGI(TAG, "writeConfig() finish");
  #endif
}

void WebSettingsStorage::setValue(uint16_t name, const String &value, uint8_t dataType)
{
  #ifdef WEBSET_DEBUG
  BSC_LOGI(TAG,"setValue(): name=%i, value=%s, dataType=%i",name,value.c_str(),dataType);
  #endif

  switch(dataType)
  {
    case PARAM_DT_U8:
      mSettingValues_i8[name] = static_cast<uint8_t>(value.toInt());
      break;
    case PARAM_DT_I8:
      mSettingValues_i8[name] = static_cast<int8_t>(value.toInt());
      break;
    case PARAM_DT_U16:
      mSettingValues_i16[name] = static_cast<uint16_t>(value.toInt());
      break;
    case PARAM_DT_I16:
      mSettingValues_i16[name] = static_cast<int16_t>(value.toInt());
      break;
    case PARAM_DT_U32:
      mSettingValues_i32[name] = static_cast<uint32_t>(value.toInt());
      break;
    case PARAM_DT_I32:
      mSettingValues_i32[name] = static_cast<int32_t>(value.toInt());
      break;
    case PARAM_DT_FL:
      mSettingValues_fl[name] = value.toFloat();
      break;
    case PARAM_DT_ST:
      mSettingValues_str[name] = value;
      break;
    case PARAM_DT_BO:
      mSettingValues_bo[name] = static_cast<bool>(value.toInt());
      break;
  }
}

} // namespace web
