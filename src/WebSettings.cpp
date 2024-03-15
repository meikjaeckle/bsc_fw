// Copyright (c) 2022 tobias
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#include <cstring>
#include <Arduino.h>
#include <FS.h>
#include <WebServer.h>

#include "defines.h"
#include "log.h"
#include <json/Utils.h>
#include <web/Utils.h>
#include <web/WebSettingsStorage.h>
#include "web/webSettings_web.h"
#include "WebSettings.h"

static constexpr const char * TAG = "WEB_SETTINGS";

static char _buf[2000] {};
static String st_mSendBuf = "";

bool bo_hasNewKeys=false;

uint8_t u8_mAktOptionGroupNr {};

// static Preferences prefs;

//HTML templates
const char HTML_START[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html lang='de'>
<head>
<meta http-equiv='Content-Type' content='text/html' charset='utf-8'/>
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>BSC</title>
)rawliteral";

const char HTML_START_2[] PROGMEM = R"rawliteral(
</head>
<body>
<div class="topnav">
  <span class='btnBack' onclick=location.href='../'>&#10094;</span>
  <span class='btnBack' onclick=location.href='/'>&#10094;&#10094;</span>
  <span class='hl'>%s</span>
</div>
<div id='lc' class="loading-container"><div class="loading-spinner"></div></div>
<div class="content">
<form>
<table>
)rawliteral";

const char HTML_END_1[] PROGMEM = R"rawliteral(
</table>
</form>
)rawliteral";

const char HTML_END_2[] PROGMEM = R"rawliteral(
</div>
<br><div id='data_div'></div>
)rawliteral";

const char HTML_END_3[] PROGMEM = R"rawliteral(
</body>
</html>
)rawliteral";

const char HTML_ENTRY_TEXTFIELD[] PROGMEM =
"<tr class='Ctr'><td class='Ctd'><b>%s</b></td>\n"
"<td class='Ctd'><input type='%s' value='%s' name='%s'>&nbsp;%s</td><td class='t1'></td><td class='Ctd'><span class='secVal' id='s%s'></span></td></tr>\n";
const char HTML_ENTRY_AREA[] PROGMEM =
"<tr class='Ctr'><td class='Ctd'><b>%s</b></td>\n"
"<td class='Ctd'><textarea rows='%i' cols='%i' name='%s'>%s</textarea></td><td class='t1'></td><td class='Ctd'><span class='secVal' id='s%s'></span></td></tr>\n"; //
const char HTML_ENTRY_FLOAT[] PROGMEM =
"<tr class='Ctr'><td class='Ctd'><b>%s</b></td>\n"
"<td class='Ctd'><input type='number' step='%s' min='%i' max='%i' value='%s' name='%s'>&nbsp;%s</td><td class='t1'></td><td class='Ctd'><span class='secVal' id='s%s'></span></td></tr>\n";
const char HTML_ENTRY_NUMBER[] PROGMEM =
"<tr class='Ctr'><td class='Ctd'><b>%s</b></td>\n"
"<td class='Ctd'><input type='number' min='%i' max='%i' value='%s' name='%s'>&nbsp;%s</td><td class='t1'></td><td class='Ctd'><span class='secVal' id='s%s'></span></td></tr>\n";
const char HTML_ENTRY_RANGE[] PROGMEM =
"<tr class='Ctr'><td class='Ctd'><b>%s</b></td>\n"
"<td class='Ctd'>%i&nbsp;<input type='range' min='%i' max='%i' value='%s' name='%s'>&nbsp;%i</td><td class='t1'></td><td class='Ctd'><span class='secVal' id='s%s'></span></td></tr>\n";
const char HTML_ENTRY_CHECKBOX[] PROGMEM =
"<tr class='Ctr'><td class='Ctd'><b>%s</b></td><td class='Ctd'><input type='checkbox' %s name='%s'></td><td class='t1'></td><td class='Ctd'><span class='secVal' id='s%s'></span></td></tr>\n";

const char HTML_ENTRY_SELECT_START[] PROGMEM =
"<tr class='Ctr'><td class='Ctd'><b>%s</b></td>\n"
"<td class='Ctd'><select name='%s'>\n";
const char HTML_ENTRY_SELECT_OPTION[] PROGMEM =
"<option value='%s' %s>%s</option>\n";
const char HTML_ENTRY_SELECT_END[] PROGMEM =
"</select></td><td class='t1'></td><td class='Ctd'><span class='secVal' id='s%s'></span></td></tr>\n";

const char HTML_ENTRY_MULTI_START[] PROGMEM =
"<tr class='Ctr'><td class='Ctd'><b>%s</b></td>\n"
"<td class='Ctd'><fieldset style='text-align:left;'>\n";
const char HTML_ENTRY_MULTI_OPTION[] PROGMEM =
"<input type='checkbox' name='%s' value='%i' %s>%s<br>\n";
const char HTML_ENTRY_MULTI_END[] PROGMEM =
"</fieldset></td><td class='t1'></td></tr>\n";  //<td class='Ctd'><span class='secVal' id='s%s'></span></td>

