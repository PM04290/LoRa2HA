/*
  Version 1.0
  Designed for ATTiny3216 (internal 10MHz)
  Hardware : MLD03 v1.0

*/
#include <Arduino.h>
#include <RadioLink.h>
#include <Wire.h>
#include <EEPROM.h>
#include <MLiotElements.h>

#define DEBUG_LED
#define DEBUG_SERIAL
#define HARDWARE_RTC

const char* deviceName = "Arrosage";
const char* deviceModel = "MLD03";

extern RadioLinkClass RLcomm;
extern uint8_t uid;
extern uint8_t hubid;
extern uint16_t mVCC;

const uint32_t LRfreq = 433;
const uint8_t LRrange = 1;
bool LoraOK = false;
bool needPairing = false;
bool starting = true;
bool RTCok = false;

typedef struct __attribute__((packed))
{
  uint8_t version;
  int lqi;
  rl_packets packets;
} packet_version;

#define MAX_PACKET 5
packet_version packetTable[MAX_PACKET];
volatile byte idxReadTable = 0;
volatile byte idxWriteTable = 0;

extern char *__brkval;
int freeMemory() {
  char top;
  return &top - __brkval;
}

#include "config.hpp"

Analog* Moisture1;
Analog* Moisture2;
Schedule* SchZone1;
Schedule* SchZone2;
Input* Z1Day;
Input* Z1Hum;
Input* Z1Hour;
Input* Z1Dur;
Input* Z2Day;
Input* Z2Hum;
Input* Z2Hour;
Input* Z2Dur;
Relay* Relay1;
Relay* Relay2;

volatile bool topSec = false;
#if defined(__AVR_ATtiny1616__) || defined(__AVR_ATtiny3216__)
ISR(RTC_PIT_vect)
{
  topSec = true;
  RTC.PITINTFLAGS = RTC_PI_bm;              /* Clear interrupt flag by writing '1' (required)              */
}
#else
ISR(TIMER1_COMPA_vect) {
  topSec = true;
}
#endif

void timerInit()
{
  cli();
#if defined(__AVR_ATtiny1616__) || defined(__AVR_ATtiny3216__)
  while (RTC.STATUS > 0) {}                /* Wait for all register to be synchronized                    */
  RTC.CTRLA       = RTC_RTCEN_bm;          /* We must enable the RTC if using the PIT! See errata         */
  RTC.CLKSEL      = RTC_CLKSEL_INT32K_gc;  /* 32.768kHz Internal Ultra-Low-Power Oscillator (OSCULP32K)   */
  RTC.PITINTCTRL  = RTC_PI_bm;             /* PIT Interrupt: enabled                                      */
  RTC.PITCTRLA    = RTC_PERIOD_CYC32768_gc /* RTC Clock Cycles 16384, resulting in 32.768kHz/32768 = 1Hz  */
                    | RTC_PITEN_bm;        /* Enable PIT counter: enabled                                 */
#else
  // Remise à zéro du prescaler du timer 1
  TCCR1A = 0;
  TCCR1B = 0;
  // Remise à zéro de la valeur du compteur
  TCNT1  = 0;
  // Positionnement de la valeur du registre de comparaison
  // 16MHz / 512 = 1Hz
  OCR1A = 15624;
  // Activation du comparateur pour le timer 1
  TCCR1B |= (1 << WGM12);
  // Prescaler de 256 sur le timer 1
  //TCCR1B |= (1 << CS12);
  TCCR1B |= (1 << CS12) | (1 << CS10);
  // Activation des interruptions du timer 1
  TIMSK1 |= (1 << OCIE1A);
#endif
  // Activation des interruptions globales (Set External Interrupt)
  sei();
  DEBUGln("Timer ON");
}

