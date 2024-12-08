/*
  Version : 1.0
  PCB : MLE31 V1.0

  PIN config ATTiny3216 (internal oscillator 4MHz)

  This firmware manage gate (by his dry contact)

  IN1 is N.O. dry contact for Ring button
  IN2 is N.O. dry contact to indicate Gate is Closed
  IN3 is N.O. dry contact to indicate Gate is Opened
  OUT1 is the relay switch (N.O.), for temporary pulse on gate dry contact
  It is possible to define the pulse length time (300ms by default).
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

// I/O pin
#define PIN_IN1       PIN_PA5
#define PIN_IN2       PIN_PB4
#define PIN_IN3       PIN_PB5
#define PIN_IN4       PIN_PC2
#define PIN_OUT1      PIN_PC1
#define PIN_OUT2      PIN_PC0
#define PIN_DEBUG_LED PIN_PC2

//--- debug tools ---
#define DEBUG_LED
//#define DEBUG_SERIAL

// EEPROM dictionnary
#define EEP_UID         0
#define EEP_HUBID       (EEP_UID+1)
#define EEP_INPUT_TEMPO (EEP_HUBID+1)

#include <MLiotElements.h>

// LoRa
extern RadioLinkClass RLcomm;
extern uint8_t hubid; // Hub ID
extern uint8_t uid;   // this module ID

const uint32_t LRfreq = 433;
const uint8_t LRrange = 1;
bool LoraOK = false;
bool needPairing = false;

#define CHILD_ID_IN1    1 // sensor on J1
#define CHILD_ID_IN2    2 // sensor on J2
#define CHILD_ID_IN3    3 // sensor on J3
#define CHILD_ID_IN4    4 // sensor on J4 (unused)
#define CHILD_ID_OUT1   5 // relay on OUT1
#define CHILD_ID_OUT2   6 // relay on OUT2 (unused)
#define CHILD_ID_TEMPO  7 // timer
#define CHILD_ID_BUTTON 8 // H.A. button

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

Input* Tempo;
Relay* RelayOut1;

uint32_t curTime;
uint32_t tempoRelay = 0;

void setup()
{
  LED_INIT;

  DEBUGinit();
  DEBUGln("\nStart");

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
  } else
  {
    uid = EEPROM.read(EEP_UID);
  }

  LED_GREEN(255);
  if (uid == 0xFF)
  { // blank EEPROM
    DEBUGln(F("Initialize EEPROM"));
    uid = 40;
    hubid = 0;

    // update
    EEPROM.put(EEP_UID, uid);
    EEPROM.put(EEP_HUBID, hubid);

    inputRecord ir;
    ir = {0, 2000, 300, 1};             // min:0 max:2000 sel:300 div:1
    EEPROM.put(EEP_INPUT_TEMPO, ir);

    needPairing = true;
  }
  EEPROM.get(EEP_HUBID, hubid);

  // Initialisation des Elements
  ML_addElement(new Binary(PIN_IN1, CHILD_ID_IN1, F("Ring"), stateInverted, nullptr));
  ML_addElement(new Binary(PIN_IN2, CHILD_ID_IN2, F("Closed"), stateInverted, nullptr));
  ML_addElement(new Binary(PIN_IN3, CHILD_ID_IN3, F("Opened"), stateInverted, nullptr));

  RelayOut1 = (Relay*)ML_addElement(new Relay(PIN_OUT1, CHILD_ID_OUT1, F("Command")));

  Tempo = (Input*)ML_addElement(new Input(CHILD_ID_TEMPO, EEP_INPUT_TEMPO, F("Pulse time"), F("ms")));

  ML_addElement(new Button(CHILD_ID_BUTTON, F("Open")));

  LoraOK = RLcomm.begin(LRfreq * 1E6, onReceive, NULL, 14, LRrange);
  if (LoraOK)
  {
    RLcomm.setWaitOnTx(true);
    if (needPairing)
    {
      // publish config
      uint8_t h = hubid;
      hubid = RL_ID_BROADCAST;
      ML_PublishConfigElements(F("Gate"), F("MLE31"));
      while (needPairing)
      {
        uint32_t tick = millis() / 3;
        LED_GREEN(abs((tick % 511) - 255));
      }
    }
  } else
  {
#ifdef DEBUG_LED
    // To show LoRa error on H1 LED
    for (byte b = 0; b < 3; b++)
    {
       LED_RED(255); delay(200); LED_OFF; delay(200);
    }
#endif
  }
  LED_OFF;

}

void loop()
{
  curTime = millis();
  // control timer of Relay
  if ((tempoRelay > 0) && (curTime > tempoRelay))
  {
    tempoRelay = 0;
    RelayOut1->setValue(false);
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
        bool activate = pIn->data.num.value > 0;
        RelayOut1->setValue(activate);
        tempoRelay = 0;
        int t = Tempo->getValue();
        // active auto OFF timeout
        if (activate && t)
        {
          tempoRelay = (millis() + t) | 1;
        }
        return;
      }
      if (pIn->childID == CHILD_ID_TEMPO)
      {
        Tempo->setValue(pIn->data.num.value / pIn->data.num.divider);
        return;
      }
      if (pIn->childID == CHILD_ID_BUTTON)
      {
        int t = Tempo->getValue();
        // active auto OFF timeout
        if (t)
        {
          RelayOut1->setValue(1);
          tempoRelay = (millis() + t) | 1;
        }
        return;
      }
    }
  }
}