const char HTML_ENTRY_MULTI_COLLAPSIBLE_START[] PROGMEM =
"<tr class='Ctr'><td class='Ctd'><b>%s</b></td>\n"
"<td class='Ctd'>\n"
"<input id='t%i' class='toggle' type='checkbox'>\n"
"<label for='t%i' class='lbl-toggle'>%s</label>\n"
"<div class='collapsible-content'>\n"
"<div class='content-inner'>\n"
"<fieldset style='text-align:left;'>\n";
const char HTML_ENTRY_MULTI_COLLAPSIBLE_END[] PROGMEM =
"</fieldset></div></div></td><td class='t1'></td></tr>\n";

const char HTML_GROUP_START[] PROGMEM = "<tr><td class='Ctd2' colspan='3'><b>%s</b></td></tr>\n";
const char HTML_GROUP_START_DETAILS[] PROGMEM = "</table><details><summary><b>%s</b></summary><table>\n"; //<details open>
const char HTML_GROUP_END_DETAILS[]   PROGMEM = "</table></details><table>\n";

const char HTML_ENTRY_SEPARATION[] PROGMEM = "<tr class='Ctr'><td class='sep' colspan='3'><b><u>%s</u></b></td></tr>\n";

const char HTML_BUTTON[] PROGMEM = "<div class='Ctd'><button id='%s' onclick='btnClick(\"%s\")'>%s</button></div>\n";


WebSettings::WebSettings()
  : mMutex(xSemaphoreCreateMutex())
{
  // TODO MEJ Why is this static???
  u8_mAktOptionGroupNr=0;
};


void WebSettings::init(web::WebSettingsStorage &mgr, const char *parameter, const String &confName)
{
  mMgr = &mgr;
  mConfName = confName;

  mParameterFile = parameter;

  const bool hasNewKeys = mMgr->addDefaultValuesFromNewKeys(mParameterFile, 8, confName);
  if (hasNewKeys)
  {
    mMgr->sync();
  }

  #ifdef WEBSET_DEBUG
  BSC_LOGI(TAG, "init() end");
  #endif
}

// --------------------------------------------------------
// TODO MEJ Following methods are used by libwebapp2 library, that's why we have to provide a wrapper until libwebapp2 is updated
boolean WebSettings::writeConfig()
{
  return WEBSETTINGS.sync();
}

/* static */
int32_t WebSettings::getInt(uint16_t name, uint8_t groupNr, uint8_t dataType)
{
  return WEBSETTINGS.getInt(name, groupNr, dataType);
}

/* static */
int32_t WebSettings::getIntFlash(uint16_t name, uint8_t groupNr, uint8_t dataType)
{
  return WEBSETTINGS.getIntFlash(name, groupNr, dataType);
}
// --------------------------------------------------------

void WebSettings::setTimerHandlerName(const String &handlerName, uint16_t timerSec)
{
  mAjaxGetDataTimerHandlerName = handlerName;
  mAjaxGetDataTimerSec = timerSec;
}

void WebSettings::sendContentHtml(WebServer *server, const char *buf, bool send)
{
  assert(buf);

  if (nullptr == buf)
    return;

  //send==true - > buf wird jetzt gesendet
  //send==false -> buf wird angehängt bis max. Größe erreicht ist

  const std::size_t length = std::strlen(buf);
  if(length > 500)
  {
    send=true;
  }
  else
  {
    st_mSendBuf += buf;
  }

  if(send==true || (st_mSendBuf.length() > 2000))
  {
    if(!st_mSendBuf.isEmpty())
      server->sendContent(st_mSendBuf);

    st_mSendBuf.clear();
  }

  if(length > 500)
  {
    server->sendContent(buf);
  }
}

