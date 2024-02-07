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
//                           +-\/-+
//                     VCC  1|    |14  GND
//             (D 10)  PB0  2|    |13  AREF (D  0)
//             (D  9)  PB1  3|    |12  PA1  (D  1)
//                     PB3  4|    |11  PA2  (D  2)
//  PWM  INT0  (D  8)  PB2  5|    |10  PA3  (D  3)
//  PWM        (D  7)  PA7  6|    |9   PA4  (D  4)
//  PWM        (D  6)  PA6  7|    |8   PA5  (D  5)        PWM
//                           +----+

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

// tools
#define PIN_LED        10

// RadioLink
#define HUB_ID          0 // HUB ID
#define SENSOR_ID      20 // module ID
#define CHILD_PARAM     0 // future use
#define CHILD_ID_IO0    1 // sensor on IO0
#define CHILD_ID_IO1    2 // sensor on IO1
#define CHILD_RELAY_ID  3 // relay

RadioLinkClass RLcomm;

uint32_t curTime, oldTime = 0;
int nbSec = 0;

int relay = 0;
uint32_t tempoRelay = 0;

bool IO0active = false;
uint32_t oldIO0Time = 0;

bool IO1active = false;
uint32_t oldIO1Time = 0;

bool LoraOK = false;

void setup()
{
  // Initialisation des pins
  pinMode(PIN_IO_0, INPUT_PULLUP);
  pinMode(PIN_IO_1, INPUT_PULLUP);

  pinMode(PIN_RELAY, OUTPUT);
  digitalWrite(PIN_RELAY, LOW);

  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, HIGH);

  LoraOK = RLcomm.begin(435E6, onReceive, NULL, 15);
  if (LoraOK)
  {
    RLcomm.setWaitOnTx(true);
    delay(100);
  } else
  {
    for (byte b = 0; b < 3; b++)
    {
      delay(100);
      digitalWrite(PIN_LED, LOW);
      delay(100);
      digitalWrite(PIN_LED, HIGH);
    }
  }
  digitalWrite(PIN_LED, LOW);
}

void loop()
{
  // receive activation switch
  curTime = millis();
  if ((curTime > oldTime + 1000) || (curTime < oldTime))
  {
    oldTime = curTime;

    // comment to reduce consumption
    //digitalWrite(PIN_LED, !digitalRead(PIN_LED));

    nbSec++;
  }
  if (nbSec >= 30)
  {
    nbSec = 0;
    // Do something every 30sec
  }

  if ((tempoRelay > 0) && (curTime > tempoRelay))
  {
    tempoRelay = 0;
    digitalWrite(PIN_RELAY, LOW);
    // send OFF state to H.A.
    RLcomm.publishNum(0, SENSOR_ID, CHILD_RELAY_ID, S_SWITCH, 0);
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
        RLcomm.publishNum(0, SENSOR_ID, CHILD_ID_IO0, S_BINARYSENSOR, IO0active);
        digitalWrite(PIN_LED, HIGH);
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
      RLcomm.publishNum(0, SENSOR_ID, CHILD_ID_IO0, S_BINARYSENSOR, IO0active);
      digitalWrite(PIN_LED, LOW);
      delay(100);
    }
    oldIO0Time = 0;
  }

  // Digital input on IO_0 (button with pressed time code
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
      RLcomm.publishNum(0, SENSOR_ID, CHILD_ID_IO1, S_NUMERICSENSOR, code);
    }
    oldIO1Time = 0;
    IO1active = false;
  }
}

void onReceive(uint8_t len, rl_packet_t* pIn)
{
  if (pIn->destinationID == SENSOR_ID && pIn->senderID == 0)
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
      digitalWrite(PIN_RELAY, HIGH);

      // send ON state to H.A.
      RLcomm.publishNum(0, SENSOR_ID, CHILD_RELAY_ID, S_SWITCH, 1);

      // active auto OFF timeout
      tempoRelay = (curTime + duree) | 1;
      return;
    }
  }
}
