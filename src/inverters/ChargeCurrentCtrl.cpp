// Copyright (c) 2024 Tobias Himmler
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#include "inverter/ChargeCurrentCtrl.hpp"
#include "inverter/BmsDataUtils.hpp"
#include "defines.h"
#include "WebSettings.h"

namespace inverters
{

void ChargeCurrentCtrl::calcChargCurrent(Inverter &inverter, inverters::InverterData &inverterData, bool alarmSetChargeCurrentToZero)
{
  int16_t i16_lMaxChargeCurrentList[4] {};
  int16_t i16_mNewChargeCurrent {0};

  if(alarmSetChargeCurrentToZero)
  {
    i16_mNewChargeCurrent = 0;
  }
  else
  {
    //int16_t i16_lMaxChargeCurrentOld=inverterData.inverterChargeCurrent;
    int16_t i16_lMaxChargeCurrent = (int16_t)(WebSettings::getInt(ID_PARAM_BMS_MAX_CHARGE_CURRENT,0,DT_ID_PARAM_BMS_MAX_CHARGE_CURRENT));
    //u8_mModulesCntCharge=1;

    //Maximal zulässigen Ladestrom aus den einzelnen Packs errechnen
    if(inverterData.u16_bmsDatasourceAdd>0)
    {
      uint16_t u16_lMaxCurrent=0;
      for(uint8_t i=0;i<SERIAL_BMS_DEVICES_COUNT;i++)
      {
        if((inverterData.u8_bmsDatasource-BMSDATA_FIRST_DEV_SERIAL)==i || (inverterData.u16_bmsDatasourceAdd>>i)&0x01)
        {
          if(getBmsErrors(BMSDATA_FIRST_DEV_SERIAL+i)==0 && (millis()-getBmsLastDataMillis(BMSDATA_FIRST_DEV_SERIAL+i)<CAN_BMS_COMMUNICATION_TIMEOUT) &&
            getBmsStateFETsCharge(BMSDATA_FIRST_DEV_SERIAL+i))
          {
            u16_lMaxCurrent += WebSettings::getInt(ID_PARAM_BATTERY_PACK_CHARGE_CURRENT,i,DT_ID_PARAM_BATTERY_PACK_CHARGE_CURRENT);
            //u8_mModulesCntCharge++;
          }
        }
      }
      if(u16_lMaxCurrent<i16_lMaxChargeCurrent) i16_lMaxChargeCurrent=u16_lMaxCurrent;
    }
    #ifdef CAN_DEBUG
    BSC_LOGI(TAG,"chargeCurrent Pack:%i",i16_lMaxChargeCurrent);
    #endif

    i16_lMaxChargeCurrentList[0] = calcLadestromZellspanung(inverterData, i16_lMaxChargeCurrent);
    i16_lMaxChargeCurrentList[1] = calcLadestromSocAbhaengig(i16_lMaxChargeCurrent, (uint8_t)inverterData.inverterSoc);
    i16_lMaxChargeCurrentList[2] = calcLadestromBeiZelldrift(inverterData, i16_lMaxChargeCurrent);
    //i16_lMaxChargeCurrentList[3] = calcChargecurrent_MaxCurrentPerPackToHigh(i16_lMaxChargeCurrent); //ToDO: Ein/Aus
    i16_lMaxChargeCurrentList[3] = calcChargeCurrentCutOff(inverterData, i16_lMaxChargeCurrent);

    #ifdef CAN_DEBUG
    BSC_LOGD(TAG,"chargeCurrent Zellspg.:%i, SoC:%i, Zelldrift:%i",i16_lMaxChargeCurrentList[0],i16_lMaxChargeCurrentList[1],i16_lMaxChargeCurrentList[2]);
    #endif

    //Bestimmt kleinsten Ladestrom aller Optionen
    for(uint8_t i=0;i<sizeof(i16_lMaxChargeCurrentList)/sizeof(i16_lMaxChargeCurrentList[0]);i++)
    {
      if(i16_lMaxChargeCurrentList[i] < i16_lMaxChargeCurrent)
      {
        i16_lMaxChargeCurrent = i16_lMaxChargeCurrentList[i];
      }
    }

    i16_mNewChargeCurrent = calcMaximalenLadestromSprung(i16_lMaxChargeCurrent, inverterData.inverterChargeCurrent); //calcMaximalenLadestromSprung

    #ifdef CAN_DEBUG
    BSC_LOGD(TAG, "Soll Ladestrom: %i, %i, %i",i16_lMaxChargeCurrentList[0], i16_lMaxChargeCurrentList[1], i16_lMaxChargeCurrentList[2]);
    BSC_LOGI(TAG,"New charge current: %i, %i",i16_lMaxChargeCurrent,i16_mNewChargeCurrent);
    #endif
  }

  inverter.inverterDataSemaphoreTake();
  inverterData.inverterChargeCurrent = i16_mNewChargeCurrent;

  inverterData.calcChargeCurrentCellVoltage = i16_lMaxChargeCurrentList[0];
  inverterData.calcChargeCurrentSoc = i16_lMaxChargeCurrentList[1];
  inverterData.calcChargeCurrentCelldrift = i16_lMaxChargeCurrentList[2];
  inverterData.calcChargeCurrentCutOff = i16_lMaxChargeCurrentList[3];
  inverter.inverterDataSemaphoreGive();
}


int16_t ChargeCurrentCtrl::calcMaximalenLadestromSprung(int16_t i16_pNewChargeCurrent, int16_t i16_lastChargeCurrent)
{
    //Evtl. Sprünge im Batteriestrom und hohe Lastströme berücksichtigen

    /* Wird der neue Soll-Ladestrom kleiner, dann wird dieser sofort geändert
    * um bei hoher Zellspannung schnell ausregeln zu können.
    * Ist der neue Soll-Ladestrom größer, dann wird dieser nur alle 30 Sekunden geändert. */
    if (i16_pNewChargeCurrent < i16_lastChargeCurrent)
    {
        #ifdef CAN_DEBUG
        BSC_LOGD(TAG, "Sprung unten > 5A (a): i16_pNewChargeCurrent=%i, i16_lastChargeCurrent=%", i16_pNewChargeCurrent, i16_lastChargeCurrent);
        #endif

        if (i16_lastChargeCurrent >= 50)
        {
            i16_pNewChargeCurrent = i16_lastChargeCurrent - 10;
        }
        else if (i16_lastChargeCurrent >= 25 && i16_lastChargeCurrent < 50)
        {
            i16_pNewChargeCurrent = i16_lastChargeCurrent - 5;
        }
        else if (i16_lastChargeCurrent >= 10 && i16_lastChargeCurrent < 25)
        {
            i16_pNewChargeCurrent = i16_lastChargeCurrent - 3;
        }
        else if (i16_lastChargeCurrent < 10)
        {
            i16_pNewChargeCurrent = i16_lastChargeCurrent - 1;
        }
        if (i16_pNewChargeCurrent < 0)
            i16_pNewChargeCurrent = 0;
        #ifdef CAN_DEBUG
        BSC_LOGD(TAG, "Sprung unten: i16_pNewChargeCurrent=%i, i16_mMaxChargeCurrent=%i", i16_pNewChargeCurrent, i16_mMaxChargeCurrent);
        #endif

        return i16_pNewChargeCurrent;
    }
    else
    {
        if (i16_pNewChargeCurrent - i16_lastChargeCurrent > 10) //Maximal 10A Sprünge nach oben erlauben
        {
            i16_pNewChargeCurrent = i16_lastChargeCurrent + 10;
            #ifdef CAN_DEBUG
            BSC_LOGD(TAG, "Sprung oben: i16_pNewChargeCurrent=%i, i16_lastChargeCurrent=%i", i16_pNewChargeCurrent, i16_lastChargeCurrent);
            #endif
        }

        return i16_pNewChargeCurrent;
    }
}

/* Berechnet der Maximalzulässigen Ladestrom anhand der eigestellten Zellspannungsparameter */
int16_t ChargeCurrentCtrl::calcLadestromZellspanung(inverters::InverterData &inverterData, int16_t i16_pMaxChargeCurrent)
{
  if(WebSettings::getBool(ID_PARAM_INVERTER_LADESTROM_REDUZIEREN_ZELLSPG_EN,0)==true) //wenn enabled
  {
    //Maximale Zellspannung von den aktiven BMSen ermitteln
    uint16_t u16_lAktuelleMaxZellspg = BmsDataUtils::getMaxCellSpannungFromBms(inverterData.u8_bmsDatasource, inverterData.u16_bmsDatasourceAdd);

    uint16_t u16_lStartSpg = WebSettings::getInt(ID_PARAM_INVERTER_LADESTROM_REDUZIEREN_ZELLSPG_STARTSPG,0,DT_ID_PARAM_INVERTER_LADESTROM_REDUZIEREN_ZELLSPG_STARTSPG);
    if(u16_lStartSpg<=u16_lAktuelleMaxZellspg)
    {
      uint16_t u16_lEndSpg = WebSettings::getInt(ID_PARAM_INVERTER_LADESTROM_REDUZIEREN_ZELLSPG_ENDSPG,0,DT_ID_PARAM_INVERTER_LADESTROM_REDUZIEREN_ZELLSPG_ENDSPG);
      int16_t i16_lMindestChargeCurrent = (WebSettings::getInt(ID_PARAM_INVERTER_LADESTROM_REDUZIEREN_ZELLSPG_MINDEST_STROM,0,DT_ID_PARAM_INVERTER_LADESTROM_REDUZIEREN_ZELLSPG_MINDEST_STROM));

      if(u16_lStartSpg>u16_lEndSpg) return i16_pMaxChargeCurrent; //Startspannung > Endspannung => Fehler
      if(i16_pMaxChargeCurrent<=i16_lMindestChargeCurrent) return i16_pMaxChargeCurrent; //Maximaler Ladestrom < Mindest-Ladestrom => Fehler

      if(u16_lAktuelleMaxZellspg>u16_lEndSpg)
      {
        //Wenn die aktuelle Zellspannung bereits größer als der Endzellspannung ist,
        //dann Ladestrom auf Mindest-Ladestrom einstellen
        return i16_lMindestChargeCurrent;
      }
      else
      {
        uint32_t u32_lAenderungProMv = ((u16_lEndSpg-u16_lStartSpg)*100)/(i16_pMaxChargeCurrent-i16_lMindestChargeCurrent); //Änderung pro mV
        uint32_t u32_lStromAenderung = ((u16_lAktuelleMaxZellspg-u16_lStartSpg)*100)/u32_lAenderungProMv; //Ladestrom um den theoretisch reduziert werden muss
        if(u32_lStromAenderung>(i16_pMaxChargeCurrent-i16_lMindestChargeCurrent))
        {
          return i16_lMindestChargeCurrent;
        }
        else
        {
          uint16_t u16_lNewChargeCurrent = i16_pMaxChargeCurrent-u32_lStromAenderung; //neuer Ladestrom
          return u16_lNewChargeCurrent;
        }

      }
    }
  }
  return i16_pMaxChargeCurrent;
}


/* */
int16_t ChargeCurrentCtrl::calcLadestromBeiZelldrift(inverters::InverterData &inverterData, int16_t i16_pMaxChargeCurrent)
{
  int16_t i16_lMaxChargeCurrent = i16_pMaxChargeCurrent;

  if(WebSettings::getBool(ID_PARAM_INVERTER_LADESTROM_REDUZIEREN_ZELLDRIFT_EN,0)==true) //wenn enabled
  {
    //Maximalen Ladestrom berechnen
    uint16_t u32_lMaxCellDrift = BmsDataUtils::getMaxCellDifferenceFromBms(inverterData.u8_bmsDatasource, inverterData.u16_bmsDatasourceAdd);
    uint16_t u16_lstartDrift = WebSettings::getInt(ID_PARAM_INVERTER_LADESTROM_REDUZIEREN_STARTABWEICHUNG,0,DT_ID_PARAM_INVERTER_LADESTROM_REDUZIEREN_STARTABWEICHUNG);
    if(u32_lMaxCellDrift>0)
    {
      if(u32_lMaxCellDrift>u16_lstartDrift) //Wenn Drift groß genug ist
      {
        if(BmsDataUtils::getMaxCellSpannungFromBms(inverterData.u8_bmsDatasource, inverterData.u16_bmsDatasourceAdd)>=WebSettings::getInt(ID_PARAM_INVERTER_LADESTROM_REDUZIEREN_STARTSPG_ZELLE,0,DT_ID_PARAM_INVERTER_LADESTROM_REDUZIEREN_STARTSPG_ZELLE)) //Wenn höchste Zellspannung groß genug ist
        {
          i16_lMaxChargeCurrent = i16_lMaxChargeCurrent-((u32_lMaxCellDrift-u16_lstartDrift)*WebSettings::getInt(ID_PARAM_INVERTER_LADESTROM_REDUZIEREN_A_PRO_MV,0,DT_ID_PARAM_INVERTER_LADESTROM_REDUZIEREN_A_PRO_MV));
          if(i16_lMaxChargeCurrent<0) i16_lMaxChargeCurrent=0;
        }
      }
    }
  }

  return i16_lMaxChargeCurrent;
}


/* */
int16_t ChargeCurrentCtrl::calcLadestromSocAbhaengig(int16_t i16_lMaxChargeCurrent, uint8_t u8_lSoc)
{
  if(WebSettings::getBool(ID_PARAM_INVERTER_LADESTROM_REDUZIEREN_SOC_EN,0)==true) //wenn enabled
  {
    uint8_t u8_lReduzierenAbSoc = WebSettings::getInt(ID_PARAM_INVERTER_LADESTROM_REDUZIEREN_AB_SOC,0,DT_ID_PARAM_INVERTER_LADESTROM_REDUZIEREN_AB_SOC);
    if(u8_lSoc>=u8_lReduzierenAbSoc)
    {
      uint8_t u8_lReduzierenUmA = WebSettings::getInt(ID_PARAM_INVERTER_LADESTROM_REDUZIEREN_A_PRO_PERCENT_SOC,0,DT_ID_PARAM_INVERTER_LADESTROM_REDUZIEREN_A_PRO_PERCENT_SOC);

      if(i16_lMaxChargeCurrent-((u8_lSoc-u8_lReduzierenAbSoc+1)*u8_lReduzierenUmA)>=0)
      {
        return i16_lMaxChargeCurrent-((u8_lSoc-u8_lReduzierenAbSoc+1)*u8_lReduzierenUmA);
      }
      else
      {
        return 0;
      }
    }

    return i16_lMaxChargeCurrent;
  }

  return i16_lMaxChargeCurrent;
}


/*
* Ladestrom herunterregeln, wenn von einem Pack die Stromgrenze überschritten wird
*/
/*int16_t ChargeCurrentCtrl::calcChargecurrent_MaxCurrentPerPackToHigh(int16_t i16_pMaxChargeCurrent)
{
  if(u16_mBmsDatasourceAdd>0)
  {
    //uint16_t u16_lMaxCellDiff=0;
    for(uint8_t i=0;i<SERIAL_BMS_DEVICES_COUNT;i++)
    {
      if((u16_mBmsDatasourceAdd>>i)&0x01)
      {
        if((millis()-getBmsLastDataMillis(BMSDATA_FIRST_DEV_SERIAL+i))<CAN_BMS_COMMUNICATION_TIMEOUT) //So lang die letzten 5000ms Daten kamen ist alles gut
        {
          float fl_lTotalCurrent=getBmsTotalCurrent(BMSDATA_FIRST_DEV_SERIAL+i);
          if(fl_lTotalCurrent>(float)WebSettings::getInt(ID_PARAM_BATTERY_PACK_CHARGE_CURRENT,i,DT_ID_PARAM_BATTERY_PACK_CHARGE_CURRENT))
          {
            //Ladestrom für einen Pack (BMS) wird zu groß -> Ladestrom herunterregeln
            #ifdef CAN_DEBUG
            BSC_LOGD(TAG,"MaxCurrentPerPackToHigh: current=%i",i16_pMaxChargeCurrent); //nur zum Debug
            #endif
            return i16_mAktualChargeCurrentSoll-10;
          }
        }
      }
    }
  }

  return i16_pMaxChargeCurrent;
}*/




/********************************************************************************************
 * calcChargeCurrentCutOff(int16_t u16_lChargeCurrent)
 * Ladestrom auf 0 setzen, wenn längere Zeit mit einem geringen Ladestrom geladen wurde.
 *
 ********************************************************************************************/
int16_t ChargeCurrentCtrl::calcChargeCurrentCutOff(inverters::InverterData &inverterData, int16_t u16_lChargeCurrent)
{
  float fl_lTotalCurrent=0;

  uint16_t u16_lCutOffTime = (uint16_t)WebSettings::getInt(ID_PARAM_INVERTER_CHARGE_CURRENT_CUT_OFF_TIME,0,DT_ID_PARAM_INVERTER_CHARGE_CURRENT_CUT_OFF_TIME);
  if(u16_lCutOffTime==0) return u16_lChargeCurrent;

  float fl_lCutOffCurrent = WebSettings::getFloat(ID_PARAM_INVERTER_CHARGE_CURRENT_CUT_OFF_CURRENT,0);
  uint8_t u16_lCutOffSoc = (uint8_t)WebSettings::getInt(ID_PARAM_INVERTER_CHARGE_CURRENT_CUT_OFF_SOC,0,DT_ID_PARAM_INVERTER_CHARGE_CURRENT_CUT_OFF_SOC);

  uint8_t u8_lSoc = (uint8_t)inverterData.inverterSoc;
  fl_lTotalCurrent = inverterData.batteryCurrent/10;

  #ifdef CAN_DEBUG
  uint16_t u16_mChargeCurrentCutOfTimerOld = u16_mChargeCurrentCutOfTimer; //nur fürs Debug
  #endif

  if(inverterData.u16_mChargeCurrentCutOfTimer>=u16_lCutOffTime)
  {
    if(u8_lSoc < u16_lCutOffSoc) //Wenn SoC zur Freigabe wieder unterschritten
    {
      inverterData.u16_mChargeCurrentCutOfTimer=0;
    }
    else u16_lChargeCurrent=0;
  }
  else
  {
    //Timer hochzählen, wenn Strom kleiner
    if(fl_lTotalCurrent<fl_lCutOffCurrent && u8_lSoc>=u16_lCutOffSoc) inverterData.u16_mChargeCurrentCutOfTimer++;
    else inverterData.u16_mChargeCurrentCutOfTimer=0;
  }

  #ifdef CAN_DEBUG
  if(u16_mChargeCurrentCutOfTimerOld!=u16_mChargeCurrentCutOfTimer)
  {
    if(u16_mChargeCurrentCutOfTimer==0 || u16_mChargeCurrentCutOfTimer==1 || u16_mChargeCurrentCutOfTimer==u16_lCutOffTime)
    BSC_LOGI(TAG,"calcChargeCurrentCutOff: u16_mChargeCurrentCutOfTimer=%i, u16_lChargeCurrent=%i", u16_mChargeCurrentCutOfTimer, u16_lChargeCurrent);
  }
  #endif

  return u16_lChargeCurrent;
}
} // namespace inverters