//Function to response HTTP request from the form to save data
void WebSettings::handleHtmlFormRequest(WebServer * server)
{
  assert(server); // TODO MEJ pass server by reference
  assert(mMgr);

  if(json::getArraySize(mParameterFile, 8) == 0)
  {
    BSC_LOGI(TAG,"Error read json");
  }
  else
  {
    bool exit = false;
    if (server->hasArg(F("SAVE")))
    {
      for(uint8_t i=0; i < server->args(); ++i)
      {
        const String argName (std::move(server->argName(i)));
        if(argName != "SAVE")
        {
          const String argValue(std::move(server->arg(i)));

          #ifdef WEBSET_DEBUG
          BSC_LOGD(TAG, "SAVE: name:%s, val:%s", argName, argValue);
          #endif

          if (web::isNumber(argName)) //Wenn keine Zahl, dann Fehler
          {
            const uint64_t u64_argName  = strtoull(argName.c_str(), nullptr, 10);
            const uint16_t u16_argName  = static_cast<uint16_t>(u64_argName & 0xFFFF);
            const uint8_t  datatype     = static_cast<uint8_t>((u64_argName >> 32) & 0xff);
            const bool     storeToPrefs = (u64_argName & ((uint64_t)1<<40)) ? true : false;

            mMgr->writeValue(u16_argName, argValue, datatype, storeToPrefs);

            if (!storeToPrefs) // Preference values are already stored in flash.
              mMgr->sync(); // Write RAM data to flash

            server->send(200, "text/html", "OK"); //Sende ok an Website (Rückmeldung, dass gespeichert wurde)
          }
        }
      }

      //Lesen der Values von der Webseite
      //readWebValues(server, mParameterFile, 0);

      if (fn_mOnButtonSave)
      {
        fn_mOnButtonSave();
        exit = true;
      }
      return;
    }
    if (server->hasArg(F("BTN1")) && fn_mOnButton1)
    {
      fn_mOnButton1();
      exit = true;
    }
    if (server->hasArg(F("BTN2")) && fn_mOnButton2)
    {
      fn_mOnButton2();
      exit = true;
    }
    if (server->hasArg(F("BTN3")) && fn_mOnButton3)
    {
      fn_mOnButton3();
      exit = true;
    }

    //Senden der HTML Seite
    if(!exit)
    {
      server->setContentLength(CONTENT_LENGTH_UNKNOWN);
      sprintf(_buf,HTML_START/*,BACKGROUND_COLOR*/);
      server->send(200, "text/html", _buf);

      sendContentHtml(server,webSettingsStyle,false);

      sprintf(_buf,HTML_START_2,mConfName.c_str());
      sendContentHtml(server,_buf,false);

      buildSendHtml(server, mParameterFile, 8);

      sendContentHtml(server,HTML_END_1,false);

      //Buttons einfügen
      if ((u8_mButtons & 0x02) == 0x02) //Button 1
      {
        sprintf(_buf,HTML_BUTTON,"btn1","BTN1",str_mButton1Text.c_str());
        sendContentHtml(server,_buf,false);
      }
      if ((u8_mButtons & 0x04) == 0x04) //Button 2
      {
        sprintf(_buf,HTML_BUTTON,"btn2","BTN2",str_mButton2Text.c_str());
        sendContentHtml(server,_buf,false);
      }
      if ((u8_mButtons & 0x08) == 0x08) //Button 3
      {
        sprintf(_buf,HTML_BUTTON,"btn3","BTN3",str_mButton3Text.c_str());
        sendContentHtml(server,_buf,false);
      }

      sendContentHtml(server,HTML_END_2,false);
      sendContentHtml(server,webSettingsScript,false);

      //Timer bei bedarf einfügen
      if(!mAjaxGetDataTimerHandlerName.isEmpty())
      {
        sprintf(_buf,webSettingsScript_Timer,mAjaxGetDataTimerHandlerName.c_str(),mAjaxGetDataTimerSec);
        sendContentHtml(server,_buf,false);
      }

      sendContentHtml(server,HTML_END_3,true);
    }
  }
}

/*not use*/
#if 0
void WebSettings::readWebValues(WebServer * server, const String *parameter, uint32_t jsonStartPos)
{
  uint8_t g, optionGroupSize;
  String tmpStr = "";

  BSC_LOGD(tag,"SAVE readWebValues()");

  for (uint8_t a=0; a<json.getArraySize(parameter, jsonStartPos); a++)
  {
    uint8_t type, optionCnt, jsonSize;
    uint32_t jsonName, jsonNameBase;

    type = web::getJsonType(parameter, a, jsonStartPos);
    optionCnt = web::getJsonOptionsCnt(parameter, a, jsonStartPos);
    jsonSize = web::getJsonSize(parameter, a, jsonStartPos);
    jsonNameBase = web::getJsonName(parameter, a, jsonStartPos);

    for(uint8_t n=0; n<jsonSize; n++)
    {
      tmpStr = "";
      jsonName = getParmId(jsonNameBase,u8_mSettingNr,u8_mAktOptionGroupNr,n);

      if (type == HTML_OPTIONGROUP)
      {
        optionGroupSize = web::getJsonGroupsize(parameter,a,jsonStartPos);
        for(g=0; g<optionGroupSize; g++)
        {
          u8_mAktOptionGroupNr=g;
          String retStr;
          uint32_t jsonArrayGroupStart = 0;
          bool ret = json.getValue(parameter, a, "group", jsonStartPos, retStr, jsonArrayGroupStart);
          readWebValues(server, parameter, jsonArrayGroupStart);
        }
        u8_mAktOptionGroupNr = 0;
        break;
      }
      else if (type == HTML_INPUTCHECKBOX)
      {
        if (server->hasArg(String(jsonName))){setString(jsonName,"1");}
        else{setString(jsonName,"0");}
      }
      else
      {
        tmpStr = server->arg(String(jsonName));
        if (server->hasArg(String(jsonName))) setString(jsonName,tmpStr);
      }
    }
  }
}
#endif

