
// I/O pin
#if defined(__AVR_ATtiny1616__) || defined(__AVR_ATtiny3216__)
#define PIN_I_1        2
#define PIN_I_2        3
#define PIN_I_3        4
#define PIN_I_4        5
#define PIN_R_2        10
#define PIN_R_1        11
#define PIN_LEDS       12
#else
#define PIN_I_1        A0
#define PIN_I_2        A1
#define PIN_I_3        A2
#define PIN_I_4        A3
#define PIN_R_1        5
#define PIN_R_2        4
#define PIN_LEDS       3
#endif

// RadioLink Ids (child 254 a 255 are reserved, 0 prohibited
#define HUB_UID            3 // HUB ID
#define SENSOR_ID         23 // module ID
#define CHILD_ID_INPUT1    1 // sensor on JI1
#define CHILD_ID_INPUT2    2 // sensor on JI2
#define CHILD_ID_INPUT3    3 // sensor on JI3 (or CN1)
#define CHILD_ID_INPUT4    4 // sensor on JI4
#define CHILD_RELAY1       5 // relay on JR1
#define CHILD_RELAY2       6 // relay on JR2
#define CHILD_THRESHOLD1L  7 // thresholds Low for R1
#define CHILD_THRESHOLD1H  8 // thresholds High for R2
#define CHILD_THRESHOLD2L  9 // thresholds Low for R1
#define CHILD_THRESHOLD2H 10 // thresholds High for R2
#define CHILD_THRESHOLD3  11 // thresholds start hour
#define CHILD_REGUL1      12 // mode for Regul 1 (Manu, Auto);
#define CHILD_REGUL2      13 // mode for Regul 2 (Manu, Auto);
#define CHILD_SCHED2      14 // scheduler for filter
#define CHILD_IDLEFILTER  15 // button for idle filter

// EEPROM mapping (do not change order)
#define EEP_NONE -1

#define EEP_UID         0
#define EEP_HUBID       (EEP_UID+1)
#define EEP_REGUL1      (EEP_HUBID+1)
#define EEP_REGUL2      (EEP_REGUL1+1)
#define EEP_THLD1L      (EEP_REGUL2+1)
#define EEP_THLD1H      (EEP_THLD1L+sizeof(thresholdRecord))
#define EEP_THLD2L      (EEP_THLD1H+sizeof(thresholdRecord))
#define EEP_THLD2H      (EEP_THLD2L+sizeof(thresholdRecord))
#define EEP_THLDSTART   (EEP_THLD2H+sizeof(thresholdRecord))
#define EEP_SCHED1      (EEP_THLDSTART+sizeof(thresholdRecord))

// ###########################################
// ## If LED debug wanted                   ##
// ##                                       ##
#ifdef DEBUG_LED
#include <tinyNeoPixel.h>
#define NUM_LEDS 3
tinyNeoPixel leds = tinyNeoPixel(NUM_LEDS, PIN_LEDS, NEO_GRB + NEO_KHZ800);
#define LED_INIT {leds.begin(); leds.setBrightness(10);}
#define LED_RED(x) {leds.setPixelColor(x, leds.Color(255, 0, 0)); leds.show();}
#define LED_GREEN(x) {leds.setPixelColor(x, leds.Color(0, 255, 0)); leds.show();}
#define LED_BLUE(x) {leds.setPixelColor(x, leds.Color(0, 0, 255)); leds.show();}
#define LED_OFF(x) {leds.setPixelColor(x, leds.Color(0, 0, 0)); leds.show();}
#else
#define LED_INIT
#define LED_RED(x)
#define LED_GREEN(x)
#define LED_BLUE(x)
#define LED_OFF(x)
#endif
// ##                                       ##
// ###########################################

// ###########################################
// ## If Serial debug wanted                ##
// ##                                       ##
#ifdef DEBUG_SERIAL
#define DEBUGinit() Serial.begin(115200)
#define DEBUG(x) Serial.print(x)
#define DEBUGln(x) Serial.println(x)
#else
#define DEBUGinit()
#define DEBUG(x)
#define DEBUGln(x)
#endif
// ##                                       ##
// ###########################################

// ###########################################
// ## If RTC DS3231 is installed            ##
// ##                                       ##
#ifdef HARDWARE_RTC
DateTime rtc;
#define RTC_READ rtc.readDS3231()
#define RTC_WRITE rtc.writeDS3231()
#else
DateTime rtc;
#define RTC_READ
#define RTC_WRITE
#endif
// ##                                       ##
// ###########################################
