/*
  Version : 1.0
  PCB : MLE42 V1.0 + T3216

  ATTiny3216 (internal oscillator 4MHz)

*/
//                 ATtiny3216
//                   _____
//            VDD  1|*    |20 GND
// (nSS)  PA4  0~  2|     |19 16~ PA3 (SCK)
// (IN1)  PA5  1~  3|     |18 15  PA2 (MISO)
// (DIO0) PA6  2   4|     |17 14  PA1 (MOSI)
// (nRST) PA7  3   5|     |16 17  PA0 (UPDI)
// (IN3)  PB5  4   6|     |15 13  PC3 (IN4)
// (IN2)  PB4  5   7|     |14 12  PC2 (LEDs)
// (RXD)  PB3  6   8|     |13 11~ PC1 (OUT1)
// (TXD)  PB2  7~  9|     |12 10~ PC0 (OUT2)
// (SDA)  PB1  8~ 10|_____|11  9~ PB0 (SCL)
#include <Arduino.h>
#include <RadioLink.h>
#include <EEPROM.h>

// EEPROM mapping (do not change order)
#define EEP_UID               0
#define EEP_HUBID             (EEP_UID+1)
#define EEP_INPUT_TEMPO       (EEP_HUBID+1)

// I/O pin
#define PIN_IN1       PIN_PA5
#define PIN_IN2       PIN_PB4
#define PIN_IN3       PIN_PB5
#define PIN_IN4       PIN_PC2
#define PIN_OUT1      PIN_PC1
#define PIN_OUT2      PIN_PC0
#define PIN_DEBUG_LED PIN_PC2

// uncomment line below if have RTC DS3231Pi
//#define HARDWARE_RTC

//--- debug tools ---
#define DEBUG_LED
//#define DEBUG_SERIAL

#ifdef DEBUG_LED
#include <tinyNeoPixel_Static.h>
#define NUMLEDS 3
byte pixels[NUMLEDS * 3];
tinyNeoPixel leds = tinyNeoPixel(NUMLEDS, PIN_DEBUG_LED, NEO_GRB, pixels);
#define LED_INIT {pinMode(PIN_DEBUG_LED, OUTPUT); leds.setBrightness(10);}
#define LED_RED(x) {leds.setPixelColor(0, leds.Color(x, 0, 0)); leds.show();}
#define LED_GREEN(x) {leds.setPixelColor(0, leds.Color(0, x, 0)); leds.show();}
#define LED_BLUE(x) {leds.setPixelColor(0, leds.Color(0, 0, x)); leds.show();}
#define LED_OFF {leds.setPixelColor(0, leds.Color(0, 0, 0)); leds.show();}
#else
#define LED_INIT
#define LED_RED(x)
#define LED_GREEN(x)
#define LED_BLUE(x)
#define LED_OFF
#endif

// RadioLink
extern RadioLinkClass RLcomm;
extern uint8_t hubid; // Hub ID
extern uint8_t uid;   // this module ID
bool LoraOK = false;
const uint32_t LRfreq = 433;
const uint8_t LRrange = 1; // 3 = 500m/1484 ms, 2 = 200m/660 ms, 1 = 100m/186 ms,  0 = 30m/27 ms
bool needPairing = false;

#define CHILD_ID_IN1    1 // sensor on J1
#define CHILD_ID_IN2    2 // sensor on J2
#define CHILD_ID_IN3    3 // reserved
#define CHILD_ID_IN4    4 // reserved
#define CHILD_ID_OUT1   5 // cover (motor)
#define CHILD_ID_OUT2   6 // reserved
#define CHILD_ID_TEMPO  7 // timer

// MLiotElements
#include <MLiotElements.h>

Binary* ContactOpened;
Binary* ContactClosed;
Motor* Cover;
Input* Tempo;

bool RTCok = false;

volatile bool topSec = false;
ISR(RTC_PIT_vect)
{
  topSec = true;
  RTC.PITINTFLAGS = RTC_PI_bm;              /* Clear interrupt flag by writing '1' (required)              */
}

void timerInit()
{
  cli();
  while (RTC.STATUS > 0) {}                /* Wait for all register to be synchronized                    */
  RTC.CTRLA       = RTC_RTCEN_bm;          /* We must enable the RTC if using the PIT! See errata         */
  RTC.CLKSEL      = RTC_CLKSEL_INT32K_gc;  /* 32.768kHz Internal Ultra-Low-Power Oscillator (OSCULP32K)   */
  RTC.PITINTCTRL  = RTC_PI_bm;             /* PIT Interrupt: enabled                                      */
  RTC.PITCTRLA    = RTC_PERIOD_CYC32768_gc /* RTC Clock Cycles 16384, resulting in 32.768kHz/32768 = 1Hz  */
                    | RTC_PITEN_bm;        /* Enable PIT counter: enabled                                 */
  // Activation des interruptions globales (Set External Interrupt)
  sei();
  DEBUGln("Timer ON");
}

