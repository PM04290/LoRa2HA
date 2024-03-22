/*
  Version 1.0
  PIN config ATTiny84 (internal oscillator 1 or 8MHz)

  ##########################################
  opt1 : ANA0 or D0
  opt2 : ANA1 or D1
*/
#include <Arduino.h>
#include <RadioLink.h>

// ATMEL ATTINY84 / ARDUINO
//
//                          +-\/-+
//                    VCC  1|    |14  GND
//       DEBUG (D10)  PB0  2|    |13  PA0  (D 0)  IO0
//             (D 9)  PB1  3|    |12  PA1  (D 1)  IO1
//              RST   PB3  4|    |11  PA2  (D 2)      NRST
//  DIO0 (INT0)(D 8)  PB2  5|    |10  PA3  (D 3)      NSS
//       RELAY (D 7)  PA7  6|    |9   PA4  (D 4)      SCK
//  MISO       (D 6)  PA6  7|    |8   PA5  (D 5)      MOSI
//                          +----+

//--- Tiny84 LoRa module pin ---
// NRST : 2
// NSS  : 3
// SCK  : 4
// MOSI : 5
// MISO : 6
// DIO0 : 8

// I/O pin
#define PIN_IO_0        0
#define PIN_IO_1        1
#define PIN_RELAY       7

//--- debug tools ---
//#define DEBUG_LED

#ifdef DEBUG_LED
#define PIN_DEBUG_LED  10
#define LED_INIT pinMode(PIN_DEBUG_LED, OUTPUT)
#define LED_ON   digitalWrite(PIN_DEBUG_LED, HIGH)
#define LED_OFF  digitalWrite(PIN_DEBUG_LED, LOW)
#else
#define LED_INIT
#define LED_ON
#define LED_OFF
#endif

//#define DEBUG_SERIAL

#ifdef DEBUG_SERIAL
#include <SendOnlySoftwareSerial.h>
SendOnlySoftwareSerial Serial(10);  // Tx pin
#define DEBUGinit() Serial.begin(2400)
#define DEBUG(x) Serial.print(x)
#define DEBUGln(x) Serial.println(x)
#else
#define DEBUGinit()
#define DEBUG(x)
#define DEBUGln(x)
#endif

// RadioLink
#define HUB_ID          0 // HUB ID
#define SENSOR_ID      20 // module ID
#define CHILD_PARAM  0xFF // config ID
#define CHILD_ID_IO0    1 // sensor on IO0
#define CHILD_ID_IO1    2 // sensor on IO1
#define CHILD_RELAY_ID  3 // relay

RadioLinkClass RLcomm;

uint32_t curTime, oldTime = 0;
int nbSec = 0;

uint8_t oldRelay = 0xFF; // FF to send state relay at start
uint32_t tempoRelay = 0;

bool IO0active = false;
uint32_t oldIO0Time = 0;

bool IO1active = false;
uint32_t oldIO1Time = 0;

bool LoraOK = false;
const uint32_t LRfreq = 433;

void setup()
{
  LED_INIT;
  LED_ON;
  DEBUGinit();

  // Initialisation des pins
  pinMode(PIN_IO_0, INPUT_PULLUP);
  pinMode(PIN_IO_1, INPUT_PULLUP);

  pinMode(PIN_RELAY, OUTPUT);
  digitalWrite(PIN_RELAY, LOW);

  LoraOK = RLcomm.begin(LRfreq * 1E6, onReceive, NULL, 17);
  if (LoraOK)
  {
    RLcomm.setWaitOnTx(true);
    DEBUGln("LoRa OK");
  } else
  {
    DEBUGln("LoRa Error");
#ifdef DEBUG_LED
    // To show LoRa error on H1 LED
    for (byte b = 0; b < 3; b++)
    {
      LED_OFF; delay(100); LED_ON; delay(100);
    }
#endif
  }

  // publish config
  rl_config_t cnf;
  memset(&cnf, 0, sizeof(cnf));
  //
  cnf.childID = CHILD_ID_IO0;
  cnf.deviceType = (uint8_t)S_BINARYSENSOR;
  cnf.dataType = (uint8_t)V_BOOL;
  RLcomm.publishConfig(HUB_ID, SENSOR_ID, CHILD_PARAM, cnf);
  //
  cnf.childID = CHILD_ID_IO1;
  cnf.deviceType = (uint8_t)S_NUMERICSENSOR;
  cnf.dataType = (uint8_t)V_NUM;
  RLcomm.publishConfig(HUB_ID, SENSOR_ID, CHILD_PARAM, cnf);
  //
  cnf.childID = CHILD_RELAY_ID;
  cnf.deviceType = (uint8_t)S_SWITCH;
  cnf.dataType = (uint8_t)V_BOOL;
  RLcomm.publishConfig(HUB_ID, SENSOR_ID, CHILD_PARAM, cnf);

  LED_OFF;
}

