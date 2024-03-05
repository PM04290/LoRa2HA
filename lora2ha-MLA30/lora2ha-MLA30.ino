/*
  Version 1.0
  Designed for ATTiny84 (internal oscillator 1MHz)
  Hardware : MLA30 v1.1

  ##########################################
  IO0 : ANA0 or D0 or 1WIRE
  IO1 : ANA1 or D1
  IO2 : ANA7 or D7

  PMos load switch :
  https://blog.mbedded.ninja/electronics/circuit-design/load-switches/
  https://wolles-elektronikkiste.de/en/the-mosfet-as-switch

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
#include "avr/power.h"
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

//--- VL33 Driver (for LoRa module) ---
#define PIN_VL33_DRV    9
#define DRVON           1
#define DRVOFF          0

//--- tools ---
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

// uncomment #define below according the hardware

//#define WITH_BINARY1       PIN_IO_0
//#define WITH_BINARY2       PIN_IO_1
#define WITH_BINARY3       PIN_IO_2

//#define WITH_DALLAS        PIN_IO_0  // only on IO_0

//#define WITH_THERMISTOR    PIN_IO_0
//#define WITH_THERMISTOR    PIN_IO_1
//#define WITH_THERMISTOR    PIN_IO_2

//#define WITH_PHOTORESISTOR PIN_IO_0
//#define WITH_PHOTORESISTOR PIN_IO_1
//#define WITH_PHOTORESISTOR PIN_IO_2

// sleep mode cofiguration if IO interrupt wake up
// according with WITH_BINARYn upper
// only binary sensor FALL to GND event
//#define WAKEUP_ON_IO0
//#define WAKEUP_ON_IO1
#define WAKEUP_ON_IO2

#ifdef WITH_DALLAS
#include <OneWire.h>
#include <DallasTemperature.h>
#endif

// RadioLink IDs
#define HUB_UID         0  // Gateway UID Code
#define SENSOR_ID      50  // Device @
#define CHILD_PARAM     0  // For future
#define CHILD_ID_VBAT   1  // Vbat : ID 1
#define CHILD_ID_INPUT1 2  // IO0  : ID 2
#define CHILD_ID_INPUT2 3  // IO1  : ID 3
#define CHILD_ID_INPUT3 4  // IO2  : ID 4

RadioLinkClass RLcomm;

#ifdef WITH_DALLAS
OneWire oneWire(PIN_IO_0);             // Verify compatible hardware for IO0 input
DallasTemperature sensor0(&oneWire);
#endif

bool LoraOK = false;
bool DallasOK = false;

// interrupts
uint16_t WDTcount = 0;

// LoRa frequency
const uint32_t LRfreq = 435;

uint16_t mVCC = 3700; // 3.7v

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

  DEBUGln("\nStart");

  if (startLoRa())
  {
    delay(100);
  }
  LED_OFF
  delay(500);

#ifdef WITH_DALLAS
  sensor0.begin();
  DallasOK = sensor0.getDeviceCount() > 0;
  if (DallasOK)
  {
    DEBUG("Dallas OK");
    // ok
  } else
  {
    DEBUG("Dallas Error");
    for (byte b = 0; b < 3; b++)
    {
      LED_ON;
      delay(50);
      LED_OFF;
      delay(50);
    }
  }
#endif
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
    DEBUGln("LoRa OK");
    return true;
  } else
  {
    DEBUGln("LoRa Error");
    // To show LoRa error on H1 LED
    for (byte b = 0; b < 3; b++)
    {
      LED_OFF;
      delay(100);
      LED_ON;
      delay(100);
    }
  }
  return false;
}

void sendData()
{
  static uint16_t oldvBat = 0;
  static int16_t oldTemp = -2000;
  static int16_t oldLux = 0;
  static uint8_t oldBinary0 = 0xFF;
  static uint8_t oldBinary1 = 0xFF;
  static uint8_t oldBinary2 = 0xFF;
  if (LoraOK)
  {
    DEBUGln("Send data");
    // VCC
    mVCC = readVcc();
    if (mVCC != oldvBat)
    {
      oldvBat = mVCC;
      DEBUGln(" > VCC");
      RLcomm.publishFloat(HUB_UID, SENSOR_ID, CHILD_ID_VBAT, S_NUMERICSENSOR, mVCC, 1000, 1);
    }

#ifdef WITH_THERMISTOR
    // Send Thermistor sensor (analog sensor) from IO_0
    uint16_t ADCtemperature = analogRead(WITH_THERMISTOR);
    int16_t valTemp = calcTemperature(ADCtemperature) * 10.0; // 1/10°
    if (abs(valTemp - oldTemp) > 3)
    {
      oldTemp = valTemp;
      int chidID = WITH_THERMISTOR == PIN_IO_0 ? CHILD_ID_INPUT1 : WITH_THERMISTOR == PIN_IO_1 ? CHILD_ID_INPUT2 : CHILD_ID_INPUT3;
      DEBUGln(" > Therm");
      RLcomm.publishFloat(HUB_UID, SENSOR_ID, chidID, S_NUMERICSENSOR, valTemp, 10, 1);
    }
#endif

#ifdef WITH_DALLAS
    // Send Dallas sensor, from IO_0
    if (DallasOK)
    {
      sensor0.begin();
      sensor0.requestTemperatures();
      int16_t tDallas = sensor0.getTempCByIndex(0) * 10;
      if (abs(tDallas - oldTemp) > 3)
      {
        oldTemp = tDallas;
        DEBUGln(" > Dallas");
        RLcomm.publishFloat(HUB_UID, SENSOR_ID, CHILD_ID_INPUT1, S_NUMERICSENSOR, tDallas, 10, 1);
      }
    }
#endif

#ifdef WITH_PHOTORESISTOR
    uint16_t valLux = ADCtoLux(analogRead(WITH_PHOTORESISTOR));
    if (abs(valLux - oldLux) > 10)
    {
      oldLux = valLux;
      int chidID = WITH_PHOTORESISTOR == PIN_IO_0 ? CHILD_ID_INPUT1 : WITH_PHOTORESISTOR == PIN_IO_1 ? CHILD_ID_INPUT2 : CHILD_ID_INPUT3;
      DEBUGln(" > Photo");
      RLcomm.publishNum(HUB_UID, SENSOR_ID, chidID, S_NUMERICSENSOR, valLux);
    }
#endif

#ifdef WITH_BINARY1
    // Send digital sensor IO_0
    uint8_t sBinary0 = digitalRead(WITH_BINARY1);
    if (sBinary0 != oldBinary0)
    {
      oldBinary0 = sBinary0;
      DEBUGln(" > Bin1");
      RLcomm.publishNum(HUB_UID, SENSOR_ID, CHILD_ID_INPUT1, S_BINARYSENSOR, !sBinary0);
    }
#endif

#ifdef WITH_BINARY2
    // Send digital sensor IO_1
    uint8_t sBinary1 = digitalRead(WITH_BINARY2);
    if (sBinary1 != oldBinary1)
    {
      oldBinary1 = sBinary1;
      DEBUGln(" > Bin2");
      RLcomm.publishNum(HUB_UID, SENSOR_ID, CHILD_ID_INPUT2, S_BINARYSENSOR, !sBinary1);
    }
#endif

#ifdef WITH_BINARY3
    // Send digital sensor IO_2
    uint8_t sBinary2 = digitalRead(WITH_BINARY3);
    if (sBinary2 != oldBinary2)
    {
      oldBinary2 = sBinary2;
      DEBUGln(" > Bin3");
      RLcomm.publishNum(HUB_UID, SENSOR_ID, CHILD_ID_INPUT3, S_BINARYSENSOR, !sBinary2);
    }
#endif
  } else
  {
    LED_ON;
    delay(50);
    LED_OFF;
  }
}

// Power Off driver : Watchdog timing and IO Interrupt
void powerOff()
{
  DEBUGln("goto sleep");
  RLcomm.sleep();
  RLcomm.end();
  digitalWrite(PIN_VL33_DRV, DRVOFF);
  ADCSRA &= ~(1 << ADEN); // Disable ADC

#if defined(WAKEUP_ON_IO0) || defined(WAKEUP_ON_IO1) || defined(WAKEUP_ON_IO2)
  GIMSK |= bit(PCIE0);      // Enable Pin Change Interrupts PCINT7..0

#if defined(WAKEUP_ON_IO0)
  PCMSK0 |= bit(PCINT0);    // Use IO0 as interrupt pin
#endif
#if defined(WAKEUP_ON_IO1)
  PCMSK0 |= bit(PCINT1);    // Use IO1 as interrupt pin
#endif
#if defined(WAKEUP_ON_IO2)
  PCMSK0 |= bit(PCINT7);    // Use IO2 as interrupt pin
#endif

#endif
  //
  sleep_enable();
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);

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

  DEBUGln("wake up");

  digitalWrite(PIN_VL33_DRV, DRVON);
  ADCSRA |= (1 << ADEN); // ADC enabled
  delay(20);

  startLoRa();
}

// Thermistor calculation
#ifdef WITH_THERMISTOR
#define B           3100   // coef beta
#define RESISTOR    4700   // reference resistor
#define THERMISTOR  5000   // thermistor value
#define NOMINAL     298.15 // nominal temperature (Kelvin) (25°)
// coef calculator if unknown : https://www.thinksrs.com/downloads/programs/Therm%20Calc/NTCCalibrator/NTCcalculator.htm
float calcTemperature(int adcTemp)
{
  float vcc = float(mVCC) / 1000.;

  // if ntc on GND
  float ntcVoltage = ((float)adcTemp * vcc) / 1023.;
  float ntcResistance = (ntcVoltage) * RESISTOR / (vcc - ntcVoltage);

  // if ntc on VCC
  // float ntcResistance = RESISTOR * (1023. / adcTemp - 1.);

  float kelvin = (B * NOMINAL) / (B + (NOMINAL * log(ntcResistance / THERMISTOR)));
  float celcius = kelvin - 273.15;
  return celcius;
}
#endif

// Measure lux with Photoresistor
#ifdef WITH_PHOTORESISTOR
#define REF_RESISTANCE      100000
#define L1 2000      // forte lumière
#define R1 100000
#define L2 10        // faible lumière
#define R2 20000000

float coef_m = NAN;
float coef_b = NAN;
uint32_t ADCtoLux(int ldrRaw)
{
  if (isnan(coef_m)) {
    coef_m = (log10(L2) - log10(L1)) / (log10(R2) - log10(R1));
    coef_b =  log10(L1) - (coef_m * log10(R1));
  }
  float vcc = float(mVCC) / 1000.;

  // for LDR on VCC
  //float cRvoltage = ((float)ldrRaw * vcc) / 1023.;
  //float ldrResistance = (vcc - cRvoltage) * REF_RESISTANCE / cRvoltage;

  // for LDR on GND
  float ldrVoltage = ((float)ldrRaw * vcc) / 1023.;
  float ldrResistance = (ldrVoltage) * REF_RESISTANCE / (vcc - ldrVoltage);

  float lux = kalman( pow(10, coef_b) * pow(ldrResistance, coef_m) );
  return lux;
}
#endif

// for kalman
float q = 0.03;
float kalman_gain = 0;
float err_measure = 1;
float err_estimate = 2;
float current_estimate = 0;
float last_estimate = 0;
float kalman(float newVal)
{
  kalman_gain = err_estimate / (err_estimate + err_measure);
  current_estimate = last_estimate + kalman_gain * (newVal - last_estimate);
  err_estimate =  (1.0f - kalman_gain) * err_estimate + fabsf(last_estimate - current_estimate) * q;
  last_estimate = current_estimate;
  return current_estimate;
}

// Measure internal VCC
long readVcc() {
  long result;
  // Read 1.1V reference against AVcc
  // set the reference to Vcc and the measurement to the internal 1.1V reference
#if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
  ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#elif defined (__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
  ADMUX = _BV(MUX5) | _BV(MUX0) ;
#else
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#endif
  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Convert
  while (bit_is_set(ADCSRA, ADSC));
  result = ADCL;
  result |= ADCH << 8;
  result = 1126400L / result; // Back-calculate AVcc in mV
  return result;
}
