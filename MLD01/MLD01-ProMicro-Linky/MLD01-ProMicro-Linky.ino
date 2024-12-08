/*
  Designed for Pro Micro (ATmega32U4)
  Hardware PCB : MLD01 version >= 1.0

  Mode HISTORIQUE
*/
#include <TeleInfo.h>

//#define DEBUG_SERIAL

#ifdef DEBUG_SERIAL
#define DEBUGinit() Serial.begin(115200)
#define DEBUG(x) Serial.print(x)
#define DEBUGln(x) Serial.println(x)
#else
#define DEBUGinit()
#define DEBUG(x)
#define DEBUGln(x)
#endif

#define RL_SX1278 1
#include <RadioLink.h>

//--- ProMicro LoRa pin ---
// MOSI : 16
// MISO : 14
// SCK  : 15
// NSS  : 10
// DIO0 : 7
// NRST : 18

RadioLinkClass RLcomm;

#define HUB_ID          0

#define SENSOR_ID      10

#define CHILD_PARAM  0xFF

#define CHILD_OPTARIF   1
#define CHILD_BASE      2
#define CHILD_HCHC      3
#define CHILD_HCHP      4

#define CHILD_PAPP     20

TeleInfo teleinfo(&Serial1);

//#define TRIPHASE

#define AVGSIZE 5
long arrPower[AVGSIZE];
byte idxPower = 0;
bool pwrStart = true;
long counter = 0;
uint32_t curTime;
uint32_t Time1s = 0;
uint32_t ConsoTime;
const char oldTarif[10] = {0};

uint8_t waitForConfig = 2; // 2 frames to analyze data and send config

void setup()
{
#ifdef DEBUG_SERIAL
  Serial.begin(115200);
  while (!Serial);
  Serial.println("--- serial debug ON --- ");
#endif

  Serial1.begin(1200);
  teleinfo.begin();

  if (RLcomm.begin(433E6, NULL, NULL, 17, 1))
  {
    RLcomm.setWaitOnTx(true);
    DEBUGln("LORA OK");
  } else
  {
    DEBUGln(F("LORA Error"));
  }
  ConsoTime = millis() + 30000; // 30sec
}

void loop()
{
  static uint32_t seconds = 0;

  curTime = millis();

  if (curTime > Time1s)
  {
    Time1s = curTime + 1000;
    seconds++;
  }

  teleinfo.process();
  if (teleinfo.available())
  {
    // close serial to save cpu charge
    Serial1.end();

    const char* opTarif = teleinfo.getStringVal("OPTARIF");
    long meterBASE = teleinfo.getLongVal("BASE");
    long meterHCHC = teleinfo.getLongVal("HCHC");
    long meterHCHP = teleinfo.getLongVal("HCHP");
    int papp = teleinfo.getLongVal("PAPP");

    DEBUGln(F("--- tele info available ---"));
    DEBUG(F("OPTARIF = "));
    DEBUGln(opTarif);

    if (waitForConfig == 0)
    {
      if (strcmp(opTarif, oldTarif) != 0)
      {
        // Send new Tarif if change
        RLcomm.publishText(HUB_ID, SENSOR_ID, CHILD_OPTARIF, opTarif);
        strcpy(oldTarif, opTarif);
      }

      if (papp >= 0)
      {
        // Send power if value exeed 20w difference of agv 5 last values
        arrPower[idxPower++] = papp;
        if (idxPower >= AVGSIZE)
        {
          idxPower = 0;
          pwrStart = false; // 1rst array is full
        }
        if (pwrStart == false)
        {
          long avgPower = 0;
          for (byte i = 0; i < AVGSIZE; i++)
          {
            avgPower += arrPower[i];
          }
          avgPower = avgPower / AVGSIZE;

          DEBUG(F("Avg Power = "));
          DEBUG(avgPower);
          DEBUG(F(" / "));
          DEBUG(papp);
          DEBUG(F(" -> "));
          DEBUGln(abs(avgPower - papp));

          if (abs(avgPower - papp) > 30)
          {
            for (byte i = 0; i < AVGSIZE; i++)
            {
              arrPower[i] = papp;
            }
            idxPower = 0;
            RLcomm.publishNum(HUB_ID, SENSOR_ID, CHILD_PAPP, papp);
          }
        }
      }

      // Send total counter every 10 minutes in KWh
      if (curTime > ConsoTime)
      {
        ConsoTime = (millis() + 600000) | 1;
        bool forcePAPP = false;
        if (meterBASE > 1)
        {
          RLcomm.publishFloat(HUB_ID, SENSOR_ID, CHILD_BASE, meterBASE, 1000, 1);
          forcePAPP = true;
        }
        if (meterHCHC > 1)
        {
          RLcomm.publishFloat(HUB_ID, SENSOR_ID, CHILD_HCHC, meterHCHC, 1000, 1);
          forcePAPP = true;
        }
        if (meterHCHP > 1)
        {
          RLcomm.publishFloat(HUB_ID, SENSOR_ID, CHILD_HCHP, meterHCHP, 1000, 1);
          forcePAPP = true;
        }
        if (forcePAPP)
        {
          RLcomm.publishNum(HUB_ID, SENSOR_ID, CHILD_PAPP, papp);
          forcePAPP = true;
        }
      }
    } else {
      waitForConfig--;
      if (waitForConfig == 0) { // send config
        // publish config
        /*
        rl_config_t cnf;
        memset(&cnf, 0, sizeof(cnf));
        //
        cnf.childID = CHILD_OPTARIF;
        cnf.deviceType = (uint8_t)S_TEXTSENSOR;
        cnf.dataType = (uint8_t)V_TEXT;
        RLcomm.publishConfig(HUB_ID, SENSOR_ID, CHILD_PARAM, cnf);
        //
        cnf.deviceType = (uint8_t)S_NUMERICSENSOR;
        cnf.dataType = (uint8_t)V_NUM;
        if (strcmp(opTarif, "BASE") == 0) {
          cnf.childID = CHILD_BASE;
          RLcomm.publishConfig(HUB_ID, SENSOR_ID, CHILD_PARAM, cnf);
        } else {
          cnf.childID = CHILD_HCHC;
          RLcomm.publishConfig(HUB_ID, SENSOR_ID, CHILD_PARAM, cnf);
          cnf.childID = CHILD_HCHP;
          RLcomm.publishConfig(HUB_ID, SENSOR_ID, CHILD_PARAM, cnf);
        }
        cnf.childID = CHILD_PAPP;
        RLcomm.publishConfig(HUB_ID, SENSOR_ID, CHILD_PARAM, cnf);
*/
      }
    }
    // restore serial input
    Serial1.begin(1200);
    teleinfo.begin();
  }
}