void WebSettings::buildSendHtml(WebServer * server, const char *parameter, uint32_t jsonStartPos)
{
  assert(server);
  assert(mMgr);

  uint8_t g, optionGroupSize, jsonSize, u8_dataType, u8_jsonLabelOffset;
  uint16_t u16_jsonName, jsonNameBase;
  uint64_t u64_jsonName;
  String jsonLabel, st_jsonLabelEntry, strlHelp;
  boolean bo_lIsGroup, bo_loadFromFlash;

  bo_lIsGroup=false;
  bo_loadFromFlash=false;
  u8_dataType=0;

  for (uint8_t a {0}; a < json::getArraySize(parameter, jsonStartPos); ++a)
  {
    u64_jsonName = 0;
    jsonLabel = web::getJsonLabel(parameter, a, jsonStartPos);
    jsonNameBase = web::getJsonName(parameter, a, jsonStartPos);

    bo_loadFromFlash=false;
    u8_dataType=0;
    u16_jsonName = web::getParmId(jsonNameBase,u8_mAktOptionGroupNr);
    u64_jsonName = u16_jsonName;

    //Load from RAM or Flash
    u8_dataType = (uint8_t)web::getJson_Key(parameter, "dt", a, jsonStartPos, "").toInt();
    u64_jsonName = u64_jsonName | ((uint64_t)u8_dataType<<32); // | (1ULL<<40);
    if(web::getJson_Key(parameter, "flash", a, jsonStartPos, "").equals("1"))
    {
      u64_jsonName |= (1ULL<<40);
      bo_loadFromFlash=true;
    }

    uint8_t u8_lJsonType = web::getJsonType(parameter, a, jsonStartPos);
    uint16_t u16_lDepId;
    uint8_t u8_lDepVal, u8_lDepDt;
    switch(u8_lJsonType)
    {
      case HTML_OPTIONGROUP:
      case HTML_OPTIONGROUP_COLLAPSIBLE:
        //bo_lIsGroup=true;
        st_jsonLabelEntry = web::getJsonLabelEntry(parameter, a, jsonStartPos);
        u8_jsonLabelOffset = web::getJson_Key(parameter, "label_offset", a, jsonStartPos, "0").toInt();
        optionGroupSize = web::getJsonGroupsize(parameter, a, jsonStartPos);
        u16_lDepId = (uint16_t)web::getJson_Key(parameter, "depId", a, jsonStartPos, "0").toInt(); //depence
        u8_lDepVal = (uint8_t)web::getJson_Key(parameter, "depVal", a, jsonStartPos, "0").toInt();
        u8_lDepDt = (uint8_t)web::getJson_Key(parameter, "depDt", a, jsonStartPos, "1").toInt();
        if(optionGroupSize>1 && !jsonLabel.equals(""))
        {
          if(u8_lJsonType==HTML_OPTIONGROUP_COLLAPSIBLE) sprintf(_buf,HTML_GROUP_START_DETAILS,jsonLabel.c_str());
          else sprintf(_buf,HTML_GROUP_START,jsonLabel.c_str());
          sendContentHtml(server,_buf,false);
        }
        for(g=0; g<optionGroupSize; g++)
        {
          u8_mAktOptionGroupNr=g;
          //BSC_LOGI(TAG,"u16_lDepId=%i, u8_lDepVal=%i, u8_lDepDt=%i, g=%i, val=%i",u16_lDepId,u8_lDepVal,u8_lDepDt,g,getInt(u16_lDepId,g,u8_lDepDt));
          if(mMgr->getInt(u16_lDepId,g,u8_lDepDt)!=u8_lDepVal) continue; //depence
          //BSC_LOGI(TAG,"DEP OK");

          sprintf(_buf,"<tr><td colspan='3'><b>%s %i</b></td></tr>",st_jsonLabelEntry.c_str(), g+u8_jsonLabelOffset);
          sendContentHtml(server,_buf,false);

          String retStr;
          uint32_t jsonArrayGroupStart = 0;
          bool ret = json::getValue(parameter, a, "group", jsonStartPos, retStr, jsonArrayGroupStart);
          buildSendHtml(server, parameter, jsonArrayGroupStart);

          sprintf(_buf,"<tr><td colspan='3'><hr style='border:none; border-top:1px dashed black; height:1px; color:#000000; background:transparent'></td></tr>");
          sendContentHtml(server,_buf,false);
        }
        if(u8_lJsonType==HTML_OPTIONGROUP_COLLAPSIBLE && optionGroupSize>1 && !jsonLabel.isEmpty())
        {
          sprintf(_buf,HTML_GROUP_END_DETAILS);
          sendContentHtml(server,_buf,false);
        }
        u8_mAktOptionGroupNr = 0;
        break;
      case HTML_INPUTTEXT: // TODO MEJ Why is u64_jsonName passed as first argument of readValue(..). The readValue parameter 1 itself is only uint16_t
        createHtmlTextfield(_buf, u16_jsonName, u64_jsonName, jsonLabel, parameter, a, jsonStartPos, "text", mMgr->readValue(u64_jsonName, u8_dataType, bo_loadFromFlash));
        BSC_LOGD(TAG,"buildSendHtml HTML_INPUTTEXT: %s", _buf);
        break;
      case HTML_INPUTTEXTAREA:
        createHtmlTextarea(_buf, u16_jsonName, u64_jsonName, jsonLabel, parameter, a, jsonStartPos, mMgr->readValue(u64_jsonName, u8_dataType, bo_loadFromFlash));
          break;
      case HTML_INPUTPASSWORD:
        createHtmlTextfield(_buf, u16_jsonName, u64_jsonName, jsonLabel, parameter, a, jsonStartPos, "password", mMgr->readValue(u64_jsonName, u8_dataType, bo_loadFromFlash));
        break;
      case HTML_INPUTDATE:
        createHtmlTextfield(_buf, u16_jsonName, u64_jsonName, jsonLabel, parameter, a, jsonStartPos, "date", mMgr->readValue(u64_jsonName, u8_dataType, bo_loadFromFlash));
        break;
      case HTML_INPUTTIME:
        createHtmlTextfield(_buf, u16_jsonName, u64_jsonName, jsonLabel, parameter, a, jsonStartPos, "time", mMgr->readValue(u64_jsonName, u8_dataType, bo_loadFromFlash));
        break;
      case HTML_INPUTCOLOR:
        createHtmlTextfield(_buf, u16_jsonName, u64_jsonName, jsonLabel, parameter, a, jsonStartPos, "color", mMgr->readValue(u64_jsonName,u8_dataType, bo_loadFromFlash));
        break;
      case HTML_INPUTFLOAT:
        createHtmlFloat(_buf, u16_jsonName, u64_jsonName, jsonLabel, parameter, a, jsonStartPos, mMgr->readValue(u64_jsonName, u8_dataType,bo_loadFromFlash));
        break;
      case HTML_INPUTNUMBER:
        createHtmlNumber(_buf, u16_jsonName, u64_jsonName, jsonLabel, parameter, a, jsonStartPos, mMgr->readValue(u64_jsonName, u8_dataType, bo_loadFromFlash));
        break;
      case HTML_INPUTRANGE:
        createHtmlRange(_buf, u16_jsonName, u64_jsonName, jsonLabel, parameter, a, jsonStartPos, mMgr->readValue(u64_jsonName, u8_dataType, bo_loadFromFlash));
        break;
      case HTML_INPUTCHECKBOX:
        createHtmlCheckbox(_buf, u16_jsonName, u64_jsonName, jsonLabel, parameter, a, jsonStartPos, mMgr->readValue(u64_jsonName, u8_dataType, bo_loadFromFlash));
        break;
      case HTML_INPUTSELECT:
      {
        createHtmlStartSelect(_buf, u64_jsonName, jsonLabel, parameter, a, jsonStartPos);
        // TODO MEJ size of mOptions and mOptionLabels can be different then optionsCnt !!!
        const uint8_t optionsCnt = web::getJsonOptionsCnt(parameter,a,jsonStartPos);
        const std::vector<String> mOptions {std::move(web::getJsonOptionValues(parameter,a,jsonStartPos))};
        const std::vector<String> mOptionLabels {std::move(web::getJsonOptionLabels(parameter,a,jsonStartPos))};
        for (uint8_t j = 0 ; j < optionsCnt; ++j)
        {
          sendContentHtml(server,_buf,false);
          String str_lNewLabel = mOptionLabels[j];
          String str_lDes = mMgr->getStringFlash(web::getJsonArrValue(parameter, "options", "d", j, a, jsonStartPos));
          if(str_lDes.length() > 0)
            str_lNewLabel += " ("+str_lDes+")";

          createHtmlAddSelectOption(_buf, mOptions.at(j), str_lNewLabel, mMgr->readValue(u16_jsonName, u8_dataType, bo_loadFromFlash));
        }

        sendContentHtml(server,_buf,false);
        sprintf(_buf,HTML_ENTRY_SELECT_END,String(u16_jsonName));
      } break;
      case HTML_INPUTMULTICHECK:
      case HTML_INPUTMULTICHECK_COLLAPSIBLE:
      {
        createHtmlStartMulti(_buf, jsonLabel, parameter, a, jsonStartPos, u8_lJsonType);
        const uint8_t optionsCnt = web::getJsonOptionsCnt(parameter,a,jsonStartPos);
        const std::vector<String> mOptionLabels {std::move(web::getJsonOptionLabels(parameter,a,jsonStartPos))};
        for (uint8_t j = 0 ; j<optionsCnt; ++j)
        {
          sendContentHtml(server,_buf,false);
          String str_lNewLabel = mOptionLabels[j];
          String str_lDes = mMgr->getStringFlash(web::getJsonArrValue(parameter, "options", "d", j, a, jsonStartPos));
          if(str_lDes.length()>0)
            str_lNewLabel += " (" + str_lDes + ")";

          createHtmlAddMultiOption(_buf, u16_jsonName, u64_jsonName, parameter, a, jsonStartPos, j, str_lNewLabel, (uint32_t)mMgr->getInt(u16_jsonName,u8_dataType), u8_dataType);
        }
        sendContentHtml(server,_buf,false);
        if(u8_lJsonType==HTML_INPUTMULTICHECK_COLLAPSIBLE)
          strcpy_P(_buf,HTML_ENTRY_MULTI_COLLAPSIBLE_END);
        else
          strcpy_P(_buf,HTML_ENTRY_MULTI_END);;
      } break;
      case HTML_SEPARATION:
        sprintf(_buf,HTML_ENTRY_SEPARATION,jsonLabel.c_str());
        break;
      default:
        _buf[0] = 0;
        break;
    }

    uint8_t u8_lType=web::getJsonType(parameter, a, jsonStartPos);
    if(u8_lType!=HTML_OPTIONGROUP && u8_lType!=HTML_OPTIONGROUP_COLLAPSIBLE)
    {
      sendContentHtml(server,_buf,false);
    }

    //Help einfügen
    if(!bo_lIsGroup)
    {
      strlHelp = web::getJsonHelp(parameter, a, jsonStartPos);
      if(!strlHelp.equals(""))
      {
        strlHelp.replace("\n","<br>");
        sprintf(_buf, "<tr><td colspan='3' class='td0'><div class='help'>%s</div></td></tr>", strlHelp.c_str());
        sendContentHtml(server, _buf, false);
      }
    }
  }
}

