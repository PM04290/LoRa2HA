/*
  Version 2.0
  Designed for ATTiny3216 (internal oscillator 1MHz)
  Hardware : MLA30 v2.0 (min)

  ##########################################

*/
// LoRa2HA ATTiny serie-1 general pinout
//
//                 ATtiny3216
//                   _____
//            VDD  1|*    |20 GND
// (nSS)  PA4  0~  2|     |19 16~ PA3 (SCK)
// (IN1)  PA5  1~  3|     |18 15  PA2 (MISO)
// (DIO0) PA6  2   4|     |17 14  PA1 (MOSI)
// (nRST) PA7  3   5|     |16 17  PA0 (UPDI)
// (IN3)  PB5  4   6|     |15 13  PC3 (IN4 only binary)
// (IN2)  PB4  5   7|     |14 12  PC2 (LEDs)
// (RXD)  PB3  6   8|     |13 11~ PC1 (OUT1)
// (TXD)  PB2  7~  9|     |12 10~ PC0 (OUT2)
// (SDA)  PB1  8~ 10|_____|11  9~ PB0 (SCL)

#include <Arduino.h>
#include <EEPROM.h>
#include "avr/sleep.h"

#define DSerial Serial
#define DEBUG_LED
//#define DEBUG_SERIAL

//--- I/O pin ---
#define PIN_IN1       PIN_PA5
#define PIN_IN2       PIN_PB4
#define PIN_IN3       PIN_PB5
#define PIN_IN4       PIN_PC3
#define PIN_OUT1      PIN_PC1
#define PIN_OUT2      PIN_PC0
#define PIN_DEBUG_LED PIN_PC2


// EEPROM dictionnary
#define EEP_UID      0
#define EEP_HUBID    (EEP_UID+1)

#define ML_SX1278
#include <MLiotComm.h>
#include <MLiotElements.h>

// LoRa
extern uint8_t uid;
extern uint8_t hubid;

const uint32_t LRfreq = 433;
const uint8_t  LRrange = 1;
bool LoraOK = false;
bool needPairing = false;
bool pairingPending = false;

// RadioLink IDs
#define CHILD_ID_VBAT   1
#define CHILD_ID_INPUT1 2
#define CHILD_ID_INPUT2 3
#define CHILD_ID_INPUT3 4

//--- V33 Driver (for LoRa module & IO) ---
#define PIN_VCMD      PIN_OUT2
#define DRVON         1
#define DRVOFF        0

//--- debug tools ---

#ifdef DEBUG_LED
#define LED_INIT pinMode(PIN_DEBUG_LED, OUTPUT)
#define LED_ON   digitalWrite(PIN_DEBUG_LED, HIGH)
#define LED_OFF  digitalWrite(PIN_DEBUG_LED, LOW)
#define LED_PWM(x) analogWrite(PIN_DEBUG_LED, x);
#else
#define LED_INIT
#define LED_ON
#define LED_OFF
#define LED_PWM(x)
#endif

//############################
//## Equipment dictionnary ###
//############################


extern uint16_t mVCC;

volatile uint16_t sleepCounter = 0;

ISR(RTC_PIT_vect)
{
  RTC.PITINTFLAGS = RTC_PI_bm; // Clear interrupt flag by writing '1' (required)
  sleepCounter++;
}

// initial setup
void setup()
{
  // disable all pins in first to prevent consumption
  for (uint8_t pin = 0; pin < 8; pin++) {
    (&PORTA.PIN0CTRL)[pin] = PORT_ISC_INPUT_DISABLE_gc;
    (&PORTB.PIN0CTRL)[pin] = PORT_ISC_INPUT_DISABLE_gc;
#if defined(__AVR_ATtinyxy6__)
    (&PORTC.PIN0CTRL)[pin] = PORT_ISC_INPUT_DISABLE_gc;
#endif
  }
  PORTA.PIN2CTRL = 0; // enable input for MISO

  DEBUGinit();
  DEBUGln(F("\nStart"));

  LED_INIT;

  // use nRST pin from LoRa module to active "pairing"
  pinMode(RL_DEFAULT_RESET_PIN, INPUT);
  while (analogRead(RL_DEFAULT_RESET_PIN) < 100)
  {
    LED_ON; delay(millis() > 10000 ? 100 : 300);
    LED_OFF; delay(millis() > 10000 ? 100 : 300);
    needPairing = true;
  }
  PORTA.PIN7CTRL = PORT_ISC_INPUT_DISABLE_gc; // nRST
  if (millis() > 10000)
  { // CFG pressed more than 10s : factory setting
    uid = 0xFF;
  } else
  {
    uid = EEPROM.read(EEP_UID);
  }

  LED_ON;
  delay(50); LED_OFF; delay(50); LED_ON; delay(100); LED_OFF; delay(50); LED_ON;

  if (uid == 0xFF)
  { // blank EEPROM (factory settings)
    DEBUGln(F("Initialize EEPROM"));
    uid = 30;
    hubid = 0;

    // update
    EEPROM.put(EEP_UID, uid);
    EEPROM.put(EEP_HUBID, hubid);

    needPairing = true;
  }
  EEPROM.get(EEP_HUBID, hubid);

  Analog* vcc = (Analog*)deviceManager.addElement(new Analog(PIN_NONE, CHILD_ID_VBAT, F("Vcc"), F("V"), 0.01, 100));
  vcc->setParams(onAnalogVCC, 0, 0, 0);

  deviceManager.addElement(new Binary(PIN_IN1, CHILD_ID_INPUT1, F("Contact 1"), trigFall, nullptr));
  deviceManager.addElement(new Binary(PIN_IN2, CHILD_ID_INPUT2, F("Contact 2"), trigFall, nullptr));
  deviceManager.addElement(new Binary(PIN_IN3, CHILD_ID_INPUT3, F("Contact 3"), trigFall, nullptr));

  // activate interrupt for wake up
  attachInterrupt(digitalPinToInterrupt(PIN_IN1), onPinWakeup, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_IN2), onPinWakeup, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_IN3), onPinWakeup, CHANGE);

  // secondary V33 active
  pinMode(PIN_VCMD, OUTPUT);
  digitalWrite(PIN_VCMD, DRVON);
  delay(5);

  // start LoRa module
  if (startLoRa())
  {
    if (needPairing)
    { // send painring configuration and wait acknowledge
      hubid = RL_ID_BROADCAST;
      deviceManager.publishConfigElements(F("Mailbox"), F("MLA30"));// you can change name of device (replace "Mailbox")
      pairingPending = true;
      while (pairingPending)
      { // wait for receive acknowledge from HUB (see "onReceive")
        uint32_t tick = millis() / 3;
        LED_PWM(abs((tick % 511) - 255));
      }
    } else
    {
      delay(50);
    }
  }

  // prepare for sleep
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();

  // init Timer
  cli();
  while (RTC.STATUS > 0) {}                // Wait for all register to be synchronized
  RTC.CLKSEL      = RTC_CLKSEL_INT32K_gc;  // 32.768kHz Internal Ultra-Low-Power Oscillator (OSCULP32K)
  RTC.PITINTCTRL  = RTC_PI_bm;             // PIT Interrupt: enabled
  RTC.PITCTRLA    = RTC_PERIOD_CYC32768_gc // RTC Clock Cycles 32768, resulting in 32.768kHz/32768 = 1Hz
                    | RTC_PITEN_bm;        // Enable PIT counter
  sei();

  LED_OFF;
}

