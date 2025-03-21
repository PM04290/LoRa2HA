/*
   SSD1306xLED - Drivers for SSD1306 controlled dot matrix OLED/PLED 128x32 displays

   @created: 2014-08-12
   @author: Neven Boyanov
   @date: 2020-04-14
   @author: Simon Merrett
   @version: 0.2

   Source code for original version available at: https://bitbucket.org/tinusaur/ssd1306xled
   Source code for ATtiny 0 and 1 series with megaTinyCore: https://github.com/SimonMerrett/tinyOLED
   megaTinyCore available at: https://github.com/SpenceKonde/megaTinyCore

   Changelog:
   - V0.2 use conditional #includes to take advantage of memory-saving TinyMegaI2CMaster library by David Johnson-Davies
   - V0.1 initial commit for sharing and comment. Main changes from Digistump and Tinusaur:
     - removed pgmspace use, as megaTinyCore doesn't need it to save arrays in flash (instead of RAM)
     - removed stdint, stdlib, interrupt, util/delay #includes
     - changed initialisation values, including to work initially only with 128*32 display
     - #defined _nofont_8x16 so that compiled code would fit on tiny402
     - removed Wire.writeAvailabe() from ssd1306_send_data_byte() as not using TinyWire wrapper
*/
//#include <stdint.h> //**
// #define TINYWIRE // Comment this out if you want to use the normal Wire library for I2C (more memory used though).
#include <Arduino.h>
#ifdef TINYWIRE
#include <TinyWireM.h>
#else
#include <Wire.h>
#endif // TINYWIRE
// #include <avr/pgmspace.h>
// #include <avr/interrupt.h>
// #include <util/delay.h> //**

#ifndef TINYOLED_H
#define TINYOLED_H

//#define _nofont_8x16		//tBUG
#ifndef _nofont_8x16	//tBUG
#define FONT8X16		1
#endif
#define FONT6X8		0

// ----------------------------------------------------------------------------

#ifndef SSD1306
#define SSD1306		0x3C	// Slave address
#endif

// ----------------------------------------------------------------------------

class SSD1306Device: public Print {

  public:
    SSD1306Device(void);
    void begin(void);
    void sleep(void);
    void setFont(uint8_t font);
    void ssd1306_send_command(uint8_t command);
    void ssd1306_send_data_byte(uint8_t byte);
    void ssd1306_send_data_start(void);
    void ssd1306_send_data_stop(void);
    void setCursor(uint8_t x, uint8_t y);
    void fill(uint8_t fill);
    void clear(void);
    void drawPixel(uint8_t x0, uint8_t y0);
    void bitmap(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, const uint8_t bitmap[]);
    void ssd1306_send_command_start(void);
    void ssd1306_send_command_stop(void);
    void ssd1306_char_f8x16(uint8_t x, uint8_t y, const char ch[]);
    virtual size_t write(byte c);
    using Print::write;


};


extern SSD1306Device oled;

// ----------------------------------------------------------------------------

#endif // TINYOLED_H