void WebSettings::createHtmlTextfield(char * buf,
                                      uint16_t name,
                                      const uint64_t &nameExt,
                                      const String &label,
                                      const char *parameter,
                                      uint8_t idx,
                                      uint32_t startPos,
                                      const char * type,
                                      String value) // Pass by copy because we may modify the value
{
  // Get default value from Json if given value is empty.
  if(value.isEmpty())
    value = std::move(web::getJsonDefault(parameter, idx, startPos));

  sprintf(buf, HTML_ENTRY_TEXTFIELD, label.c_str(),
                                      type,
                                      value.c_str(),
                                      String(nameExt).c_str(),
                                      web::getJson_Key(parameter, "unit", idx, startPos, "").c_str(),
                                      String(name).c_str()); // TODO MEJ before just String(name) was passed instead of String(name).c_str()
}

void WebSettings::createHtmlTextarea(char * buf,
                                     uint16_t name,
                                     const uint64_t &nameExt,
                                     const String &label,
                                     const char *parameter,
                                     uint8_t idx,
                                     uint32_t startPos,
                                     String value) // Pass by copy because we may modify the value
{
  if(value.isEmpty())
    value = std::move(web::getJsonDefault(parameter, idx, startPos));

  sprintf(buf, HTML_ENTRY_AREA, label.c_str(),
                                web::getJsonOptionsMax(parameter, idx, startPos),
                                web::getJsonOptionsMin(parameter, idx, startPos),
                                String(nameExt).c_str(),
                                value.c_str(),
                                String(name).c_str());  // TODO MEJ before just String(name) was passed instead of String(name).c_str()
}

