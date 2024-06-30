/**
 * DS1307 and DS3231 RTCs with AT24C32 (and compatible) integrated EEPROM basic library
 *
 * Really tiny library to basic RTC and EEPROM (incorporated) functionality on Arduino.
 *
 * DS1307 and DS3231 RTCs are supported AT24C32 EEPROM supported (and compatibles)
 *
 *
 * @copyright Naguissa
 * @author Naguissa
 * @email naguissa.com@gmail.com
 * @version 1.0
 * @created 2015-05-07
 *
 * MODIFIED by M&L 2024-04-26
 *   - suppress STM32
 *   - suppress EEPROM
 *   - naturally set available
*/
#ifndef RTCLIB
	#define RTCLIB
	#include "Arduino.h"
	#include "Wire.h"
	/*
	RTC I2C Address:
	DS3231 ROM 0x57
	DS3231 RTC 0x68
	*/
	#define RTCLIB_ADDRESS 0x68
	#define RTCLIB_EE_ADDRESS 0x57

	// Convert normal decimal numbers to binary coded decimal
	#define RTCLIB_decToBcd(val) ((uint8_t) ((val / 10 * 16) + (val % 10)))
	//#define RTCLIB_decToBcd(val) ((uint8_t) (val + 6 * (val / 10)))

	// Convert binary coded decimal to normal decimal numbers
	//#define RTCLIB_bcdToDec(val) ((uint8_t) (val - 6 * (val >> 4)))
	#define RTCLIB_bcdToDec(val) ((uint8_t) ((val / 16 * 10) + (val % 16)))


	class RTCLib {
		public:
			RTCLib();
			uint8_t second();
			uint8_t minute();
			uint8_t hour();
			uint8_t day();
			uint8_t month();
			uint8_t year();
			uint8_t seconds();
			uint8_t dayOfWeek();
			void refresh();
			void set(uint8_t second, uint8_t minute, uint8_t hour, uint8_t dayOfWeek, uint8_t dayOfMonth, uint8_t month, uint8_t year);

		private:
			uint8_t _second ;
			uint8_t _minute;
			uint8_t _hour ;
			uint8_t _day ;
			uint8_t _month ;
			uint8_t _year ;
			uint8_t _dayOfWeek ;
	};
#endif
