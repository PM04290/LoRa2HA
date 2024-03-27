/*
  Version 1.0
  Designed for ATTiny84 (internal oscillator 1MHz)
  Hardware : MLA30 v1.1

  ##########################################
  IO0 : ANA0 or D0 or 1WIRE
  IO1 : ANA1 or D1
  IO2 : ANA7 or D7

*/

// ATMEL AVR ATTINY84
//                   +-\/-+
//             VCC  1|    |14  GND
//      (D10)  PB0  2|    |13  PA0  (D 0) IO_0
//      (D 9)  PB1  3|    |12  PA1  (D 1) IO_1
//       RST   PB3  4|    |11  PA2  (D 2) NRST
// DIO0 (D 8)  PB2  5|    |10  PA3  (D 3) NSS
// IO_2 (D 7)  PA7  6|    |9   PA4  (D 4) SCK
// MISO (D 6)  PA6  7|    |8   PA5  (D 5) MOSI
//                   +----+
// IMPORTANT : below, pin number "n" is for "D n"
// ---------

#include <Arduino.h>
#include <RadioLink.h>
#include "avr/sleep.h"
#include "avr/wdt.h"

//--- LoRa pin (implicit) ---
// NRST : 2    (can be modified, but need RadioLink modification)
// NSS  : 3    (can be modified, but need RadioLink modification)
// SCK  : 4    internal define, can't be modified
// MOSI : 5    internal define, can't be modified
// MISO : 6    internal define, can't be modified
// DIO0 : 8    (INT0, can be modified, but need RadioLink modification)

//--- I/O pin ---
#define PIN_IO_0        0
#define PIN_IO_1        1
#define PIN_IO_2        7

//--- VL33 Driver (for LoRa module & IO) ---
#define PIN_VL33_DRV    9
#define DRVON           1
#define DRVOFF          0

//--- Different define for compilation
#define T_TMP36  0x80
#define T_LM35   0x81
#define W_HX711  0x82
#define D_PULSE  0x83


//--- debug tools ---
#define DEBUG_LED

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

//############################
//## Equipment dictionnary ###
//############################

// uncomment #define below according the hardware

//#define WITH_BINARY_0       PIN_IO_0  //              Switch on IO0
//#define WITH_BINARY_1       PIN_IO_1  //              Switch on IO1
//#define WITH_BINARY_2       PIN_IO_2  //              Switch on IO2

//#define WITH_TEMP_0         PIN_IO_0  // \            Thermistor on IO0
//#define WITH_TEMP_0         T_TMP36   //  > 1 of 3    TMP36 on IO0
//#define WITH_TEMP_0         T_LM35    // /            LM35 on IO0
//#define WITH_TEMP_1         PIN_IO_1  //              Thermistor on IO1
//#define WITH_TEMP_2         PIN_IO_2  //              Thermistor on IO0

//#define WITH_PHOTO_0        PIN_IO_0  //
//#define WITH_PHOTO_1        PIN_IO_1  //
//#define WITH_PHOTO_2        PIN_IO_2  //

//#define WITH_WEIGHT_0       PIN_IO_0  //              ampli-op on IO0 (analog read)
//#define WITH_WEIGHT_1       PIN_IO_1  //              ampli-op on IO1 (analog read)
//#define WITH_WEIGHT_2       PIN_IO_2  // \ 1 of 2     ampli-op on IO2 (analog read)
//#define WITH_WEIGHT_2       W_HX711   // /            implicit use IO_1 (SCK) and IO_2 (DT)

//#define WITH_DISTANCE       D_PULSE   //              implicit use IO_1 (pulse) and IO_2 (echo)

// sleep mode cofiguration if IO interrupt wake up
// according with WITH_BINARYn upper
// wake up on any change of IO (FALL, RISE)
//#define WAKEUP_ON_IO0
//#define WAKEUP_ON_IO1
//#define WAKEUP_ON_IO2