void WebSettings::createHtmlNumber(char * buf, uint16_t name, const uint64_t &nameExt, const String &label, const char *parameter, uint8_t idx, uint32_t startPos, String value)
{
  if(value.isEmpty())
    value = std::move(web::getJsonDefault(parameter, idx, startPos));

  sprintf(buf, HTML_ENTRY_NUMBER, label.c_str(),
                                  web::getJsonOptionsMin(parameter, idx, startPos),
                                  web::getJsonOptionsMax(parameter, idx, startPos),
                                  value.c_str(),
                                  String(nameExt).c_str(),
                                  web::getJson_Key(parameter, "unit", idx, startPos, "").c_str(),
                                  String(name).c_str());  // TODO MEJ before just String(name) was passed instead of String(name).c_str()
}

void WebSettings::createHtmlFloat(char * buf, uint16_t name, const uint64_t &nameExt, const String &label, const char *parameter, uint8_t idx, uint32_t startPos, String value)
{
  if(value.isEmpty())
    value = std::move(web::getJsonDefault(parameter, idx, startPos));

  sprintf(buf, HTML_ENTRY_FLOAT, label.c_str(),
                                 web::getJson_Key(parameter, "step", idx, startPos, "0.01").c_str(),
                                 web::getJsonOptionsMin(parameter, idx, startPos),
                                 web::getJsonOptionsMax(parameter, idx, startPos),
                                 value.c_str(),
                                 String(nameExt).c_str(),
                                 web::getJson_Key(parameter, "unit", idx, startPos, "").c_str(),
                                 String(name).c_str());  // TODO MEJ before just String(name) was passed instead of String(name).c_str()
}

void WebSettings::createHtmlRange(char * buf, uint16_t name, const uint64_t &nameExt, const String &label, const char *parameter, uint8_t idx, uint32_t startPos, String value)
{
  if(value.isEmpty())
    value = std::move(web::getJsonDefault(parameter, idx, startPos));

  sprintf(buf,HTML_ENTRY_RANGE, label.c_str(),
                                web::getJsonOptionsMin(parameter, idx, startPos),
                                web::getJsonOptionsMin(parameter, idx, startPos),
                                web::getJsonOptionsMax(parameter, idx, startPos),
                                value.c_str(),
                                String(nameExt).c_str(), // MEJ TODO: nameExt was passed befor as pointer
                                web::getJsonOptionsMax(parameter, idx, startPos),
                                String(name).c_str()); // TODO MEJ before just String(name) was passed instead of String(name).c_str()
}

