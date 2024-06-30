/*
  Version 1.0
  Designed for ATTiny3216 (internal 10MHz)
  Hardware : MLD03 v1.0

*/
#include <Arduino.h>
#include <RadioLink.h>
#include <Wire.h>
#include <EEPROM.h>
#include "src/datetime.h"
#include "src/timespan.h"

#define DEBUG_LED
#define DEBUG_SERIAL
#define HARDWARE_RTC

const char* deviceName = "Piscine";
const char* deviceModel = "MLD03";

RadioLinkClass RLcomm;
const uint32_t LRfreq = 433;
bool LoraOK = false;
bool starting = true;
bool RTCok = true;
uint16_t mVCC = 5000; // 5.0 V

typedef struct __attribute__((packed))
{
  uint8_t version;
  int lqi;
  rl_packets packets;
} packet_version;

#define MAX_PACKET 2
packet_version packetTable[MAX_PACKET];
volatile byte idxReadTable = 0;
volatile byte idxWriteTable = 0;

extern char *__brkval;
int freeMemory() {
  char top;
  return &top - __brkval;
}

typedef struct {
  int16_t minValue;
  int16_t maxValue;
  int16_t selValue;
  int16_t divider;
} thresholdRecord;

typedef struct {
  int8_t mode;
  uint8_t confPeriodic;
  uint8_t confWeekdays;
} scheduleRecord;

uint8_t uid;
uint8_t hubid;

#include "config.hpp"
#include "MLelement.hpp"
#include "src/kalman.hpp"
#include "binary.hpp"
#include "relay.hpp"
#include "analog.hpp"
#include "digital.hpp"
#include "threshold.hpp"
#include "regul.hpp"
#include "schedule.hpp"

