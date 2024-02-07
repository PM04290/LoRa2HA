/*
  Designed for Pro Micro (ATmega32U4)
  Hardware PCB : MLD01 1.0

*/
#include <TeleInfo.h>

//#define DEBUG_SERIAL

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
#define CHILD_OPTARIF   1
#define CHILD_BASE      2
#define CHILD_HCHC      3
#define CHILD_HCHP      4
#define CHILD_PAPP      5
#define CHILD_IISNT1    6
#define CHILD_IISNT2    7
#define CHILD_IISNT3    8

TeleInfo teleinfo(&Serial1);

#define AVGSIZE 5
long arrPower[AVGSIZE];
byte idxPower = 0;
bool pwrStart = true;
long counter = 0;
uint32_t curTime;
uint32_t Time1s = 0;
uint32_t ConsoTime;
const char oldTarif[10] = {0};

void setup()
{
#ifdef DEBUG_SERIAL
  Serial.begin(115200);
  while (!Serial);
  Serial.println("--- serial debug ON --- ");
#endif

  Serial1.begin(1200);
  teleinfo.begin();

  if (RLcomm.begin(433E6, NULL, NULL, 17))
  {
#ifdef DEBUG_SERIAL
    Serial.println(F("LORA OK"));
#endif
  } else
  {
#ifdef DEBUG_SERIAL
    Serial.println(F("LORA Error"));
#endif
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
    int Iinst = teleinfo.getLongVal("IINST");
    int Iinst1 = 0;
    int Iinst2 = 0;
    int Iinst3 = 0;
    if (Iinst == -1) // si triphasé ?
    {
      Iinst1 = teleinfo.getLongVal("IINST1");
      Iinst2 = teleinfo.getLongVal("IINST2");
      Iinst3 = teleinfo.getLongVal("IINST3");
    }

#ifdef DEBUG_SERIAL
    Serial.println(F("--- tele info available ---"));
    Serial.print(F("Option Tarifaire = "));
    opTarif == NULL ? Serial.println("unknown") : Serial.println(opTarif);
    Serial.print(F("PAPP = "));
    papp < 0 ? Serial.println(F("unknown")) : Serial.println(papp);
    Serial.print("BASE = ");
    meterBASE < 0 ? Serial.println(F("unknown")) : Serial.println(meterBASE);
#endif

    if (strcmp(opTarif, oldTarif) != 0)
    {
      // Send new Tarif if change
      RLcomm.publishText(HUB_ID, SENSOR_ID, CHILD_OPTARIF, opTarif);
      strcpy(oldTarif, opTarif);
      delay(100);
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

#ifdef DEBUG_SERIAL
        Serial.print(F("Avg Power = "));
        Serial.print(avgPower);
        Serial.print(F(" / "));
        Serial.print(papp);
        Serial.print(F(" -> "));
        Serial.println(abs(avgPower - papp));
#endif

        if (abs(avgPower - papp) > 20)
        {
          for (byte i = 0; i < AVGSIZE; i++)
          {
            arrPower[i] = papp;
          }
          idxPower = 0;
          RLcomm.publishNum(HUB_ID, SENSOR_ID, CHILD_PAPP, S_NUMERICSENSOR, papp);
          delay(100);
          if (Iinst >= 0)
          {
            RLcomm.publishNum(HUB_ID, SENSOR_ID, CHILD_IISNT1, S_NUMERICSENSOR, Iinst);
          } else
          {
            RLcomm.publishNum(HUB_ID, SENSOR_ID, CHILD_IISNT1, S_NUMERICSENSOR, Iinst1);
            delay(100);
            RLcomm.publishNum(HUB_ID, SENSOR_ID, CHILD_IISNT2, S_NUMERICSENSOR, Iinst2);
            delay(100);
            RLcomm.publishNum(HUB_ID, SENSOR_ID, CHILD_IISNT3, S_NUMERICSENSOR, Iinst3);
          }
        }
      }
    }

    // Send total counter every 10 minutes in KWh
    if (curTime > ConsoTime)
    {
      ConsoTime = millis() + 600000;
      bool forcePAPP = false;
      if (meterBASE > 1)
      {
        RLcomm.publishFloat(HUB_ID, SENSOR_ID, CHILD_BASE, S_NUMERICSENSOR, meterBASE, 1000, 1);
        forcePAPP = true;
        delay(100);
      }
      if (meterHCHC > 1)
      {
        RLcomm.publishFloat(HUB_ID, SENSOR_ID, CHILD_HCHC, S_NUMERICSENSOR, meterHCHC, 1000, 1);
        forcePAPP = true;
        delay(100);
      }
      if (meterHCHP > 1)
      {
        RLcomm.publishFloat(HUB_ID, SENSOR_ID, CHILD_HCHP, S_NUMERICSENSOR, meterHCHP, 1000, 1);
        forcePAPP = true;
        delay(100);
      }
      if (forcePAPP)
      {
        RLcomm.publishNum(HUB_ID, SENSOR_ID, CHILD_PAPP, S_NUMERICSENSOR, papp);
        forcePAPP = true;
        delay(100);
      }
    }

    // restore serial input
    Serial1.begin(1200);
    teleinfo.begin();
  }
}