void WebSettings::createHtmlCheckbox(char * buf, uint16_t name, const uint64_t &nameExt, const String &label, const char *parameter, uint8_t idx, uint32_t startPos, String value)
{
  if(value.isEmpty())
    value = std::move(web::getJsonDefault(parameter, idx, startPos));

  String sValue(name);
  if (!value.equals("0")) {
    sprintf(buf, HTML_ENTRY_CHECKBOX, label.c_str(), "checked", String(nameExt).c_str(), sValue.c_str());
    //sprintf(buf, HTML_ENTRY_CHECKBOX, label.c_str(), "checked", String(nameExt).c_str(), String(name)); // <= original TODO MEJ Why is last parameter String(name) would expect .c_str()
  } else {
    sprintf(buf, HTML_ENTRY_CHECKBOX, label.c_str(), "", String(nameExt).c_str(), sValue.c_str());
  }
}

void WebSettings::createHtmlStartSelect(char * buf, const uint64_t &nameExt, const String &label, const char * /* parameter */, uint8_t /* idx */, uint32_t /* startPos */)
{
  sprintf(buf, HTML_ENTRY_SELECT_START, label.c_str(), String(nameExt).c_str());
}

void WebSettings::createHtmlAddSelectOption(char * buf, const String &option, const String &label, const String &value)
{
  if (option.equals(value))
    sprintf(buf, HTML_ENTRY_SELECT_OPTION, option.c_str(), "selected", label.c_str());
  else
    sprintf(buf, HTML_ENTRY_SELECT_OPTION, option.c_str(), "", label.c_str());
}

void WebSettings::createHtmlStartMulti(char * buf, const String &label, const char */* parameter */, uint8_t /* idx */, uint32_t /* startPos */, uint8_t u8_jsonType)
{
  if(u8_jsonType==HTML_INPUTMULTICHECK_COLLAPSIBLE)
  {
    const int32_t u32_lMillis = millis();
    sprintf(buf, HTML_ENTRY_MULTI_COLLAPSIBLE_START, label.c_str(), u32_lMillis, u32_lMillis, label.c_str());
  }
  else
  {
    sprintf(buf, HTML_ENTRY_MULTI_START, label.c_str());
  }
}

void WebSettings::createHtmlAddMultiOption(char * buf,
                                           uint16_t name,
                                           const uint64_t &nameExt,
                                           const char *parameter,
                                           uint8_t idx,
                                           uint32_t startPos,
                                           uint8_t option,
                                           const String &label,
                                           uint32_t value,
                                           uint8_t u8_dataType)
{
  #ifdef WEBSET_DEBUG
  BSC_LOGD(TAG,"createHtmlAddMultiOption: option=%i, value=%i",option,value);
  #endif

  if(!mMgr->isKeyExist(name,u8_dataType))
    value = web::getJsonDefault(parameter, idx, startPos).toInt();

  if(value & (1 << option))
    sprintf(buf, HTML_ENTRY_MULTI_OPTION, String(nameExt).c_str(), option, "checked", label.c_str());
  else
    sprintf(buf, HTML_ENTRY_MULTI_OPTION, String(nameExt).c_str(), option, "", label.c_str());
}

void WebSettings::setButtons(uint8_t buttons, const String &btnLabel)
{
  if(buttons==BUTTON_1)
  {
    str_mButton1Text = btnLabel;
    u8_mButtons |= (1<<BUTTON_1);
  }
  else if(buttons==BUTTON_2)
  {
    str_mButton2Text = btnLabel;
    u8_mButtons |= (1<<BUTTON_2);
  }
  else if(buttons==BUTTON_3)
  {
    str_mButton3Text = btnLabel;
    u8_mButtons |= (1<<BUTTON_3);
  }
}

//register callback for Buttons
void WebSettings::registerOnSave(void (*callback)())
{
  fn_mOnButtonSave = callback;
}
void WebSettings::registerOnButton1(void (*callback)())
{
  fn_mOnButton1 = callback;
}
void WebSettings::registerOnButton2(void (*callback)())
{
  fn_mOnButton2 = callback;
}
void WebSettings::registerOnButton3(void (*callback)())
{
  fn_mOnButton3 = callback;
}

