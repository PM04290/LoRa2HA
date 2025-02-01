#pragma once

#define VERSION "1.2"

#if defined(ARDUINO_LOLIN_S2_MINI)
#define PIN_WMODE 13 // MLH1
//#define UPDI_TX   
//#define UPDI_RX   
#elif defined(ARDUINO_ESP32_POE_ISO) || defined(ARDUINO_ESP32_POE)
#define PIN_WMODE 32 // MLH2
#define UPDI_TX   13
#define UPDI_RX   16
#elif defined(ARDUINO_WT32_ETH01)
#define PIN_WMODE 32 // MLH3
#define UPDI_TX   17
#define UPDI_RX    5
#else
#define PIN_WMODE 32 // MLH4 or custon ESP32 WROOM
#define UPDI_TX   21
#define UPDI_RX   22
#endif

// automatic use ETHERNET with Olimex POE and WT32-ETH01
#if defined(ARDUINO_WT32_ETH01) || defined(ARDUINO_ESP32_POE_ISO) || defined(ARDUINO_ESP32_POE)
#define USE_ETHERNET
#define FORCE_ETHERNET // uncomment this if you want to force Ethernet every time
#endif

// this define permit switch from WiFi to Ehternet (need define USE_ETHERNET)
#if defined(ARDUINO_WT32_ETH01)
#define PIN_ETH_LINK 33
#endif

#define DEFAULT_ONLINE_WATCHDOG 10 // in min

#define EEPROM_MAX_SIZE    512

#define EEPROM_DATA_CODE     0
#define EEPROM_DATA_FREQ     1
#define EEPROM_DATA_RANGE    2
#define EEPROM_DATA_WDOG    14

#define EEPROM_TEXT_OFFSET  16
#define EEPROM_TEXT_SIZE    48

char Wifi_ssid[EEPROM_TEXT_SIZE] = "";  // WiFi SSID
char Wifi_pass[EEPROM_TEXT_SIZE] = "";  // WiFi password

char mqtt_host[EEPROM_TEXT_SIZE] = "192.168.0.100";
uint16_t mqtt_port = 1883;
char mqtt_user[EEPROM_TEXT_SIZE] = "";  // MQTT user
char mqtt_pass[EEPROM_TEXT_SIZE] = "";  // MQTT password

char datetimeTZ[EEPROM_TEXT_SIZE] = "CET-1CEST,M3.5.0,M10.5.0/3";
char datetimeNTP[EEPROM_TEXT_SIZE] = "europe.pool.ntp.org";
char defaultNTP[EEPROM_TEXT_SIZE] = "pool.ntp.org";

char AP_ssid[9] = "lora2ha0";  // AP WiFi SSID
char AP_pass[9] = "12345678";  // AP WiFi password

uint8_t UIDcode = 0;
uint16_t RadioFreq = 433;
uint8_t RadioRange = 1;
uint8_t rstCount = 0;
uint8_t Watchdog = DEFAULT_ONLINE_WATCHDOG;

const char HAConfigRoot[]                PROGMEM = {"homeassistant"};
const char HAlora2ha[]                   PROGMEM = {"lora2ha"};

const char HAComponentBinarySensor[]     PROGMEM = {"binary_sensor"};
const char HAComponentSensor[]           PROGMEM = {"sensor"};
const char HAComponentNumber[]           PROGMEM = {"number"};
const char HAComponentSelect[]           PROGMEM = {"select"};
const char HAComponentSwitch[]           PROGMEM = {"switch"};
const char HAComponentTag[]              PROGMEM = {"tag"};
const char HAComponentDeviceAutomation[] PROGMEM = {"device_automation"};

const char HAIdentifiers[]       PROGMEM = {"ids"};
const char HAManufacturer[]      PROGMEM = {"mf"};
const char HAModel[]             PROGMEM = {"mdl"};
const char HASwVersion[]         PROGMEM = {"sw"};
const char HAViaDevice[]         PROGMEM = {"via_device"};
const char HAUniqueID[]          PROGMEM = {"unique_id"};
const char HAObjectID[]          PROGMEM = {"objt_id"};
const char HAName[]              PROGMEM = {"name"};
const char HATopic[]             PROGMEM = {"topic"};
const char HAPayload[]           PROGMEM = {"payload"};
const char HAState[]             PROGMEM = {"state"};
const char HAStateTopic[]        PROGMEM = {"stat_t"};
const char HAStateOpen[]         PROGMEM = {"stat_open"};
const char HAStateOpening[]      PROGMEM = {"stat_opening"};
const char HAStateClosed[]       PROGMEM = {"stat_closed"};
const char HAStateClosing[]      PROGMEM = {"stat_closing"};
const char HAStateStopped[]      PROGMEM = {"stat_stopped"};
const char HAPayloadOpen[]       PROGMEM = {"pl_open"};
const char HAPayloadStop[]       PROGMEM = {"pl_stop"};
const char HAPayloadClose[]      PROGMEM = {"pl_close"};
const char HACommand[]           PROGMEM = {"command"};
const char HACommandTopic[]      PROGMEM = {"cmd_t"};
const char HADeviceClass[]       PROGMEM = {"dev_cla"};
const char HAEntityCategory[]    PROGMEM = {"ent_cat"};
const char HAUnitOfMeasurement[] PROGMEM = {"unit_of_meas"};
const char HAStateNone[]         PROGMEM = {"none"};
const char HAStateOn[]           PROGMEM = {"ON"};
const char HAStateOff[]          PROGMEM = {"OFF"};
const char HAExpireAfter[]       PROGMEM = {"exp_aft"};
const char HAMin[]               PROGMEM = {"min"};
const char HAMax[]               PROGMEM = {"max"};
const char HAIcon[]              PROGMEM = {"icon"};

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