void loop()
{
  // receive activation switch
  curTime = millis();
  if ((curTime > oldTime + 1000) || (curTime < oldTime))
  {
    oldTime = curTime;
    nbSec++;
  }
  if (nbSec >= 30)
  {
    nbSec = 0;
    // Do something every 30sec
  }

  // control timer of Relay
  if ((tempoRelay > 0) && (curTime > tempoRelay))
  {
    tempoRelay = 0;
    digitalWrite(PIN_RELAY, LOW);
  }

  // send relay State to H.A. if change
  uint8_t relay = digitalRead(PIN_RELAY);
  if (oldRelay != relay)
  {
    oldRelay = relay;
    RLcomm.publishSwitch(HUB_ID, SENSOR_ID, CHILD_RELAY_ID, relay);
  }

  bool IODown;
  // Digital input on IO_0 (simple contact information)
  IODown = (digitalRead(PIN_IO_0) == LOW);
  if (IODown)
  {
    if (oldIO0Time > 0)
    {
      // debounce
      if ((IO0active == false) && (curTime > oldIO0Time + 50))
      {
        IO0active = true;
        // Send IO state to H.A.
        RLcomm.publishBool(HUB_ID, SENSOR_ID, CHILD_ID_IO0, IO0active);
        delay(100);
      }
    } else
    {
      // antirebond
      oldIO0Time = curTime | 1;
    }
  } else {
    if (IO0active)
    {
      IO0active = false;
      // Send IO state to H.A.
      RLcomm.publishBool(HUB_ID, SENSOR_ID, CHILD_ID_IO0, IO0active);
      delay(100);
    }
    oldIO0Time = 0;
  }

  // Digital input on IO_1 (button with pressed time code)
  IODown = (digitalRead(PIN_IO_1) == LOW);
  if (IODown)
  {
    if (oldIO1Time > 0)
    {
      // debounce
      if ((IO1active == false) && (curTime > oldIO1Time + 50))
      {
        IO1active = true;
      }
    } else
    {
      // antirebond
      oldIO1Time = curTime | 1;
    }
  } else {
    if (IO1active)
    {
      // code corresponding to pressed time
      // 1.5s  < 3s < +
      //   1     2    3
      int code = 1;
      if ((curTime - oldIO1Time) > 1500)
      {
        code = 2;
      }
      if ((curTime - oldIO1Time) > 3000)
      {
        code = 3;
      }
      // Send timing code to H.A.
      RLcomm.publishNum(HUB_ID, SENSOR_ID, CHILD_ID_IO1, code);
      delay(300);
      RLcomm.publishNum(HUB_ID, SENSOR_ID, CHILD_ID_IO1, 0);
    }
    oldIO1Time = 0;
    IO1active = false;
  }
}

void onReceive(uint8_t len, rl_packet_t* pIn)
{
  // ***************************************
  // ! ! ! don't publish anything here ! ! !
  // ***************************************
  if (pIn->destinationID == SENSOR_ID && pIn->senderID == HUB_ID)
  {
    if (pIn->childID == CHILD_PARAM)
    {
      // soon
      return;
    }
    if (pIn->childID == CHILD_RELAY_ID)
    {
      // receive relay active duration (in ms) or 1
      int duree = pIn->data.num.value;
      if (duree < 300)
      {
        duree = 300;
      } else if (duree > 1000)
      {
        duree = 1000;
      }
      DEBUG("Relay ");
      DEBUGln(duree);

      digitalWrite(PIN_RELAY, HIGH);
      // active auto OFF timeout
      tempoRelay = (curTime + duree) | 1;
      return;
    }
  }
}