void setup()
{
  DEBUGinit();
  DEBUGln(F("\nStart"));

  LED_INIT;
  LED_RED(0);

  uid = EEPROM.read(EEP_UID);

  // RST pin from LoRa module is used to active "send config"
  pinMode(RL_DEFAULT_RESET_PIN, INPUT);
  while (analogRead(RL_DEFAULT_RESET_PIN) < 100)
  {
    LED_BLUE(0); delay(millis() > 10000 ? 100 : 300);
    LED_OFF(0); delay(millis() > 10000 ? 100 : 300);
    needPairing = true;
  }
  if (millis() > 10000)
  { // CFG pressed more than 10s : factory setting
    uid = 0xFF;
  }

  LED_BLUE(0);
  if (uid == 0xFF)
  { // blank EEPROM
    DEBUGln("Initialize EEPROM");
    uid = MODULE_ID;
    hubid = HUB_UID;
    // update
    EEPROM.put(EEP_UID, uid);
    EEPROM.put(EEP_HUBID, hubid);

    inputRecord ir;
    ir = {0, 6, 1, 1};             // min:0 max:6 sel:1 div:1
    EEPROM.put(EEP_INPUT_DAY1, ir);
    ir = {0, 100, 50, 1};          // min:0% max:100 sel:60 div:1
    EEPROM.put(EEP_INPUT_HUM1, ir);
    ir = {0, 23, 20, 1};           // min:0h max:23h sel:20 div:1
    EEPROM.put(EEP_INPUT_HOUR1, ir);
    ir = {10, 600, 60, 1};         // min:10mn max:600mn sel:60mn div:1
    EEPROM.put(EEP_INPUT_DUR1, ir);

    ir = {0, 6, 1, 1};             // min:0 max:6 sel:1 div:1
    EEPROM.put(EEP_INPUT_DAY2, ir);
    ir = {0, 100, 50, 1};          // min:0% max:100 sel:60 div:1
    EEPROM.put(EEP_INPUT_HUM2, ir);
    ir = {0, 23, 21, 1};           // min:0h max:23h sel:21 div:1
    EEPROM.put(EEP_INPUT_HOUR2, ir);
    ir = {10, 600, 60, 1};         // min:10mn max:600mn sel:60mn div:1
    EEPROM.put(EEP_INPUT_DUR2, ir);

    EEPROM.write(EEP_SCHED1, 0);

    EEPROM.write(EEP_SCHED2, 0);

    needPairing = true;
  }
  EEPROM.get(EEP_HUBID, hubid);
  DEBUG(uid); DEBUG(" > ");
  DEBUG(hubid); DEBUG(" / ");
  DEBUGln(LRfreq);

  LoraOK = RLcomm.begin(LRfreq * 1E6, onReceive, NULL, 19, LRrange);
  if (LoraOK)
  {
    RLcomm.setWaitOnTx(true);
    DEBUGln("LoRa OK");
    LED_GREEN(0);
    delay(100);
  } else
  {
    DEBUGln("LoRa Error");
    for (byte b = 0; b < 3; b++)
    {
      LED_RED(1); delay(100); LED_OFF(1); delay(100);
    }
  }

  Wire.begin();

  DEBUGln("Init elements :");

  // ! Names must be 15 characters max size

  Moisture1 = (Analog*)ML_addElement(new Analog(PIN_I_1, CHILD_SENSOR1, F("Z1 Humidity"), F("%"), 2, 1));
  Moisture1->setParams(onAnalogMoisture, 0, 0, 0);

  Moisture2 = (Analog*)ML_addElement(new Analog(PIN_I_2, CHILD_SENSOR2, F("Z2 Humidity"), F("%"), 2, 1));
  Moisture2->setParams(onAnalogMoisture, 0, 0, 0);

  Relay1 = ML_addElement(new Relay(PIN_R_1, CHILD_RELAY1, F("Z1 valve")));

  Relay2 = ML_addElement(new Relay(PIN_R_2, CHILD_RELAY2, F("Z2 valve")));

  Z1Day = (Input*)ML_addElement(new Input(CHILD_INPUT_DAY1, EEP_INPUT_DAY1, F("Z1 skip day")));
  Z1Hour = (Input*)ML_addElement(new Input(CHILD_INPUT_HOUR1, EEP_INPUT_HOUR1, F("Z1 start")));
  Z1Dur = (Input*)ML_addElement(new Input(CHILD_INPUT_DUR1, EEP_INPUT_DUR1, F("Z1 duration")));
  Z1Hum = (Input*)ML_addElement(new Input(CHILD_INPUT_HUM1, EEP_INPUT_HUM1, F("Z1 hum min")));

  Z2Day = (Input*)ML_addElement(new Input(CHILD_INPUT_DAY2, EEP_INPUT_DAY2, F("Z2 skip day")));
  Z2Hour = (Input*)ML_addElement(new Input(CHILD_INPUT_HOUR2, EEP_INPUT_HOUR2, F("Z2 start")));
  Z2Dur = (Input*)ML_addElement(new Input(CHILD_INPUT_DUR2, EEP_INPUT_DUR2, F("Z2 duration")));
  Z2Hum = (Input*)ML_addElement(new Input(CHILD_INPUT_HUM2, EEP_INPUT_HUM2, F("Z2 hum min")));

  SchZone1 = (Schedule*)ML_addElement(new Schedule(CHILD_SCHED1, EEP_SCHED1, Relay1, Z1Day, Z1Hour, Z1Dur, F("Z1 mode")));
  SchZone1->setInputCondition(Z1Hum, Moisture1, false); // run when Hum is below Threshold

  SchZone2 = (Schedule*)ML_addElement(new Schedule(CHILD_SCHED2, EEP_SCHED2, Relay2, Z2Day, Z2Hour, Z2Dur, F("Z2 mode")));
  SchZone2->setInputCondition(Z2Hum, Moisture2, false); // run when Hum is below Threshold

  DEBUGln(needConfig ? "send conf" : "conf ok");
  if (needPairing && LoraOK)
  {
    ML_PublishConfigElements(deviceName, deviceModel);
    while (needPairing)
    {
      processLoRa();
      uint32_t tick = millis() / 3;
      leds.setPixelColor(0, leds.Color(0, abs((tick % 511) - 255), 0)); leds.show();
    }
  }

#ifdef HARDWARE_RTC
  RTCok = RTC_READY;
  if (RTCok)
  {
    DEBUGln("RTC ok");
    RTC_READ;
    DEBUGln(rtc.timestamp());
  } else
  {
    LED_RED(2);
    delay(100);
    LED_OFF(2);
    DEBUGln("RTC error");
  }
#else
  DEBUGln("Without RTC");
#endif

  analogReference(VDD);
  timerInit();
  starting = true;
  LED_OFF(0);
  // request from HUB Datetime
  RLcomm.publishRaw(hubid, uid, RL_ID_DATETIME, nullptr, 0);
  DEBUGln("Setup done");
}