void setup()
{
  LED_INIT;

  DEBUGinit();
  DEBUGln("\nStart");

  uid = EEPROM.read(EEP_UID);

  // RST pin from LoRa module is used to active "pairing"
  pinMode(RL_DEFAULT_RESET_PIN, INPUT);
  while (analogRead(RL_DEFAULT_RESET_PIN) < 100)
  {
    LED_BLUE(255); delay(millis() > 10000 ? 100 : 300);
    LED_OFF; delay(millis() > 10000 ? 100 : 300);
    needPairing = true;
  }
  if (millis() > 10000)
  { // CFG pressed more than 10s : factory setting
    uid = 0xFF;
  }
  LED_GREEN(255);

  if (uid == 0xFF)
  { // blank EEPROM
    DEBUGln("Initialize EEPROM");
    uid = 51;
    hubid = 0;
    // update
    EEPROM.put(EEP_UID, uid);
    EEPROM.put(EEP_HUBID, hubid);

    inputRecord ir;
    // opening time in second
    ir = {1, 60, 10, 1};             // min:1 max:60 sel:10 div:1
    EEPROM.put(EEP_INPUT_TEMPO, ir);

    needPairing = true;
  }
  EEPROM.get(EEP_HUBID, hubid);

  // Initialisation des Elements
  ContactOpened = (Binary*)ML_addElement(new Binary(PIN_IN1, CHILD_ID_IN1, F("Opened"), stateInverted, nullptr));
  ContactClosed = (Binary*)ML_addElement(new Binary(PIN_IN2, CHILD_ID_IN2, F("Closed"), stateInverted, nullptr));

  Tempo = (Input*)ML_addElement(new Input(CHILD_ID_TEMPO, EEP_INPUT_TEMPO, F("Move time"), F("s")));

  Cover = (Motor*)ML_addElement(new Motor(PIN_OUT1, PIN_OUT2, CHILD_ID_OUT1, F("Door")));
  Cover->setTimer(Tempo->getValue());
  Cover->setLimits(ContactOpened, ContactClosed);

  LoraOK = RLcomm.begin(LRfreq * 1E6, onReceive, NULL, 14, LRrange);
  if (LoraOK)
  {
    RLcomm.setWaitOnTx(true);
    if (needPairing)
    {
      // publish config
      uint8_t h = hubid;
      hubid = RL_ID_BROADCAST;
      ML_PublishConfigElements(F("HenHouse"), F("MLE42"));
      while (needPairing)
      {
        uint32_t tick = millis() / 3;
        LED_BLUE(abs((tick % 511) - 255));
      }
    }
  } else
  {
#ifdef DEBUG_LED
    // To show LoRa error on H1 LED
    LED_RED(255);
    for (byte b = 0; b < 3; b++)
    {
      delay(100); LED_OFF; delay(100); LED_RED(255);
    }
#endif
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
    LED_RED(255);
    delay(100);
    LED_OFF;
    DEBUGln("RTC error");
  }
#else
  DEBUGln("Without RTC");
#endif
  analogReference(VDD);
  timerInit();

  LED_OFF;
}

void loop()
{
  if (topSec)
  {
    topSec = false;
    // manage internal RTC
    uint32_t t = rtc.unixtime() + 1;
    rtc = t;
    if ((t & 1) > 0)
    {
      LED_GREEN(255);
    } else {
      LED_OFF;
    }
  }
  ML_ProcessElements();
  ML_SendElements();
}

void onReceive(uint8_t len, rl_packet_t* pIn)
{
  // ***************************************
  // ! ! ! don't publish anything here ! ! !
  // ! ! ! and stay short time         ! ! !
  // ***************************************
  if (pIn->destinationID == uid)
  {
    if (needPairing && pIn->childID == RL_ID_CONFIG)
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
        needPairing = false;
      }
    }
    if (pIn->senderID == hubid)
    {
      if (pIn->childID == CHILD_ID_OUT1)
      { // door
        char txt[sizeof(rl_packet_t)];
        strncpy(txt, pIn->data.text, sizeof(pIn->data.text));
        txt[sizeof(pIn->data.text)] = 0;
        Cover->setText(txt);
        return;
      }
      if (pIn->childID == CHILD_ID_TEMPO)
      {
        Tempo->setValue(pIn->data.num.value);
        Cover->setTimer(Tempo->getValue());
        return;
      }
    }
  }
}