void loop()
{
  if (LoraOK)
  {
    LED_ON;
    // Walk any sensor in Module to send data
    deviceManager.processElements();
    deviceManager.sendElements();
    LED_OFF;
  }
  powerOff();
}

bool startLoRa()
{
  LoraOK = MLiotComm.begin(LRfreq * 1E6, onReceive, NULL, 14, LRrange);
  if (LoraOK)
  {
    DEBUGln(F("LoRa OK"));
    return true;
  } else
  {
    DEBUGln(F("LoRa Error"));
#ifdef DEBUG_LED
    // To show LoRa error on H1 LED
    for (byte b = 0; b < 3; b++)
    {
      LED_ON; delay(100); LED_OFF; delay(100);
    }
#endif
  }
  return false;
}

void powerOff()
{
  DEBUGln(F("sleep"));

  // Stop LoRa Module
  MLiotComm.sleep();
  MLiotComm.end();

  // power OFF V33
  digitalWrite(PIN_VCMD, DRVOFF);

  // disable ADC
  ADC0.CTRLA &= ~ADC_ENABLE_bm;

  // Sleep for 10 min
  sleepCounter = 0;
  sei();
  do {
    sleep_cpu();
  } while (sleepCounter < 600);

  // power ON V33
  pinMode(RL_DEFAULT_SS_PIN, OUTPUT); // to fix contention issue
  pinMode(RL_DEFAULT_RESET_PIN, OUTPUT);
  delay(2);
  digitalWrite(PIN_VCMD, DRVON);

  // enable ADC
  ADC0.CTRLA |= ADC_ENABLE_bm;

  DEBUGln(F("wake-up"));

  // start LoRa module
  startLoRa();
}

void onReceive(uint8_t len, rl_packet_t* pIn)
{
  // ***************************************
  // ! ! ! don't publish anything here ! ! !
  // ! ! ! and stay short time         ! ! !
  // ***************************************
  if (pIn->destinationID == uid)
  {
    if (pairingPending && pIn->childID == RL_ID_CONFIG)
    {
      rl_conf_t cnfIdx = (rl_conf_t)(pIn->sensordataType & 0x07);
      if (cnfIdx == C_PARAM)
      {
        rl_configParam_t* pcnfp = (rl_configParam_t*)&pIn->data.configs;
        if (pcnfp->childID == 0)
        { // 0 to change UID device
          uid = pcnfp->pInt;
          EEPROM.put(EEP_UID, uid);
        }
      }
      if (cnfIdx == C_END)
      {
        hubid = pIn->senderID;
        EEPROM.put(EEP_HUBID, hubid);
        pairingPending = false;
      }
    }
  }
}

void onPinWakeup()
{
  sleepCounter = 0x1FFF;
}

float onAnalogVCC(uint16_t adcRAW, uint16_t p1, uint16_t p2, uint16_t p3)
{
  analogReference(VDD);
  VREF.CTRLA = VREF_ADC0REFSEL_1V5_gc;
  // there is a settling time between when reference is turned on, and when it becomes valid.
  // since the reference is normally turned on only when it is requested, this virtually guarantees
  // that the first reading will be garbage; subsequent readings taken immediately after will be fine.
  // VREF.CTRLB|=VREF_ADC0REFEN_bm;
  // delay(10);
  uint16_t reading = analogRead(ADC_INTREF);
  reading = analogRead(ADC_INTREF);
  uint32_t intermediate = 1023UL * 1500;
  mVCC = intermediate / reading;
  return mVCC / 1000.;
}
