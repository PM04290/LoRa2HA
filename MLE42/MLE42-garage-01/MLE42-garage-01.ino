/*
  Version 2.0
  PCB : MLD42 V1.0 + T3216

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
#include <EEPROM.h>

//--- debug tools ---
#define DEBUG_LED
#define DSerial Serial
//#define DEBUG_SERIAL

#define ML_SX1278
#include <MLiotComm.h>
#include <MLiotElements.h>

// EEPROM mapping (do not change order)
#define EEP_UID               0
#define EEP_HUBID             (EEP_UID+1)

// I/O pin
#define PIN_IN1       PIN_PA5
#define PIN_IN2       PIN_PB4
#define PIN_IN3       PIN_PB5
#define PIN_IN4       PIN_PC2
#define PIN_OUT1      PIN_PC1
#define PIN_OUT2      PIN_PC0
#define PIN_DEBUG_LED PIN_PC2

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
extern uint8_t hubid; // Hub ID
extern uint8_t uid;   // this module ID

#define CHILD_ID_IN1    1 // sensor on J1
#define CHILD_ID_IN2    2 // sensor on J2
#define CHILD_ID_IN3    3 // unused
#define CHILD_ID_IN4    4 // unused
#define CHILD_ID_OUT1   5 // relay on OUT1 (Motor Open)
#define CHILD_ID_OUT2   6 // relay on OUT2 (Motor Close)
#define CHILD_ID_TOGGLE 7 // move motor opposite

bool LoraOK = false;
const uint32_t LRfreq = 433;
const uint8_t LRrange = 1; // 3 = 500m/1484 ms, 2 = 200m/660 ms, 1 = 100m/186 ms,  0 = 30m/27 ms
bool needPairing = false;

// MLiotElements

Binary* ContactOpened;
Binary* ContactClosed;
Relay* RelayOpen;
Relay* RelayClose;
Binary* Detection;
Button* Toggle;

void setup()
{

  LED_INIT;

  DEBUGinit();
  DEBUGln("\nStart");

  uid = EEPROM.read(EEP_UID);

  // RST pin from LoRa module is used to active "send config"
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
    uid = 50;
    hubid = 0;
    // update
    EEPROM.put(EEP_UID, uid);
    EEPROM.put(EEP_HUBID, hubid);

    needPairing = true;
  }
  EEPROM.get(EEP_HUBID, hubid);

  // Initialisation des Elements
  ContactOpened = (Binary*)deviceManager.addElement(new Binary(PIN_IN1, CHILD_ID_IN1, F("Opened"), stateInverted, nullptr));
  ContactClosed = (Binary*)deviceManager.addElement(new Binary(PIN_IN2, CHILD_ID_IN2, F("Closed"), stateInverted, nullptr));
  Detection = (Binary*)deviceManager.addElement(new Binary(PIN_IN3, CHILD_ID_IN3, F("Detection"), stateNormal, nullptr));

  RelayOpen = (Relay*)deviceManager.addElement(new Relay(PIN_OUT1, CHILD_ID_OUT1, F("Open")));
  RelayClose = (Relay*)deviceManager.addElement(new Relay(PIN_OUT2, CHILD_ID_OUT2, F("Close")));

  Toggle = (Button*)deviceManager.addElement(new Button(CHILD_ID_TOGGLE, F("Toggle")));

  LoraOK = MLiotComm.begin(LRfreq * 1E6, onReceive, NULL, 14, LRrange);
  if (LoraOK)
  {
    if (needPairing)
    {
      // publish config
      hubid = RL_ID_BROADCAST;
      deviceManager.publishConfigElements(F("Garage"), F("MLE42"));
      while (needPairing)
      {
        uint32_t tick = millis() / 3;
        LED_GREEN(abs((tick % 511) - 255));
      }
    }
  } else
  {
#ifdef DEBUG_LED
    LED_RED(255);
    // To show LoRa error on H1 LED
    for (byte b = 0; b < 3; b++)
    {
      delay(100); LED_OFF; delay(100); LED_RED(255);
    }
#endif
  }
  delay(100);
  LED_OFF;
  RelayOpen->setValue(0);
  RelayClose->setValue(0);
}

void loop()
{
  deviceManager.processElements();
  // control opened contact
  if (RelayOpen->getBool() == true && ContactOpened->getBool() == true)
  {
    RelayOpen->setValue(0);
  }
  // control closed contact
  if (RelayClose->getBool() == true && ContactClosed->getBool() == true)
  {
    RelayClose->setValue(0);
  }
  deviceManager.sendElements();
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
        return;
      }
      if (cnfIdx == C_END)
      {
        hubid = pIn->senderID;
        EEPROM.put(EEP_HUBID, hubid);
        needPairing = false;
        return;
      }
    }
    if (pIn->senderID == hubid)
    {
      if (pIn->childID == CHILD_ID_OUT1)
      { // Open door
        bool needOpen = pIn->data.num.value > 0;
        if (needOpen && ContactOpened->getBool() == false)
        {
          RelayOpen->setValue(1);
        }
        if (!needOpen)
        {
          RelayOpen->setValue(0);
        }
        // active auto OFF timeout
        return;
      }
      if (pIn->childID == CHILD_ID_OUT2)
      { // Close door
        bool needClose = pIn->data.num.value > 0;
        if (needClose && ContactClosed->getBool() == false)
        {
          RelayClose->setValue(1);
        }
        if (!needClose)
        {
          RelayClose->setValue(0);
        }
        return;
      }
      if (pIn->childID == CHILD_ID_TOGGLE)
      { // Move door opposite
        if (ContactClosed->getBool() == true)
        {
          RelayClose->setValue(0);
          RelayOpen->setValue(1);
        } else {
          RelayOpen->setValue(0);
          RelayClose->setValue(1);
        }
      }
    }
  }
}