// RadioLink IDs
#define HUB_UID         0  // Gateway UID Code
#define SENSOR_ID      50  // Device @
#define CHILD_ID_VBAT   1  // Vbat : ID 1
#define CHILD_ID_INPUT1 2  // IO0  : ID 2
#define CHILD_ID_INPUT2 3  // IO1  : ID 3
#define CHILD_ID_INPUT3 4  // IO2  : ID 4
#define CHILD_PARAM  0xFF  // Sending config

RadioLinkClass RLcomm;
bool LoraOK = false;

#include "MLsensor.hpp"
uint16_t mVCC = 3700; // 3.7v

#include "vcc.hpp"

#if defined(WITH_BINARY_0) || defined(WITH_BINARY_1) || defined(WITH_BINARY_2)
#include "binary.hpp"
#endif

#if defined(WITH_TEMP_0) || defined(WITH_TEMP_1) ||defined(WITH_TEMP_2)
#include "temperature.hpp"
#endif

#if defined(WITH_PHOTO_0) || defined(WITH_PHOTO_1) ||defined(WITH_PHOTO_2)
#include "photoresistor.hpp"
#endif

#if defined(WITH_WEIGHT_0) || defined(WITH_WEIGHT_1) || defined(WITH_WEIGHT_2)
#include "weight.hpp"
#endif

#if WITH_DISTANCE == D_PULSE
#include "ultrasonic.hpp"
#endif


// interrupts
uint16_t WDTcount = 0;

// LoRa frequency
const uint32_t LRfreq = 433;

ISR(WDT_vect)
{
  WDTcount++;
}

ISR(PCINT0_vect)
{
  GIFR = 0;
  WDTcount = 0x7FFF; // overflow wdt counter
}

// initial setup
void setup()
{
  LED_INIT;
  LED_ON;
  DEBUGinit();

  pinMode(PIN_IO_0, INPUT_PULLUP);
  pinMode(PIN_IO_1, INPUT_PULLUP);
  pinMode(PIN_IO_2, INPUT_PULLUP);

  // secondary V33 active
  pinMode(PIN_VL33_DRV, OUTPUT);
  digitalWrite(PIN_VL33_DRV, DRVON);
  delay(100);

  DEBUGln(F("\nStart"));

  if (startLoRa())
  {
    delay(100);
  }
  LED_OFF;

  // Declare any sensor in module
  ML_addSensor(new VCC(CHILD_ID_VBAT));

#ifdef WITH_BINARY_0
#ifdef WAKEUP_ON_IO0
  ML_addSensor(new BinaryIO(CHILD_ID_INPUT1, WITH_BINARY_0, true));  // Trigger
#else
  ML_addSensor(new BinaryIO(CHILD_ID_INPUT1, WITH_BINARY_0));        // Binary sensor
#endif
#endif

#ifdef WITH_BINARY_1
#ifdef WAKEUP_ON_IO1
  ML_addSensor(new BinaryIO(CHILD_ID_INPUT2, WITH_BINARY_1, true));
#else
  ML_addSensor(new BinaryIO(CHILD_ID_INPUT2, WITH_BINARY_1));
#endif
#endif

#ifdef WITH_BINARY_2
#ifdef WAKEUP_ON_IO2
  ML_addSensor(new BinaryIO(CHILD_ID_INPUT3, WITH_BINARY_2, true));
#else
  ML_addSensor(new BinaryIO(CHILD_ID_INPUT3, WITH_BINARY_2));
#endif
#endif

#ifdef WITH_TEMP_0
  ML_addSensor(new Temperature(CHILD_ID_INPUT1, WITH_TEMP_0, 3.0));
#endif

#ifdef WITH_TEMP_1
  ML_addSensor(new Temperature(CHILD_ID_INPUT2, WITH_TEMP_1, 3.0));
#endif

#ifdef WITH_TEMP_2
  ML_addSensor(new Temperature(CHILD_ID_INPUT3, WITH_TEMP_2, 3.0));
#endif

#ifdef WITH_PHOTO_0
  ML_addSensor(new Photoresistor(CHILD_ID_INPUT1, WITH_PHOTO_0, 30));
#endif

#ifdef WITH_PHOTO_1
  ML_addSensor(new Photoresistor(CHILD_ID_INPUT2, WITH_PHOTO_1, 30));
#endif

#ifdef WITH_PHOTO_2
  ML_addSensor(new Photoresistor(CHILD_ID_INPUT3, WITH_PHOTO_2, 30));
#endif

#ifdef WITH_WEIGHT_0
  ML_addSensor(new Weight(CHILD_ID_INPUT1, WITH_WEIGHT_0, 10));
#endif

#ifdef WITH_WEIGHT_1
  ML_addSensor(new Weight(CHILD_ID_INPUT2, WITH_WEIGHT_1, 10));
#endif

#ifdef WITH_WEIGHT_2
  ML_addSensor(new Weight(CHILD_ID_INPUT3, WITH_WEIGHT_2, 10));
#endif

#if WITH_DISTANCE == D_PULSE
  ML_addSensor(new Ultrasonic(CHILD_ID_INPUT3, 5));
#endif

  ML_begin();
  // publish all sensors configuration (only for LoRa HUB)
  ML_PublishConfigSensors();
}

