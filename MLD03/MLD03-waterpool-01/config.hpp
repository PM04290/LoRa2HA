//              ATtiny1616/3216
//                   _____
//            VDD  1|*    |20 GND
// (nSS)  PA4  0~  2|     |19 16~ PA3 (SCK)
// (nRST) PA5  1~  3|     |18 15  PA2 (MISO)
// (IN1)  PA6  2   4|     |17 14  PA1 (MOSI)
// (IN2)  PA7  3   5|     |16 17  PA0 (UPDI)
// (IN3)  PB5  4   6|     |15 13  PC3 (DIO0)
// (IN4)  PB4  5   7|     |14 12  PC2 (LEDs)
// (RXD)  PB3  6   8|     |13 11~ PC1 (OUT1)
// (TXD)  PB2  7~  9|     |12 10~ PC0 (OUT2)
// (SDA)  PB1  8~ 10|_____|11  9~ PB0 (SCL)

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
#define HUB_UID                  0 // HUB ID
#define MODULE_ID               21 // module ID
#define CHILD_SENSOR1            1 // sensor on JI1
#define CHILD_SENSOR2            2 // sensor on JI2
#define CHILD_SENSOR3            3 // sensor on JI3
#define CHILD_SENSOR4            4 // sensor on JI4 (or CN1)
#define CHILD_RELAY1             5 // relay on JR1
#define CHILD_RELAY2             6 // relay on JR2
#define CHILD_INPUTFILTER_START  7 // filter start hour
#define CHILD_INPUTREFILL_START  8 // refil start hour
#define CHILD_INPUTREFILL_DAY    9 // refill periodic
#define CHILD_INPUTREFILL_DUR   10 // refill duration
#define CHILD_INPUTFROST_L      11 // frost thresholds Low
#define CHILD_INPUTFROST_H      12 // frost thresholds High
#define CHILD_SCH_FILTER        13 // mode for scheduler (off, periodic);
#define CHILD_SCH_REFILL        14 // mode for scheduler (off, periodic);
#define CHILD_REGUL_FROST       15 // mode for Regul (Manu, Auto);

#define CHILD_NONE         0

// EEPROM mapping (do not change order)

#define EEP_UID               0
#define EEP_HUBID             (EEP_UID+1)
#define EEP_REGULFROST        (EEP_HUBID+1)
#define EEP_SCHDFILTER        (EEP_REGULFROST+1)
#define EEP_SCHDREFILL        (EEP_SCHDFILTER+1)
#define EEP_INPUTFROST_L      (EEP_SCHDREFILL+1)
#define EEP_INPUTFROST_H      (EEP_INPUTFROST_L+sizeof(inputRecord))
#define EEP_INPUTFILTER_START (EEP_INPUTFROST_H+sizeof(inputRecord))
#define EEP_INPUTREFILL_START (EEP_INPUTFILTER_START+sizeof(inputRecord))
#define EEP_INPUTREFILL_DAY   (EEP_INPUTREFILL_START+sizeof(inputRecord))
#define EEP_INPUTREFILL_DUR   (EEP_INPUTREFILL_DAY+sizeof(inputRecord))

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
