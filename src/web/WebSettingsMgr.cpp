// Copyright (c) 2024 Tobias Himmler
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#include <algorithm>
#include <array>
#include <cstdint>
#include <type_traits>
#include <vector>

#include <FS.h>
#include <sparsepp/spp.h>
#include <WString.h>

#include <crc.h>
#include <defines.h>
#include <Preferences.h>
#include <json/Utils.h>
#include <web/Utils.h>
#include <web/WebDefinitions.h>
#include <web/WebSettingsMgr.h>

namespace web
{

namespace // anonymous namespace. only visible local
{

// local helper methods

void getIdFromParamId(uint16_t paramId, uint16_t &id, uint8_t &groupIdx)
{
  id = ((paramId>>6) & 0x3FF);
  groupIdx = (paramId & 0x3F);
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

/* static */ const String WebSettingsMgr::mBackupFilePath {"/WebSettings.sich"};

WebSettingsMgr::WebSettingsMgr()
  : mMutex(xSemaphoreCreateMutex())
{
  assert(mMutex && "WebSettingsMgr: Failed to create mutex");
}

WebSettingsMgr::~WebSettingsMgr()
{
  vSemaphoreDelete(mMutex);
}

bool WebSettingsMgr::init(fs::FS &fs, const String &configFilePath)
{
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
  BSC_LOGI(TAG, "WebSettingsMgr::init() end");
  #endif

  return initSuccessful;
}

bool WebSettingsMgr::addDefaultValuesFromNewKeys(const char *parameter, uint32_t jsonStartPos, const String &confName)
{
  assert(parameter);

  // xSemaphoreTake(mMutex, portMAX_DELAY);

  constexpr uint8_t optionGroupNr {0};
  const bool hasNewKeys = addDefaultValuesFromNewKeys(parameter, jsonStartPos, confName, optionGroupNr); // Called recursive

  // xSemaphoreGive(mMutex);

  return hasNewKeys;
}

bool WebSettingsMgr::isKeyExist(uint16_t key, uint8_t u8_dataType) const
{
  bool ret = false;
  xSemaphoreTake(mMutex, portMAX_DELAY);
  switch(u8_dataType)
  {
    case PARAM_DT_U8:
      ret = mSettingValues_i8.contains(key);
      break;
    case PARAM_DT_I8:
      ret = mSettingValues_i8.contains(key);
      break;
    case PARAM_DT_U16:
      ret = mSettingValues_i16.contains(key);
      break;
    case PARAM_DT_I16:
      ret = mSettingValues_i16.contains(key);
      break;
    case PARAM_DT_U32:
      ret = mSettingValues_i32.contains(key);
      break;
    case PARAM_DT_I32:
      ret = mSettingValues_i32.contains(key);
      break;
    case PARAM_DT_FL:
      ret = mSettingValues_fl.contains(key);
      break;
    case PARAM_DT_ST:
      ret = mSettingValues_str.contains(key);
      break;
    case PARAM_DT_BO:
      ret = mSettingValues_bo.contains(key);
      break;
  }
  xSemaphoreGive(mMutex);
  return ret;
}

bool WebSettingsMgr::sync()
{
  xSemaphoreTake(mMutex, portMAX_DELAY);

  const bool ret = writeConfig();

  xSemaphoreGive(mMutex);

  return ret;
}

// Pass value by value, as we may have to modify it
void WebSettingsMgr::writeValue(const uint16_t name, String value, const uint8_t datatype, const bool toPrefs)
{
  xSemaphoreTake(mMutex, portMAX_DELAY);

  if (toPrefs) //Store  in Flash
  {
    #ifdef WEBSET_DEBUG
    BSC_LOGD(TAG,"Store in Flash; u16_argName=%i, dt=%i", name, datatype);
    #endif

    //Spezielle Parameter vor dem Speichern ändern
    uint16_t u16_lParamId  {0};
    uint8_t u8_lParamGroup {0};
    getIdFromParamId(name, u16_lParamId, u8_lParamGroup);

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

  xSemaphoreGive(mMutex);
}

String WebSettingsMgr::readValue(uint16_t name, uint8_t dataType, boolean fromFlash)
{
  String ret;

  xSemaphoreTake(mMutex, portMAX_DELAY);
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
  xSemaphoreGive(mMutex);

  #ifdef WEBSET_DEBUG
  BSC_LOGI(TAG,"readValue(): name=%i, fromFlash=%d, dataType=%i, string: %s", name, fromFlash, dataType, ret.c_str());
  #endif

  return ret;
}

String WebSettingsMgr::getString(uint16_t name, uint8_t groupNr) const
{
  xSemaphoreTake(mMutex, portMAX_DELAY);
  const String ret = readValueFromMap<String>(getParmId(name,groupNr));
  xSemaphoreGive(mMutex);
  return ret;
}

int32_t WebSettingsMgr::getInt(uint16_t name, uint8_t u8_dataType) const
{
  /*#ifdef WEBSET_DEBUG
  BSC_LOGI(TAG,"getInt(); name=%i",name);
  #endif*/

  uint32_t ret = 0;
  xSemaphoreTake(mMutex, portMAX_DELAY);
  switch(u8_dataType)
  {
    case PARAM_DT_U8:
      ret = readValueFromMap<uint8_t>(name);
      break;
    case PARAM_DT_I8:
      ret = readValueFromMap<int8_t>(name);
      break;
    case PARAM_DT_U16:
      ret = readValueFromMap<uint16_t>(name);
      break;
    case PARAM_DT_I16:
      ret = readValueFromMap<int16_t>(name);
      break;
    case PARAM_DT_U32:
      ret = readValueFromMap<uint32_t>(name);
      break;
    case PARAM_DT_I32:
      ret = readValueFromMap<int32_t>(name);
      break;
  }
  xSemaphoreGive(mMutex);
  return ret;
}

int32_t WebSettingsMgr::getInt(uint16_t name, uint8_t groupNr, uint8_t u8_dataType) const
{
  return getInt(getParmId(name,groupNr),u8_dataType);
}

float WebSettingsMgr::getFloat(uint16_t name) const
{
  // TODO MEJ Lock mutex
  return readValueFromMap<float>(name);
}
float WebSettingsMgr::getFloat(uint16_t name, uint8_t groupNr) const
{
  return getFloat(getParmId(name, groupNr));
}

bool WebSettingsMgr::getBool(uint16_t name) const
{
  // TODO MEJ Lock mutex
  return readValueFromMap<bool>(name);
}

bool WebSettingsMgr::getBool(uint16_t name, uint8_t groupNr) const
{
  return getBool(getParmId(name, groupNr));
}

//Load Data from Flash
int32_t WebSettingsMgr::getIntFlash(uint16_t name, uint8_t groupNr, uint8_t u8_dataType) const
{
  name = getParmId(name, groupNr);
  return getIntFlash(getParmId(name, groupNr), u8_dataType);
}

int32_t WebSettingsMgr::getIntFlash(uint16_t name, uint8_t u8_dataType) const
{
  // TODO MEJ Lock mutex

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

float WebSettingsMgr::getFloatFlash(uint16_t name, uint8_t groupNr) const
{
  return getFloatFlash(getParmId(name, groupNr));
}

float WebSettingsMgr::getFloatFlash(uint16_t name) const
{
  // TODO MEJ Lock mutex

  return readValueFromPrefs<float>(name);
}

bool WebSettingsMgr::getBoolFlash(uint16_t name, uint8_t groupNr) const
{
  return getBoolFlash(getParmId(name, groupNr));
}

bool WebSettingsMgr::getBoolFlash(uint16_t name) const
{
  // TODO MEJ Lock mutex

  return readValueFromPrefs<bool>(name);
}

String WebSettingsMgr::getStringFlash(uint16_t name, uint8_t groupNr) const
{
  return getStringFlash(getParmId(name, groupNr));
}

String WebSettingsMgr::getStringFlash(const String &name) const
{
  // TODO MEJ Lock mutex

  if(name.isEmpty())
    return emptyString;
  else
    return readValueFromPrefs<String>(name);
}

String WebSettingsMgr::getStringFlash(uint16_t name) const
{
  // TODO MEJ Lock mutex

  return readValueFromPrefs<String>(name);
}

void WebSettingsMgr::setParameter(uint16_t name, uint8_t group, const String &value, uint8_t u8_dataType)
{
  // TODO MEJ Lock mutex

  setValue(getParmId(name, group), value, u8_dataType);
}

// ----------------- private methods ----------------------------------------

bool WebSettingsMgr::addDefaultValuesFromNewKeys(const char *parameter, uint32_t jsonStartPos, const String &confName, uint8_t aktOptionGroupNr)
{
  bool hasNewKeys {false};

  #ifdef WEBSET_DEBUG
  BSC_LOGI(TAG,"addDefaultValuesFromNewKeys: confName=%s, arraySize=%i", confName.c_str(), json::getArraySize(parameter, jsonStartPos));
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
        hasNewKeys |= addDefaultValuesFromNewKeys(parameter, jsonArrayGroupStart, confName, g); // Recursive call of this method!
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

        if((jsonNameBase!= 0) && (isKeyExist(jsonName, u8_dataType)==false))
        {
          hasNewKeys = true;
          uint16_t id=0;
          uint8_t group=0;
          web::getIdFromParamId(jsonName,id,group);
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
          uint16_t id=0;
          uint8_t group=0;
          getIdFromParamId(jsonName,id,group);
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
  BSC_LOGI(TAG,"addDefaultValuesFromNewKeys: finish");
  #endif

  return hasNewKeys;
}

//Lese Parameter aus Datei
bool WebSettingsMgr::readConfig()
{
  assert(mFileSystem);

  // TODO MEJ Lock mutex ?

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
bool WebSettingsMgr::writeConfig()
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
    auto PrintToFile = [&f](const auto& element)
    {
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

void WebSettingsMgr::setValue(uint16_t name, const String &value, uint8_t dataType)
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
