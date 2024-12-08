/*
  Version 1.0
  Designed for ATTiny3216 (internal oscillator 1MHz)
  Hardware : any MCB with MLT3216 compatibility

  ##########################################

*/
// LoRa2HA ATTiny serie-1 general pinout
//
//                 ATtiny3216
//                   _____
//            VDD  1|*    |20 GND
// (nSS)  PA4  0~  2|     |19 16~ PA3 (SCK)
// (IN1)  PA5  1~  3|     |18 15  PA2 (MISO)
// (DIO0) PA6  2   4|     |17 14  PA1 (MOSI)
// (nRST) PA7  3   5|     |16 17  PA0 (UPDI)
// (IN3)  PB5  4   6|     |15 13  PC3 (IN4 only binary)
// (IN2)  PB4  5   7|     |14 12  PC2 (LEDs)
// (RXD)  PB3  6   8|     |13 11~ PC1 (OUT1)
// (TXD)  PB2  7~  9|     |12 10~ PC0 (OUT2)
// (SDA)  PB1  8~ 10|_____|11  9~ PB0 (SCL)

#include <Arduino.h>

//--- I/O pin ---
#define PIN_IN1       PIN_PA5
#define PIN_IN2       PIN_PB4
#define PIN_IN3       PIN_PB5
#define PIN_IN4       PIN_PC3
#define PIN_OUT1      PIN_PC1
#define PIN_OUT2      PIN_PC0
#define PIN_DEBUG_LED PIN_PC2

#define DEBUG_LED
//#define DEBUG_RGB

//--- debug tools ---

#ifdef DEBUG_LED
#define LED_INIT pinMode(PIN_DEBUG_LED, OUTPUT)
#define LED_ON   digitalWrite(PIN_DEBUG_LED, HIGH)
#define LED_OFF  digitalWrite(PIN_DEBUG_LED, LOW)
#else
#define LED_INIT
#define LED_ON
#define LED_OFF
#endif

#ifdef DEBUG_RGB
#include <tinyNeoPixel_Static.h>
#define NUMLEDS 3
byte pixels[NUMLEDS * 3];
tinyNeoPixel leds = tinyNeoPixel(NUMLEDS, PIN_DEBUG_LED, NEO_GRB, pixels);
#define LED_INIT {pinMode(PIN_DEBUG_LED, OUTPUT); leds.setBrightness(10);}
#define LED_ON {leds.setPixelColor(0, leds.Color(0, 0, 128)); leds.show();}
#define LED_OFF {leds.setPixelColor(0, leds.Color(0, 0, 0)); leds.show();}
#else
#define LED_INIT
#define LED_ON
#define LED_OFF
#endif

//############################
//## Equipment dictionnary ###
//############################

// initial setup
void setup()
{/*
  // disable all pins in first to prevent consumption
  for (uint8_t pin = 0; pin < 8; pin++) {
    (&PORTA.PIN0CTRL)[pin] = PORT_ISC_INPUT_DISABLE_gc;
    (&PORTB.PIN0CTRL)[pin] = PORT_ISC_INPUT_DISABLE_gc;
#if defined(__AVR_ATtinyxy6__)
    (&PORTC.PIN0CTRL)[pin] = PORT_ISC_INPUT_DISABLE_gc;
#endif
  }
*/
  LED_INIT;
}

void loop()
{
  LED_ON;
  delay(100);
  LED_OFF;
  delay(900);
}
