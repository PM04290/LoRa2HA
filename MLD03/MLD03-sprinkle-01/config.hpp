// ATtiny3216 / ARDUINO
//                          _____
//                  VDD   1|*    |20  GND
// (nSS)  (AIN4) PA4  0~  2|     |19  16~ PA3 (AIN3)(SCK)(EXTCLK)
//        (AIN5) PA5  1~  3|     |18  15  PA2 (AIN2)(MISO)
// (DAC)  (AIN6) PA6  2   4|     |17  14  PA1 (AIN1)(MOSI)
//        (AIN7) PA7  3   5|     |16  17  PA0 (AIN0/nRESET/UPDI)
//        (AIN8) PB5  4   6|     |15  13  PC3
//        (AIN9) PB4  5   7|     |14  12  PC2
// (RXD) (TOSC1) PB3  6   8|     |13  11~ PC1 (PWM only on 1-series)
// (TXD) (TOSC2) PB2  7~  9|     |12  10~ PC0 (PWM only on 1-series)
// (SDA) (AIN10) PB1  8~ 10|_____|11   9~ PB0 (AIN11)(SCL)
//

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

// RadioLink Ids (child 250..255 are reserved, 0 prohibited
#define HUB_UID            0 // HUB ID
#define MODULE_ID         22 // module ID
#define CHILD_SENSOR1      1 // sensor on JI1
#define CHILD_SENSOR2      2 // sensor on JI2
#define CHILD_SENSOR3      3 // sensor on JI3
#define CHILD_SENSOR4      4 // sensor on JI4
#define CHILD_RELAY1       5 // relay on JR1
#define CHILD_RELAY2       6 // relay on JR2
#define CHILD_INPUT_DAY1   7 // input day config for Z1
#define CHILD_INPUT_HOUR1  8 // input start hour for Z1
#define CHILD_INPUT_DUR1   9 // input duration for Z1
#define CHILD_INPUT_HUM1  10 // input Humidity for Z1
#define CHILD_INPUT_DAY2  11 // input day config for Z2
#define CHILD_INPUT_HOUR2 12 // input start hour for Z2
#define CHILD_INPUT_DUR2  13 // input duration for Z2
#define CHILD_INPUT_HUM2  14 // input Humidity for Z2
#define CHILD_SCHED1      15 // mode for Regul 1
#define CHILD_SCHED2      16 // mode for Regul 2

#define CHILD_NONE         0

// EEPROM mapping (do not change order)
#define EEP_NONE -1

#define EEP_UID         0
#define EEP_HUBID       (EEP_UID+1)
#define EEP_INPUT_DAY1  (EEP_HUBID+1)
#define EEP_INPUT_HUM1  (EEP_INPUT_DAY1+sizeof(inputRecord))
#define EEP_INPUT_HOUR1 (EEP_INPUT_HUM1+sizeof(inputRecord))
#define EEP_INPUT_DUR1  (EEP_INPUT_HOUR1+sizeof(inputRecord))
#define EEP_INPUT_DAY2  (EEP_INPUT_DUR1+sizeof(inputRecord))
#define EEP_INPUT_HUM2  (EEP_INPUT_DAY2+sizeof(inputRecord))
#define EEP_INPUT_HOUR2 (EEP_INPUT_HUM2+sizeof(inputRecord))
#define EEP_INPUT_DUR2  (EEP_INPUT_HOUR2+sizeof(inputRecord))
#define EEP_SCHED1      (EEP_INPUT_DUR2+sizeof(inputRecord))
#define EEP_SCHED2      (EEP_SCHED1+1)

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