Analog* WaterTemp;
Regul* RegRefill;
Regul* RegFrost;
Schedule* SchFilter;
Threshold* setPoint1L; // frost low
Threshold* setPoint1H; // frost high
Threshold* setPoint2L; // level empty
Threshold* setPoint2H; // level full
Threshold* startHourFilter; // filter start hour
EltRelay* RelayValve;
EltRelay* RelayPump;
Binary* BtnIdleFilter;

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
  bool needConfig = false;

  DEBUGinit();
  DEBUGln(F("\nStart"));

  LED_INIT;
  // RST pin form LoRa module is used to active "send config"
  pinMode(RL_DEFAULT_RESET_PIN, INPUT);
  while (analogRead(RL_DEFAULT_RESET_PIN) < 100)
  {
    LED_GREEN(0); delay(300);
    LED_OFF(0); delay(300);
    needConfig = true;
  }

  LED_BLUE(0);
  uid = EEPROM.read(EEP_UID);
  if (uid == 0xFF)
  { // blank EEPROM
    DEBUGln("Initialize EEPROM");
    uid = SENSOR_ID;
    hubid = HUB_UID;
    // update
    EEPROM.put(EEP_UID, uid);
    EEPROM.put(EEP_HUBID, hubid);
    EEPROM.write(EEP_REGUL1, 0);   // regul1 off
    EEPROM.write(EEP_REGUL2, 0);   // regul2 off
    thresholdRecord tr;
    tr = {0, 60, 60, 1};
    EEPROM.put(EEP_THLD1L, tr);
    tr = {61, 100, 80, 1};
    EEPROM.put(EEP_THLD1H, tr);
    tr = { -30, 0, 1, 10};
    EEPROM.put(EEP_THLD2L, tr);
    tr = {1, 10, 10, 10};
    EEPROM.put(EEP_THLD2H, tr);
    tr = {3, 12, 9, 1};
    EEPROM.put(EEP_THLDSTART, tr);
    scheduleRecord sr = {0, 0, 0x7F}; // off, every days, 9:00 start
    EEPROM.put(EEP_SCHED1, sr);
    needConfig = true;
  }
  EEPROM.get(EEP_HUBID, hubid);
  DEBUG(uid); DEBUG(" > ");
  DEBUG(hubid); DEBUG(" / ");
  DEBUGln(LRfreq);

  LoraOK = RLcomm.begin(LRfreq * 1E6, onReceive, NULL, 15, 0);
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

  // ! Names must be 15 characters max size

  WaterTemp = (Analog*)ML_addElement(new Analog(PIN_I_1, CHILD_ID_INPUT1, F("Water temp"), F("°C"), 0.3, 10, onAnalogNTC, nullptr));
  // set Thermistor parameters (Beta, Resistor, Thermistor)
  WaterTemp->setParams(3950, 1000, 5000);

  Analog* Tair = (Analog*)ML_addElement(new Analog(PIN_I_2, CHILD_ID_INPUT2, F("Air temp"), F("°C"), 0.3, 10, onAnalogNTC, nullptr));
  // set Thermistor parameters (Beta, Resistor, Thermistor)
  Tair->setParams(3435, 1000, 10000);

  BtnIdleFilter = (Binary*)ML_addElement(new Binary(PIN_I_3, CHILD_IDLEFILTER, F("Filter idle"), true));

  Analog* Fpress = (Analog*)ML_addElement(new Analog(PIN_I_4, CHILD_ID_INPUT4, F("Filter press"), F("psi"), 2, 1, onAnalogPressure, nullptr));
  // set sensor range (max psi range, low voltage, high voltage)
  Fpress->setParams(80, 500, 4500);

  //  Digital* Wlvl = (Digital*)ML_addElement(new Digital(CHILD_ID_INPUT3, F("Water level"), F(""), 1, 1, onWaterLevel, onTextLevel));
  Digital* Wlvl = (Digital*)ML_addElement(new Digital(CHILD_ID_INPUT3, F("Water level"), F("%"), 1, 1, onWaterLevel, nullptr));

  Element* RelayValve = ML_addElement(new EltRelay(PIN_R_1, CHILD_RELAY1, F("Refill valve")));
  Element* RelayPump = ML_addElement(new EltRelay(PIN_R_2, CHILD_RELAY2, F("Filter pump")));

  setPoint1L = (Threshold*)ML_addElement(new Threshold(CHILD_THRESHOLD1L, EEP_THLD1L, F("Refill Low")));
  setPoint1H = (Threshold*)ML_addElement(new Threshold(CHILD_THRESHOLD1H, EEP_THLD1H, F("Refill High")));
  setPoint2L = (Threshold*)ML_addElement(new Threshold(CHILD_THRESHOLD2L, EEP_THLD2L, F("Frost Low")));
  setPoint2H = (Threshold*)ML_addElement(new Threshold(CHILD_THRESHOLD2H, EEP_THLD2H, F("Frost High")));

  startHourFilter = (Threshold*)ML_addElement(new Threshold(CHILD_THRESHOLD3, EEP_THLDSTART, F("Filter start")));

  SchFilter = (Schedule*)ML_addElement(new Schedule(CHILD_SCHED2, EEP_SCHED1, RelayPump, startHourFilter, F("Filter mode")));
  SchFilter->setCondition(BtnIdleFilter);

  RegRefill = (Regul*)ML_addElement(new Regul(CHILD_REGUL1, EEP_REGUL1, Wlvl, RelayValve, setPoint1L, setPoint1H, LOW_hysteresis, F("Refill mode")));
  //RegRefill->setCondition(RelayPump);
  RegRefill->setCondition(SchFilter);

  RegFrost = (Regul*)ML_addElement(new Regul(CHILD_REGUL2, EEP_REGUL2, Tair, RelayPump, setPoint2L, setPoint2H, LOW_hysteresis, F("Frost mode")));

  DEBUGln(needConfig ? "send conf" : "conf ok");
  if (needConfig && LoraOK)
  {
    ML_PublishConfigElements();
  }
#ifdef HARDWARE_RTC
  Wire.begin();
  RTCok = RTC_READ;
  if (!RTCok)
  {
    LED_RED(2);
    delay(100);
    LED_OFF(2);
    DEBUGln("RTC error");
  } else
  {
    DEBUGln("RTC ok");
    DEBUGln(rtc.isValidDS3231());
  }
#endif

  analogReference(VDD);
  timerInit();
  starting = true;
  LED_OFF(0);
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
      SchFilter->setDuration(WaterTemp->getFloat() * 60 / 2); // set duration in minutes
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
    // calculate filter duration every hour when scheduler is inactive
    if (!SchFilter->isActive())
    {
      // number of filtration hours = Water temperature / 2
      SchFilter->setDuration(WaterTemp->getFloat() * 60 / 2); // set duration in minutes
    }
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