#ifdef INSIDER_V1
void addJsonElem(char *buf, int id, bool val)
{
  uint16_t bufLen = strlen(buf);
  if (bufLen == 0) sprintf(&buf[bufLen], "[");
  else sprintf(&buf[bufLen], ",");
  bufLen++;
  sprintf(&buf[bufLen], "{\"id\":%i, \"val\":%u}",id,val);
}
void addJsonElem(char *buf, int id, int32_t val)
{
  uint16_t bufLen = strlen(buf);
  if (bufLen == 0) sprintf(&buf[bufLen], "[");
  else sprintf(&buf[bufLen], ",");
  bufLen++;
  sprintf(&buf[bufLen], "{\"id\":%i, \"val\":%i}",id,val);
}
void addJsonElem(char *buf, int id, float val)
{
  uint16_t bufLen = strlen(buf);
  if (bufLen == 0) sprintf(&buf[bufLen], "[");
  else sprintf(&buf[bufLen], ",");
  bufLen++;
  sprintf(&buf[bufLen], "{\"id\":%i, \"val\":%.3f}",id,val);
}
void addJsonElem(char *buf, int id, String val)
{
  uint16_t bufLen = strlen(buf);
  if (bufLen == 0) sprintf(&buf[bufLen], "[");
  else sprintf(&buf[bufLen], ",");
  bufLen++;
  sprintf(&buf[bufLen], "{\"id\":%i, \"val\":\"%s\"}",id,val.c_str());
}

void WebSettings::handleGetValues(WebServer *server)
{
  uint32_t argName;
  char data[4096] {}; // TODO MEJ Enough stack memory ???
  //BSC_LOGI(TAG,"handleGetValues: args=%i",server->args());
  for(uint8_t i=0; i<server->args(); i++)
  {
    //BSC_LOGI(TAG,"handleGetValues: argNr=%i",i);
    //BSC_LOGI(TAG,"handleGetValues: name=%s, arg=%s",server->argName(i).c_str(), server->arg(i).c_str());
    if(web::isNumber(server->argName(i))) //Wenn keine Zahl, dann Fehler
    {
      argName = static_cast<uint32_t>(server->argName(i).toInt());

      uint8_t u8_storeInFlash = ((argName>>16)&0xff);
      uint8_t u8_dataType = ((argName>>24)&0xff);

      if(u8_storeInFlash==0)
      {
        if(u8_dataType==PARAM_DT_BO)
          addJsonElem(data, argName, mMgr->getBool((uint16_t)(argName&0xFFFF)));
        else if(u8_dataType==PARAM_DT_FL)
          addJsonElem(data, argName, mMgr->getFloat((uint16_t)(argName&0xFFFF)));
        else if(u8_dataType==PARAM_DT_ST)
          addJsonElem(data, argName, mMgr->readValue((uint16_t)(argName&0xFFFF),u8_dataType, false));
        else
          addJsonElem(data, argName, mMgr->getInt((uint16_t)(argName&0xFFFF), u8_dataType));
      }
      else if(u8_storeInFlash==1)
      {
        if(u8_dataType==PARAM_DT_BO)
          addJsonElem(data, argName, mMgr->getBoolFlash((uint16_t)(argName&0xFFFF)));
        else if(u8_dataType==PARAM_DT_FL)
          addJsonElem(data, argName, mMgr->getFloatFlash((uint16_t)(argName&0xFFFF)));
        else if(u8_dataType==PARAM_DT_ST)
          addJsonElem(data, argName, mMgr->getStringFlash((uint16_t)(argName&0xFFFF)));
        else
          addJsonElem(data, argName, mMgr->getIntFlash((uint16_t)(argName&0xFFFF), u8_dataType));
      }
    }
    else
    {
      BSC_LOGE(TAG,"handleGetValues: not number");
    }
  }
  sprintf(&data[strlen(data)], "]"); // TODO MEJ snprintf

  server->send(200, "application/json", data);
}

void WebSettings::handleSetValues(WebServer *server)
{
  assert(server); // TODO MEJ pass server by reference
  assert(mMgr);
  //BSC_LOGI(TAG,"handleSetValues args=%i",server->args());

  for(int i {0}; i < server->args(); ++i)
  {
    const String argName (std::move(server->argName(i)));
    //BSC_LOGI(TAG,"handleSetValues: argNr=%i",i);
    //BSC_LOGI(TAG,"handleSetValues: name=%s, arg=%s", argName.c_str(), server->arg(i).c_str());
    if(web::isNumber(argName)) //Wenn keine Zahl, dann Fehler
    {
      const String   argValue     {std::move(server->arg(i))};
      const uint32_t u32_argName  {static_cast<uint32_t>(argName.toInt())};
      const uint16_t u16_argName  {static_cast<uint16_t>(u32_argName & 0xFFFF)};
      const uint8_t  datatype     {static_cast<uint8_t>((u32_argName>>24) & 0xFF)};
      const bool     storeToPrefs {(u32_argName & ((uint32_t)1<<16)) ? true : false};
      //BSC_LOGI(TAG,"handleSetValues argNr=%i, argName=%s, val=%s", i, argName.c_str(), argValue.c_str());
      mMgr->writeValue(u16_argName, argValue, datatype, storeToPrefs);

    }
    /*else
    {
      BSC_LOGE(TAG,"handleSetValues: not number");
    }*/
  }
  server->send(200, "application/json", "{\"state\":1}");
}
#endif