void loop()
{
  static uint32_t oldHour = 99;
  static uint32_t oldMinute = 99;

  if (topSec)
  {
    topSec = false;
    // manage internal RTC
    uint32_t t = rtc.unixtime() + 1;
    rtc = t;
    if ((t & 1) > 0)
    {
      if (RTCok && LoraOK)
      {
        LED_GREEN(0);
      } else
      {
        LED_RED(0);
      }
    } else {
      LED_OFF(0);
    }

    // Do first after reset
    if (starting)
    {
      starting = false;
      // initialize data for automation
      Moisture1->Process(); // to have initial value
      Moisture2->Process();
    }

    // Do every seconds
    ML_ProcessElements();
  }
  if (oldMinute != rtc.minute())
  {
    oldMinute = rtc.minute();
    DEBUGln(rtc.timestamp());
  }
  if (oldHour != rtc.hour())
  {
    RTC_READ;
    oldHour = rtc.hour();
  }
  processLoRa();
  ML_SendElements();
  //
  static uint8_t oldRelay1 = 0xFF;
  if (digitalRead(PIN_R_1) != oldRelay1)
  {
    oldRelay1 = digitalRead(PIN_R_1);
    if (oldRelay1) {
      LED_GREEN(1);
    } else {
      LED_OFF(1);
    }
  }
  static uint8_t oldRelay2 = 0xFF;
  if (digitalRead(PIN_R_2) != oldRelay2)
  {
    oldRelay2 = digitalRead(PIN_R_2);
    if (oldRelay2) {
      LED_GREEN(2);
    } else {
      LED_OFF(2);
    }
  }
}

float onAnalogMoisture(uint16_t adcPress, uint16_t p1, uint16_t p2, uint16_t p3)
{
  uint32_t mvADC = (adcPress * (int32_t)mVCC) / 1023L;
  float p = max(0, map(mvADC, 0, 5000, 0, 100));
  return p;
}

#define T_REF       298.15 // nominal temperature (Kelvin) (25°)
// coef calculator if unknown : https://www.thinksrs.com/downloads/programs/Therm%20Calc/NTCCalibrator/NTCcalculator.htm
float onAnalogNTC(uint16_t adcNTC, uint16_t beta, uint16_t resistor, uint16_t thermistor)
{
  float celcius = NAN;
  // ! formula with NTC on GND
  float r_ntc = (float)resistor / (1023. / (float)adcNTC - 1.);
  celcius = r_ntc / (float)thermistor;
  celcius = log(celcius);
  celcius /= beta;
  celcius += 1.0 / T_REF;
  celcius = 1.0 / celcius;
  celcius -= 273.15;
  return celcius;
}

bool getPacket(packet_version* p)
{
  if (idxReadTable != idxWriteTable)
  {
    *p = packetTable[idxReadTable];
    //
    idxReadTable++;
    if (idxReadTable >= MAX_PACKET)
    {
      idxReadTable = 0;
    }
    return true;
  }
  return false;
}

