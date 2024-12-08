/*
  Version 1.1
  Designed for ATTiny3216 (internal 10MHz)
  Hardware : MLD03 v2.0

*/
#include <Arduino.h>
#include <RadioLink.h>
#include <Wire.h>
#include <EEPROM.h>

#define DEBUG_LED
#define DEBUG_SERIAL
#define HARDWARE_RTC

#include <MLiotElements.h>

extern RadioLinkClass RLcomm;
extern uint8_t uid;
extern uint8_t hubid;

const char* deviceName = "Piscine";
const char* deviceModel = "MLD03";
const uint32_t LRfreq = 433;
const uint8_t LRrange = 1;
bool LoraOK = false;
bool pairingPending = false;
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

Analog* WaterTemp;
Analog* AirTemp;
Binary* FilterIdle;
Analog* FilterPressure;
Regul* RegFrost;
Schedule* SchRefill;
Schedule* SchFilter;
Input* FilterStartHour; // filter start hour
Input* RefillStartHour; // refill start hour
Input* RefillDays;      // refill skip days
Input* RefillDuration;  // refill duration
Input* FrostL;          // frost low
Input* FrostH; // frost high
Relay* RelayValve;
Relay* RelayPump;

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
  bool needPairing = false;

  DEBUGinit();
  DEBUGln(F("\nStart"));

  LED_INIT;

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
    inputRecord ir;

    EEPROM.put(EEP_UID, uid);
    EEPROM.put(EEP_HUBID, hubid);

    EEPROM.write(EEP_REGULFROST, 0); // manu
    EEPROM.write(EEP_SCHDFILTER, 0); // off
    EEPROM.write(EEP_SCHDREFILL, 0); // off

    ir = { -30, 0, 5, 10};            // min:-3° max:0° sel:-0.5° div:10
    EEPROM.put(EEP_INPUTFROST_L, ir);
    ir = {0, 50, 20, 10};             // min:0° max:5.0° sel:2° div:10
    EEPROM.put(EEP_INPUTFROST_H, ir);

    ir = {3, 12, 8, 1};               // start hour from 3 to 12 (8 default)
    EEPROM.put(EEP_INPUTFILTER_START, ir);

    ir = {0, 20, 12, 1};              // refill start hour from 0 to 20h (12h default)
    EEPROM.put(EEP_INPUTREFILL_START, ir);
    ir = {0, 6, 0, 1};                // refill skip day (0 by defulat : every days)
    EEPROM.put(EEP_INPUTREFILL_DAY, ir);
    ir = {5, 600, 30, 1};             // refill duration from 5 to 600 (30 minutes default)
    EEPROM.put(EEP_INPUTREFILL_DUR, ir);

    needPairing = true;
  }
  EEPROM.get(EEP_HUBID, hubid);

  // range 3 = 500m/1484 ms
  //       2 = 300m/660 ms
  //       1 = 100m/186 ms
  //       0 = 50m/27 ms
  LoraOK = RLcomm.begin(LRfreq * 1E6, onReceive, NULL, 19, LRrange);
  if (LoraOK)
  {
    RLcomm.setWaitOnTx(true);
    DEBUG("LoRa from ");
    DEBUG(uid); DEBUG(" to ");
    DEBUG(hubid); DEBUG(" at ");
    DEBUG(LRfreq); DEBUG("MHz range ");
    DEBUGln(LRrange);
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

  mVCC = 5000; // 5.0 V

  Wire.begin();

  DEBUGln("Init elements :");

  // ! Names must be 13 characters max size

  WaterTemp = (Analog*)ML_addElement(new Analog(PIN_I_1, CHILD_SENSOR1, F("Water temp"), F("°C"), 0.2, 10));
  WaterTemp->setParams(onAnalogNTC, 3950, 1000, 5000);// set Thermistor parameters (Beta, Resistor, Thermistor)

  AirTemp = (Analog*)ML_addElement(new Analog(PIN_I_2, CHILD_SENSOR2, F("Air temp"), F("°C"), 0.2, 10));
  AirTemp->setParams(onAnalogNTC, 3950, 1000, 5000);// set Thermistor parameters (Beta, Resistor, Thermistor)

  FilterIdle = (Binary*)ML_addElement(new Binary(PIN_I_3, CHILD_SENSOR3, F("Idle filter"), stateInverted, nullptr));

  FilterPressure = (Analog*)ML_addElement(new Analog(PIN_I_4, CHILD_SENSOR4, F("Filter press"), F("psi"), 0.2, 10));
  FilterPressure->setParams(onAnalogPressure, 70, 500, 4500);// set sensor range (max bar range, low voltage, high voltage)

  RelayValve = (Relay*)ML_addElement(new Relay(PIN_R_1, CHILD_RELAY1, F("Filling valve")));

  RelayPump = (Relay*)ML_addElement(new Relay(PIN_R_2, CHILD_RELAY2, F("Filter pump")));

  FilterStartHour = (Input*)ML_addElement(new Input(CHILD_INPUTFILTER_START, EEP_INPUTFILTER_START, F("Filter start")));

  RefillStartHour = (Input*)ML_addElement(new Input(CHILD_INPUTREFILL_START, EEP_INPUTREFILL_START, F("Filling start")));
  RefillDays = (Input*)ML_addElement(new Input(CHILD_INPUTREFILL_DAY, EEP_INPUTREFILL_DAY, F("Filling Nday")));
  RefillDuration = (Input*)ML_addElement(new Input(CHILD_INPUTREFILL_DUR, EEP_INPUTREFILL_DUR, F("Filling time")));

  FrostL = (Input*)ML_addElement(new Input(CHILD_INPUTFROST_L, EEP_INPUTFROST_L, F("Frost Low")));
  FrostH = (Input*)ML_addElement(new Input(CHILD_INPUTFROST_H, EEP_INPUTFROST_H, F("Frost High")));

  SchFilter = (Schedule*)ML_addElement(new Schedule(CHILD_SCH_FILTER, EEP_SCHDFILTER, RelayPump, nullptr, FilterStartHour, nullptr, F("Filter mode")));
  SchFilter->setBinaryCondition(FilterIdle, false);
  SchFilter->setConditionMode(conditionAction_t::makePause);

  SchRefill = (Schedule*)ML_addElement(new Schedule(CHILD_SCH_REFILL, EEP_SCHDREFILL, RelayValve, RefillDays, RefillStartHour, RefillDuration, F("Refill mode")));

  RegFrost = (Regul*)ML_addElement(new Regul(CHILD_REGUL_FROST, EEP_REGULFROST, AirTemp, RelayPump, FrostL, FrostH, LOW_hysteresis, F("Frost mode")));

  DEBUGln(needPairing ? "need pairing" : "conf ok");
  if (needPairing && LoraOK)
  {
    pairingPending = true;
    ML_PublishConfigElements(deviceName, deviceModel);
    while (pairingPending)
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
      WaterTemp->Process(); // to have initial Water temp

      // initialize time for Filter (force when reset)
      setFilterDuration();
    }

    // Do every seconds
    ML_ProcessElements();
    ML_SendElements();
  }
  if (oldMinute != rtc.minute())
  {
    oldMinute = rtc.minute();
    DEBUGln(rtc.timestamp());
  }
  if (oldHour != rtc.hour())
  {
    // calculate filter duration every hour when scheduler is inactive
    if (!SchFilter->isActive())
    {
      // number of filtration hours = Water temperature / 2
      setFilterDuration();
    }
    RTC_READ;
    oldHour = rtc.hour();
  }
  processLoRa();
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