//#define PSI_MAX_RANGE     60  // 60 psi max
//#define VOLT_MIN_RANGE   500  // 0.5v
//#define VOLT_MAX_RANGE  4500  // 4.5v
float onAnalogPressure(uint16_t adcPress, uint16_t psi_max, uint16_t VminRange, uint16_t VmaxRange)
{
  uint32_t mvADC = (adcPress * (int32_t)mVCC) / 1023L;
  float p = map(mvADC, VminRange, VmaxRange, 0, psi_max);
  return p;
}

//#define BETA        3450   // coef beta
//#define RESISTOR    1000   // reference resistor
//#define THERMISTOR  5000   // thermistor value
#define T_REF       298.15 // nominal temperature (Kelvin) (25°)
// coef calculator if unknown : https://www.thinksrs.com/downloads/programs/Therm%20Calc/NTCCalibrator/NTCcalculator.htm
float onAnalogNTC(uint16_t adcNTC, uint16_t beta, uint16_t resistor, uint16_t thermistor)
{
  float celcius = NAN;
  // ! formula with NTC on GND
  float R_NTC = (float)resistor / (1023. / (float)adcNTC - 1.);
  celcius = R_NTC / (float)thermistor;
  celcius = log(celcius);
  celcius /= beta;
  celcius += 1.0 / T_REF;
  celcius = 1.0 / celcius;
  celcius -= 273.15;
  return celcius;
}
/*
  const char* TxtLvl0 = "Empty";
  const char* TxtLvl1 = "1/4";
  const char* TxtLvl2 = "1/2";
  const char* TxtLvl3 = "3/4";
  const char* TxtLvl4 = "Full";
  const char* onTextLevel(int16_t curval)
  {
  if (curval < 12)
    return TxtLvl0;
  if (curval < 37)
    return TxtLvl1;
  if (curval < 62)
    return TxtLvl2;
  if (curval < 87)
    return TxtLvl3;
  return TxtLvl4;
  }
*/
float onWaterLevel(uint16_t level)
{
  for (int i = 9; i >= 0; i--)
  {
    if (level & bit(i))
    {
      return (i + 1) * 10; // 1..10 => 10..100%
    }
  }
  return 0;
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
      if (cp->destinationID == RL_ID_BROADCAST && cp->childID == RL_ID_DATETIME)
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
        if (cp->childID == CHILD_THRESHOLD1L)
        {
          setPoint1L->setFloat(cp->data.num.value / cp->data.num.divider);
          return;
        }
        if (cp->childID == CHILD_THRESHOLD1H)
        {
          setPoint1H->setFloat(cp->data.num.value / cp->data.num.divider);
          return;
        }
        if (cp->childID == CHILD_THRESHOLD2L)
        {
          setPoint2L->setFloat(cp->data.num.value / cp->data.num.divider);
          return;
        }
        if (cp->childID == CHILD_THRESHOLD2H)
        {
          setPoint2H->setFloat(cp->data.num.value / cp->data.num.divider);
          return;
        }
        if (cp->childID == CHILD_THRESHOLD3)
        {
          startHourFilter->setFloat(cp->data.num.value / cp->data.num.divider);
          SchFilter->setDuration(WaterTemp->getFloat() * 60 / 2); // set duration in minutes
          return;
        }
        if (cp->childID == CHILD_REGUL1)
        {
          char txt[sizeof(rl_packet_t)];
          strncpy(txt, cp->data.text, sizeof(cp->data.text));
          txt[sizeof(cp->data.text)] = 0;
          RegRefill->setText(txt);
          return;
        }
        if (cp->childID == CHILD_REGUL2)
        {
          char txt[sizeof(rl_packet_t)];
          strncpy(txt, cp->data.text, sizeof(cp->data.text));
          txt[sizeof(cp->data.text)] = 0;
          RegFrost->setText(txt);
          return;
        }
        if (cp->childID == CHILD_SCHED2)
        {
          char txt[sizeof(rl_packet_t)];
          strncpy(txt, cp->data.text, sizeof(cp->data.text));
          txt[sizeof(cp->data.text)] = 0;
          SchFilter->setText(txt);
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