void processLoRa()
{
  packet_version p;
  if (getPacket(&p))
  {
    rl_packet_t* cp = &p.packets.current;
    DEBUG(cp->senderID); DEBUG(" > "); DEBUG(cp->destinationID); DEBUG(":"); DEBUGln(cp->childID);
    if (cp->senderID == hubid)
    {
      if ((cp->destinationID == RL_ID_BROADCAST || cp->destinationID == uid) && cp->childID == RL_ID_DATETIME)
      {
        uint8_t yy = cp->data.rawByte[0];
        uint8_t mm = max(1, min(cp->data.rawByte[1], 12));
        uint8_t dd = max(1, min(cp->data.rawByte[2], 31));
        uint8_t h = cp->data.rawByte[3] % 24;
        uint8_t m = cp->data.rawByte[4] % 60;
        uint8_t s = cp->data.rawByte[5] % 60;
        rtc = DateTime(yy, mm, dd, h, m, s);
        RTC_WRITE;
        return;
      }
      if (cp->destinationID == uid)
      {
        if (needPairing && cp->childID == RL_ID_CONFIG)
        {
          rl_conf_t cnfIdx = (rl_conf_t)(cp->sensordataType & 0x07);
          if (cnfIdx == C_PARAM)
          {
            rl_configParam_t* pcnfp = (rl_configParam_t*)&cp->data.configs;
            if (pcnfp->childID == 0)
            { // 0 to change UID device
              uid = pcnfp->pInt;
              EEPROM.put(EEP_UID, uid);
            }
          }
          if (cnfIdx == C_END)
          {
            needPairing = false;
          }
        }
        if (cp->childID == CHILD_RELAY1)
        {
          Element* e = ML_getElementByID(cp->childID);
          if (e) e->setBool(cp->data.num.value);
          return;
        }
        if (cp->childID == CHILD_RELAY2)
        {
          Element* e = ML_getElementByID(cp->childID);
          if (e) e->setBool(cp->data.num.value);
          return;
        }
        if (cp->childID == CHILD_INPUT_DAY1)
        {
          Z1Day->setFloat(cp->data.num.value / cp->data.num.divider);
          return;
        }
        if (cp->childID == CHILD_INPUT_HUM1)
        {
          Z1Hum->setFloat(cp->data.num.value / cp->data.num.divider);
          return;
        }
        if (cp->childID == CHILD_INPUT_HOUR1)
        {
          Z1Hour->setFloat(cp->data.num.value / cp->data.num.divider);
          return;
        }
        if (cp->childID == CHILD_INPUT_DUR1)
        {
          Z1Dur->setFloat(cp->data.num.value / cp->data.num.divider);
          return;
        }
        if (cp->childID == CHILD_INPUT_DAY2)
        {
          Z2Day->setFloat(cp->data.num.value / cp->data.num.divider);
          return;
        }
        if (cp->childID == CHILD_INPUT_HUM2)
        {
          Z2Hum->setFloat(cp->data.num.value / cp->data.num.divider);
          return;
        }
        if (cp->childID == CHILD_INPUT_HOUR2)
        {
          Z2Hour->setFloat(cp->data.num.value / cp->data.num.divider);
          return;
        }
        if (cp->childID == CHILD_INPUT_DUR2)
        {
          Z2Dur->setFloat(cp->data.num.value / cp->data.num.divider);
          return;
        }
        if (cp->childID == CHILD_SCHED1)
        {
          char txt[sizeof(rl_packet_t)];
          strncpy(txt, cp->data.text, sizeof(cp->data.text));
          txt[sizeof(cp->data.text)] = 0;
          SchZone1->setText(txt);
          return;
        }
        if (cp->childID == CHILD_SCHED2)
        {
          char txt[sizeof(rl_packet_t)];
          strncpy(txt, cp->data.text, sizeof(cp->data.text));
          txt[sizeof(cp->data.text)] = 0;
          SchZone2->setText(txt);
          return;
        }
      }
    }
  }
}
void onReceive(uint8_t len, rl_packet_t* p)
{
  noInterrupts();
  memcpy(&packetTable[idxWriteTable].packets.current, p, len);
  packetTable[idxWriteTable].version = 1;
  packetTable[idxWriteTable].lqi = RLcomm.lqi();
  if (len == RL_PACKETV1_SIZE) packetTable[idxWriteTable].version = 1;
  idxWriteTable++;
  if (idxWriteTable >= MAX_PACKET)
  {
    idxWriteTable = 0;
  }
  if (idxWriteTable == idxReadTable) // overload
  {
    idxReadTable++;
    if (idxReadTable >= MAX_PACKET)
    {
      idxReadTable = 0;
    }
  }
  interrupts();
}
