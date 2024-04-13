#pragma once

#define VERSION "0.6"

#if defined(ESP32S2)
#define PIN_WMODE 13 // MLH1
#elif defined(ARDUINO_ESP32_POE_ISO) || defined(ARDUINO_ESP32_POE)
#define PIN_WMODE 32 // MLH2
#elif defined(ARDUINO_WT32_ETH01)
#define PIN_WMODE 32 // MLH3
#else
#define PIN_WMODE 32 // MLH4 or custon ESP32 WROOM
#endif

// automatic use ETHERNET with Olimex POE and WT32-ETH01
#if defined(ARDUINO_WT32_ETH01) || defined(ARDUINO_ESP32_POE_ISO) || defined(ARDUINO_ESP32_POE)
#define USE_ETHERNET

//#define FORCE_ETHERNET

#endif

// this define permit switch from WiFi to Ehternet (need define USE_ETHERNET)
#if defined(ARDUINO_WT32_ETH01)
#define PIN_ETH_LINK 33
#endif

#define EEPROM_MAX_SIZE    256

#define EEPROM_DATA_CODE     0
#define EEPROM_DATA_FREQ     1
#define EEPROM_DATA_COUNT   15

#define EEPROM_TEXT_OFFSET  16
#define EEPROM_TEXT_SIZE    48

char Wifi_ssid[EEPROM_TEXT_SIZE] = "";  // WiFi SSID
char Wifi_pass[EEPROM_TEXT_SIZE] = "";  // WiFi password

char mqtt_host[EEPROM_TEXT_SIZE] = "192.168.0.100";
uint16_t mqtt_port = 1883;
char mqtt_user[EEPROM_TEXT_SIZE] = "";  // MQTT user
char mqtt_pass[EEPROM_TEXT_SIZE] = "";  // MQTT password

char AP_ssid[9] = "lora2ha0";  // AP WiFi SSID
char AP_pass[9] = "12345678";  // AP WiFi password

uint8_t UIDcode = 0;
uint16_t RadioFreq = 433;
uint8_t rstCount = 0;

#define DEBUG_SERIAL

#ifdef DEBUG_SERIAL
#define DEBUG(x) Serial.print(x)
#define DEBUGln(x) Serial.println(x)
#define DEBUGf(...) Serial.printf(__VA_ARGS__)
#else
#define DEBUG(x)
#define DEBUGln(x)
#define DEBUGf(x,y)
#endif 