void setFilterDuration()
{
  uint16_t duration = WaterTemp->getFloat() * 60 / 2;
  uint16_t maxD = (23 - FilterStartHour->getValue()) * 60;
  duration = min(maxD, duration);
  SchFilter->setDuration(duration); // set duration in minutes
}

float onAnalogPressure(uint16_t adcPress, uint16_t bar_max, uint16_t VminRange, uint16_t VmaxRange)
{
  uint32_t mvADC = (adcPress * (int32_t)mVCC) / 1023L;
  float p = max(0, map(mvADC, VminRange, VmaxRange, 0, bar_max * 10)) / 10.;
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
        if (pairingPending && cp->childID == RL_ID_CONFIG)
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
            pairingPending = false;
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
        if (cp->childID == CHILD_INPUTFROST_L)
        {
          FrostL->setFloat(cp->data.num.value / cp->data.num.divider);
          return;
        }
        if (cp->childID == CHILD_INPUTFROST_H)
        {
          FrostH->setFloat(cp->data.num.value / cp->data.num.divider);
          return;
        }
        if (cp->childID == CHILD_INPUTFILTER_START)
        {
          FilterStartHour->setFloat(cp->data.num.value / cp->data.num.divider);
          setFilterDuration();
          return;
        }
        if (cp->childID == CHILD_INPUTREFILL_START)
        {
          RefillStartHour->setFloat(cp->data.num.value / cp->data.num.divider);
          return;
        }
        if (cp->childID == CHILD_INPUTREFILL_DAY)
        {
          RefillDays->setFloat(cp->data.num.value / cp->data.num.divider);
          return;
        }
        if (cp->childID == CHILD_INPUTREFILL_DUR)
        {
          RefillDuration->setFloat(cp->data.num.value / cp->data.num.divider);
          return;
        }
        if (cp->childID == CHILD_SCH_REFILL)
        {
          char txt[sizeof(rl_packet_t)];
          strncpy(txt, cp->data.text, sizeof(cp->data.text));
          txt[sizeof(cp->data.text)] = 0;
          SchRefill->setText(txt);
          return;
        }
        if (cp->childID == CHILD_SCH_FILTER)
        {
          char txt[sizeof(rl_packet_t)];
          strncpy(txt, cp->data.text, sizeof(cp->data.text));
          txt[sizeof(cp->data.text)] = 0;
          SchFilter->setText(txt);
          return;
        }
        if (cp->childID == CHILD_REGUL_FROST)
        {
          char txt[sizeof(rl_packet_t)];
          strncpy(txt, cp->data.text, sizeof(cp->data.text));
          txt[sizeof(cp->data.text)] = 0;
          RegFrost->setText(txt);
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