void loop()
{
  sendData();
  powerOff();
}

bool startLoRa()
{
  LoraOK = RLcomm.begin(LRfreq * 1E6, NULL, NULL, 15);
  if (LoraOK)
  {
    RLcomm.setWaitOnTx(true);
    DEBUGln(F("LR OK"));
    return true;
  } else
  {
    DEBUGln(F("LR KO"));
#ifdef DEBUG_LED
    // To show LoRa error on H1 LED
    for (byte b = 0; b < 3; b++)
    {
      LED_OFF; delay(100); LED_ON; delay(100);
    }
#endif
  }
  return false;
}

void sendData()
{
  if (LoraOK)
  {
    LED_ON;
    DEBUGln(F("data?"));
    // Walk any sensor in Module to send data
    ML_SendSensors();
  } else {
    LED_ON; delay(30);
  }
  LED_OFF;
}

// Power Off driver : Watchdog timing and IO Interrupt
void powerOff()
{
  DEBUGln(F("sleep"));
  RLcomm.sleep();
  RLcomm.end();
  digitalWrite(PIN_VL33_DRV, DRVOFF);
  ADCSRA &= ~(1 << ADEN); // Disable ADC

#if defined(WAKEUP_ON_IO0) || defined(WAKEUP_ON_IO1) || defined(WAKEUP_ON_IO2)
  GIMSK |= _BV(PCIE0);      // Enable Pin Change Interrupts PCINT7..0

#if defined(WAKEUP_ON_IO0)
  PCMSK0 |= _BV(PCINT0);    // Use IO0 as interrupt pin
#endif
#if defined(WAKEUP_ON_IO1)
  PCMSK0 |= _BV(PCINT1);    // Use IO1 as interrupt pin
#endif
#if defined(WAKEUP_ON_IO2)
  PCMSK0 |= _BV(PCINT7);    // Use IO2 as interrupt pin
#endif

#endif
  //
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();

  // Set the watchdog to wake us up and turn on its interrupt
  wdt_enable(WDTO_4S);

  // Sleep for 1mn (15 x 4s)
  sei();
  do {
    WDTCSR |= bit(WDIE);
    sleep_cpu();
    wdt_reset();
  } while (WDTcount < 15);

  // Back from sleep
  sleep_disable();
  wdt_disable();
  WDTcount = 0;

  cli();
  PCMSK0 &= ~bit(PCINT0); // Turn off interrupt pin
  PCMSK0 &= ~bit(PCINT1);
  PCMSK0 &= ~bit(PCINT7);
  sei();

  DEBUGln(F("wake-up"));

  digitalWrite(PIN_VL33_DRV, DRVON);
  ADCSRA |= (1 << ADEN); // ADC enabled
  delay(20);

  startLoRa();
}
