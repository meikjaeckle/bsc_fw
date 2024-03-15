// Copyright (c) 2022 tobias
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT


#ifndef WebSettings_h
#define WebSettings_h

#include <cstdint>
#include <WebServer.h>
#include <WString.h>

#include <json/Utils.h>
#include <web/WebDefinitions.h>

// TODO MEJ not used ???
// #define BUILD_PARAM_ID(id, settingNr, groupIdx, listIdx) (id << 20)\
// |(settingNr << 14) \
// |(groupIdx << 7) \
// |(listIdx & 0x7F)

namespace web
{
  // forward declaration
  class WebSettingsStorage;
}

class WebSettings
{
public:
  WebSettings();
  void init(web::WebSettingsStorage &mgr, const char *parameter, const String &confName);

  // TODO MEJ Following methods are used by libwebapp2 library, that's why we have to provide a wrapper until libwebapp2 is updated
  boolean writeConfig();
  static int32_t getInt(uint16_t name, uint8_t groupNr, uint8_t dataType);
  static int32_t getIntFlash(uint16_t name, uint8_t groupNr, uint8_t dataType);

  //setButtons
  void setButtons(uint8_t buttons, const String &btnLabel);

  //register onSave callback
  void registerOnSave(void (*callback)());
  void registerOnButton1(void (*callback)());
  void registerOnButton2(void (*callback)());
  void registerOnButton3(void (*callback)());

  void setTimerHandlerName(const String &handlerName, uint16_t timerSec=1000);
  void handleHtmlFormRequest(WebServer * server);
  void handleGetValues(WebServer *server);
  void handleSetValues(WebServer *server);

private:
  void buildSendHtml(WebServer * server, const char *parameter, uint32_t jsonStartPos);
  void sendContentHtml(WebServer *server, const char *buf, bool send);

  void createHtmlTextfield      (char * buf, uint16_t name, const uint64_t &nameExt, const String &label, const char *parameter, uint8_t idx, uint32_t startPos, const char * type, String value);
  void createHtmlTextarea       (char * buf, uint16_t name, const uint64_t &nameExt, const String &label, const char *parameter, uint8_t idx, uint32_t startPos, String value);
  void createHtmlNumber         (char * buf, uint16_t name, const uint64_t &nameExt, const String &label, const char *parameter, uint8_t idx, uint32_t startPos, String value);
  void createHtmlFloat          (char * buf, uint16_t name, const uint64_t &nameExt, const String &label, const char *parameter, uint8_t idx, uint32_t startPos, String value);
  void createHtmlRange          (char * buf, uint16_t name, const uint64_t &nameExt, const String &label, const char *parameter, uint8_t idx, uint32_t startPos, String value);
  void createHtmlCheckbox       (char * buf, uint16_t name, const uint64_t &nameExt, const String &label, const char *parameter, uint8_t idx, uint32_t startPos, String value);
  void createHtmlStartSelect    (char * buf, const uint64_t &nameExt, const String &label, const char *parameter, uint8_t idx, uint32_t startPos);
  void createHtmlAddSelectOption(char * buf, const String &option, const String &label, const String &value);
  void createHtmlStartMulti     (char * buf, const String &label, const char *parameter, uint8_t idx, uint32_t startPos, uint8_t u8_jsonType);
  void createHtmlAddMultiOption (char * buf, uint16_t name, const uint64_t &nameExt, const char *parameter, uint8_t idx, uint32_t startPos, uint8_t option, const String &label, uint32_t value, uint8_t u8_dataType);

  void (*fn_mOnButtonSave)() = nullptr;
  void (*fn_mOnButton1)()    = nullptr;
  void (*fn_mOnButton2)()    = nullptr;
  void (*fn_mOnButton3)()    = nullptr;

private:
  SemaphoreHandle_t mMutex {nullptr};
  web::WebSettingsStorage *mMgr {nullptr};
  const char *mParameterFile {nullptr};
  String   mConfName {};
  String   mAjaxGetDataTimerHandlerName {};
  uint16_t mAjaxGetDataTimerSec {0};

  uint8_t  u8_mButtons {0};
  String   str_mButton1Text {};
  String   str_mButton2Text {};
  String   str_mButton3Text {};
};

#endif
