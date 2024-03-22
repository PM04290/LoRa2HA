#pragma once

#define VERSION "0.4"

// uncomment if you want ETHERNET instead of WIFI (like OLIMEX_POE_ISO or WT32-ETH01)
#define USE_ETHERNET

// this define permit switch from WiFi to Ehternet (need define USE_ETHERNET)
// uncomment if ETH link is available (like WT32-ETH01) or dedicated pin with swith button to swap from Wifi to Ehternet
//#define PIN_ETH_LINK 33

#define EEPROM_MAX_SIZE     256
#define EEPROM_TEXT_OFFSET  16
#define EEPROM_TEXT_SIZE    48

char Wifi_ssid[EEPROM_TEXT_SIZE] = "";  // WiFi SSID
char Wifi_pass[EEPROM_TEXT_SIZE] = "";  // WiFi password

char mqtt_host[EEPROM_TEXT_SIZE] = "192.168.0.100";
int mqtt_port = 1883;
char mqtt_user[EEPROM_TEXT_SIZE] = "";  // MQTT user
char mqtt_pass[EEPROM_TEXT_SIZE] = "";  // MQTT password

char AP_ssid[9] = "lora2ha0";  // AP WiFi SSID
char AP_pass[9] = "12345678";  // AP WiFi password

uint8_t UIDcode = 0;
uint16_t RadioFreq = 433;

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
