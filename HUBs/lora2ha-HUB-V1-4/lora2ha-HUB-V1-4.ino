/*
  Designed for ESP32's series
  Radio module : SX1278 (Ra-01 / Ra-02)

  Wifi AP
    - default ssid : lora2ha
    - default password : 12345678
    - default address : 192.164.4.1 (lora2ha0.local)

*/
#include <EEPROM.h> // native library
#include <FS.h>     // native library
#include <SPIFFS.h> // native library

// needed for ArduinoJson library
#define ARDUINOJSON_ENABLE_NAN 1
#include <ArduinoJson.h>         // available in library manager

#include <ESPAsyncWebServer.h>   // https://github.com/me-no-dev/ESPAsyncWebServer (don't get fork in library manager)
#include "config.h"              // inside file
#include "src/datetime.h"        // inside file
#include <RadioLink.h>           // GitHub project : https://github.com/PM04290/RadioLink

#ifndef ESP32
#error Only designed for ESP32
#endif

//      PCB      MLH1          MLH2            MLH3         MLH4         MLH5
// LoRa pin    Lolin ESP32   OLIMEX-POE      WT32-ETH01     ESP32        TTGO
//             (S2 Mini)                                   D1 Mini     LoRa 1.6.1
// SCK             7            14              14           18           5
// MISO            9            15              15           19          19
// MOSI           11             2               2           23          27
// NSS             5             5              12            5          18
// NRST           12             4              4             4          23
// DIO0            3            36              35            2          26

RadioLinkClass RLcomm;
bool loraOK;
JsonDocument docJson;

// UPDI
String needUploadFilename = "";

// for rtc
DateTime rtc = DateTime(2020, 1, 1, 0, 0, 0);
volatile bool newRTC = false;
portMUX_TYPE ntpMux = portMUX_INITIALIZER_UNLOCKED;

// for availability
bool needPublishOnline = true;

// buffer table for LoRa incoming packet
#define MAX_PACKET 20
typedef struct __attribute__((packed))
{
  uint8_t version;
  int lqi;
  rl_packets packets;
} packet_version;
packet_version packetTable[MAX_PACKET];
volatile byte idxReadTable = 0;
volatile byte idxWriteTable = 0;

// extern function after hub.hpp
void MQTTcallback(char* topic, byte* payload, unsigned int length);
void notifyDeviceForm(uint8_t d);
void notifyDevicePairing(uint8_t d);
void notifyDeviceHeader(uint8_t d);
void notifyChildLine(uint8_t d, uint8_t c);
void TaskUploadModule(void *pvParameters);

#include "hub.hpp"       // inside file
#include "network.hpp"   // inside file
#include "webserver.hpp" // inside file

// for 1 Second interrup
hw_timer_t* secTimer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
volatile bool topSecond = false;

// for NTP update
TaskHandle_t xHandleNTP = NULL;

#ifdef UPDI_TX
// for Upload
#include "src/AVRProg.h"   // inside file
AVRProg avrProg = AVRProg();
#endif

// Callback for memory problem
void heap_caps_alloc_failed_hook(size_t requested_size, uint32_t caps, const char *function_name)
{
#ifdef DEBUG_SERIAL
  printf("!!! %s failed to allocate %d bytes with 0x%X capabilities. \n", function_name, requested_size, caps);
  printf("heap_caps_get_largest_free_block: %d\n", heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
#endif
}

void IRAM_ATTR onTimer()
{
  portENTER_CRITICAL_ISR(&timerMux);
  topSecond = true;
  portEXIT_CRITICAL_ISR(&timerMux);
}

void TaskNTP(void *pvParameters)
{
  DateTime* _rtc = (DateTime*)pvParameters;
  const TickType_t xDelay = 2000 / portTICK_PERIOD_MS;
  configTzTime(datetimeTZ, datetimeNTP, defaultNTP);
  while (true)
  {
    struct tm timeinfo;
    uint8_t retry = DEFAULT_RETRY_NTP;
    while (retry > 0) {
      if (getLocalTime(&timeinfo))
      {
        timeinfo.tm_year += 1900;
        timeinfo.tm_mon += 1;
        DEBUGf("NTP:%04d-%02d-%02d %02d:%02d:%02d\n", timeinfo.tm_year, timeinfo.tm_mon, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
        portENTER_CRITICAL_ISR(&ntpMux);
        *_rtc = DateTime(timeinfo.tm_year, timeinfo.tm_mon, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
        newRTC = true;
        portEXIT_CRITICAL_ISR(&ntpMux);
        retry = 0;
      } else {
        retry--;
        DEBUGf("NTP:no time yet %d/10\n", DEFAULT_RETRY_NTP - retry);
        vTaskDelay( xDelay );
      }
    }
    vTaskSuspend( NULL );
  }
}

#ifdef UPDI_TX
void TaskUploadFirmware(void *pvParameters)
{
}
#endif

//TODO Bouton Reset dans H.A

bool getPacket(packet_version* p)
{
  if (idxReadTable != idxWriteTable)
  {
    *p = packetTable[idxReadTable];
    //
    idxReadTable++;
    if (idxReadTable >= MAX_PACKET)
    {
      idxReadTable = 0;
    }
    return true;
  }
  return false;
}

void onLoRaReceive(uint8_t len, rl_packet_t* p)
{
  noInterrupts();
  memcpy(&packetTable[idxWriteTable].packets.current, p, len);
  packetTable[idxWriteTable].version = 1;
  packetTable[idxWriteTable].lqi = RLcomm.lqi();
  if (len == RL_PACKETV1_SIZE) packetTable[idxWriteTable].version = 1;
  idxWriteTable++;
  if (idxWriteTable >= MAX_PACKET)
  {
    idxWriteTable = 0;
  }
  if (idxWriteTable == idxReadTable) // overload
  {
    idxReadTable++;
    if (idxReadTable >= MAX_PACKET)
    {
      idxReadTable = 0;
    }
  }
  interrupts();
}

void processLoRa()
{
  packet_version p;
  if (getPacket(&p))
  {
    rl_packet_t* cp = &p.packets.current;

    if (cp->childID == RL_ID_CONFIG && (rl_element_t)(cp->sensordataType >> 3) == E_CONFIG)
    {
      rl_conf_t cnfIdx = (rl_conf_t)(cp->sensordataType & 0x07);
      rl_configs_t* cnf = &cp->data.configs;
      uint8_t child = cnf->base.childID;
      switch (cnfIdx) {
        case C_BASE:
          DEBUGf("V%d[%d%%] %d <= %d:%d B = %s\n", p.version, p.lqi, cp->destinationID, cp->senderID, child, cnf->base.name);
          break;
        case C_UNIT:
          DEBUGf("V%d[%d%%] %d <= %d:%d U = %s\n", p.version, p.lqi, cp->destinationID, cp->senderID, child, cnf->text.text);
          break;
        case C_OPTS:
          DEBUGf("V%d[%d%%] %d <= %d:%d O = %s\n", p.version, p.lqi, cp->destinationID, cp->senderID, child, cnf->text.text);
          break;
        case C_NUMS:
          DEBUGf("V%d[%d%%] %d <= %d:%d N = %d %d\n", p.version, p.lqi, cp->destinationID, cp->senderID, child, cnf->nums.mini, cnf->nums.maxi);
          break;
        case C_END:
          DEBUGf("V%d[%d%%] %d <= %d:%d E\n", p.version, p.lqi, cp->destinationID, cp->senderID, child);
          break;
        default:
          DEBUGf("V%d[%d%%] %d <= %d:%d ? = %d\n", p.version, p.lqi, cp->destinationID, cp->senderID, child, cp->data.num.value);
          break;
      }
    } else {
      rl_data_t dt = (rl_data_t)(cp->sensordataType & 0x07);
      switch (dt) {
        case D_TEXT:
          DEBUGf("V%d[%d%%] %d <= %d:%d = %s\n", p.version, p.lqi, cp->destinationID, cp->senderID, cp->childID, cp->data.text);
          break;
        case D_FLOAT:
          DEBUGf("V%d[%d%%] %d <= %d:%d = %f\n", p.version, p.lqi, cp->destinationID, cp->senderID, cp->childID, float(cp->data.num.value) / float(cp->data.num.divider));
          break;
        default:
          DEBUGf("V%d[%d%%] %d <= %d:%d = %d\n", p.version, p.lqi, cp->destinationID, cp->senderID, cp->childID, cp->data.num.value);
          break;
      }
    }

    if (logPacket)
    { // log packet requested by client web page
      notifyLogPacket(cp, p.lqi);
    }

    if (cp->destinationID == UIDcode || cp->destinationID == RL_ID_BROADCAST)
    {
      if (cp->senderID == RL_ID_PING)
      { // if senderID = 254, ping needed
        RLcomm.publishNum(RL_ID_PING, cp->destinationID, RL_ID_PING, p.lqi);
        return;
      }
      if (cp->senderID >= 10 && cp->childID == RL_ID_DATETIME)
      { // from every sender module, need Datetime
        hub.processDateTime(cp->senderID);
        return;
      }
      if (pairingActive && cp->childID == RL_ID_CONFIG && (rl_element_t)(cp->sensordataType >> 3) == E_CONFIG)
      { // config packet
        hub.pairingConfig(cp);
        return;
      }
    }

    if (cp->destinationID == UIDcode)
    { // Only for me
      CDevice* dev = hub.getDeviceByAddress(cp->senderID);
      CEntity* ent = hub.getEntityById(cp->senderID, cp->childID);
      if (ent)
      { // process existing device
        switch (p.version) {
          case 1:
            ent->doPacketForHA(*cp);
            break;
          default:
            ent->doPacketForHA(*cp);
            break;
        }
        if (dev) {
          dev->setLQI(p.lqi);
        }
      } else
      {
        DEBUGf("Unknown configuration for sender %d child %d\n", cp->senderID, cp->childID);
      }
    }
  }
}

void MQTTcallback(char* topic, byte* payload, unsigned int length)
{
  char p[length + 1];
  strncpy(p, (char*)payload, length);
  p[length] = 0;
  hub.onMessageMQTT(topic, p);
}

void setup()
{
#ifdef DEBUG_SERIAL
  Serial.begin(115200);
  while (!Serial.availableForWrite()) delay(10);
#endif
  heap_caps_register_failed_alloc_callback(heap_caps_alloc_failed_hook);
  DEBUGln("Start debug");

  if (!SPIFFS.begin())
  {
    DEBUGln(F("SPIFFS Mount failed"));
  } else
  {
    DEBUGln(F("SPIFFS Mount succesfull"));
    listDir("/");
  }
  delay(50);

  if (!EEPROM.begin(EEPROM_MAX_SIZE))
  {
    DEBUGln("failed to initialise EEPROM");
  } else {
    /* to initialize EEPROM with config.h defines
          EEPROM.writeChar(EEPROM_DATA_CODE, AP_ssid[7]);
          EEPROM.writeUshort(EEPROM_DATA_FREQ, 433);
          EEPROM.writeByte(EEPROM_DATA_RANGE, 1);
          EEPROM.writeByte(EEPROM_DATA_NVPN, 1); // VPN off
          EEPROM.writeByte(EEPROM_DATA_WDOG, 10);
          EEPROM.writeString(EEPROM_TEXT_OFFSET, Wifi_ssid);
          EEPROM.writeString(EEPROM_TEXT_OFFSET + (EEPROM_TEXT_SIZE * 1), Wifi_pass);
          EEPROM.writeString(EEPROM_TEXT_OFFSET + (EEPROM_TEXT_SIZE * 2), mqtt_host);
          EEPROM.writeString(EEPROM_TEXT_OFFSET + (EEPROM_TEXT_SIZE * 3), mqtt_user);
          EEPROM.writeString(EEPROM_TEXT_OFFSET + (EEPROM_TEXT_SIZE * 4), mqtt_pass);
          EEPROM.writeString(EEPROM_TEXT_OFFSET + (EEPROM_TEXT_SIZE * 5), datetimeTZ);
          EEPROM.writeString(EEPROM_TEXT_OFFSET + (EEPROM_TEXT_SIZE * 6), datetimeNTP);
          EEPROM.commit();
    */
    char code = EEPROM.readChar(EEPROM_DATA_CODE);
    if (code >= '0' && code <= '9')
    {
      UIDcode = code - '0';
      AP_ssid[strlen(AP_ssid) - 1] = code;
      RadioFreq = EEPROM.readUShort(EEPROM_DATA_FREQ);
      if (RadioFreq == 0xFFFF) RadioFreq = 433;
      RadioRange = EEPROM.readByte(EEPROM_DATA_RANGE) % 4;
      vpnNeeded = (EEPROM.readByte(EEPROM_DATA_NVPN) % 2) == 0;
      Watchdog = EEPROM.readByte(EEPROM_DATA_WDOG) % 60;
      strcpy(Wifi_ssid, EEPROM.readString(EEPROM_TEXT_OFFSET).c_str());
      strcpy(Wifi_pass, EEPROM.readString(EEPROM_TEXT_OFFSET + EEPROM_TEXT_SIZE * 1).c_str());
      strcpy(mqtt_host, EEPROM.readString(EEPROM_TEXT_OFFSET + EEPROM_TEXT_SIZE * 2).c_str());
      strcpy(mqtt_user, EEPROM.readString(EEPROM_TEXT_OFFSET + EEPROM_TEXT_SIZE * 3).c_str());
      strcpy(mqtt_pass, EEPROM.readString(EEPROM_TEXT_OFFSET + EEPROM_TEXT_SIZE * 4).c_str());
      strcpy(datetimeTZ, EEPROM.readString(EEPROM_TEXT_OFFSET + EEPROM_TEXT_SIZE * 5).c_str());
      strcpy(datetimeNTP, EEPROM.readString(EEPROM_TEXT_OFFSET + EEPROM_TEXT_SIZE * 6).c_str());

      DEBUGln(AP_ssid);
      DEBUGf("Freq:%d\n", RadioFreq);
      DEBUGf("Range:%d\n", RadioRange);
      DEBUGf("VPN:%d\n", vpnNeeded);
      DEBUGf("Wd:%d\n", Watchdog);
      DEBUGln(Wifi_ssid);
      DEBUGln(Wifi_pass);
      DEBUGln(mqtt_host);
      DEBUGln(mqtt_user);
      DEBUGln(mqtt_pass);
      DEBUGln(datetimeTZ);
      DEBUGln(datetimeNTP);
    }
  }

  // Radio range 3 = 400m/1484 ms
  //             2 = 200m/660 ms
  //             1 = 100m/186 ms
  //             0 = 50m/27 ms
  loraOK = RLcomm.begin(RadioFreq * 1E6, onLoRaReceive, NULL, 20, RadioRange);
  if (loraOK)
  {
    RLcomm.setWaitOnTx(true);
    DEBUGf("LoRa ok at %dMHz (range %d)\n", RadioFreq, RadioRange);
  } else {
    DEBUGln("LoRa ERROR");
  }

  initNetwork();
  initWeb();

  if (hub.loadConfig())
  {
  }

  // define time ISR
  secTimer = timerBegin(0, 80, true);
  timerAttachInterrupt(secTimer, &onTimer, true);
  timerAlarmWrite(secTimer, 1000000, true);
  timerAlarmEnable(secTimer);
  //
  DEBUGf("heap_caps_get_largest_free_block: %d\n", heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));

  xTaskCreate( TaskNTP, "getNTP", 2048, (void*)&rtc, tskIDLE_PRIORITY, &xHandleNTP );

#ifdef UPDI_TX
  avrProg.setUPDI(&Serial2, 115200, UPDI_RX, UPDI_TX); // (RX, TX) can also try 230400
#endif
}

void loop()
{
  static uint32_t ntick = 0;
  bool doWatchDog = false;
  bool do24Hour = false;
  if (topSecond)
  {
    portENTER_CRITICAL(&timerMux);
    topSecond = false;
    portEXIT_CRITICAL(&timerMux);
    //
    uint32_t t = rtc.unixtime() + 1;
    portENTER_CRITICAL_ISR(&ntpMux);
    rtc = t;
    portEXIT_CRITICAL_ISR(&ntpMux);
    //DEBUGf("%02d:%02d:%02d\n", rtc.hour(), rtc.minute(), rtc.second());
    //
    ntick++;
    doWatchDog = ((ntick % (Watchdog * 60)) == 0);
    if (ntick >= 86400) { // 1 day
      ntick = 0;
      do24Hour = true;
    }
  }
  if (doWatchDog) {
    // must publish "Online" every 10 minutes
    DEBUGln("Need publish Online");
    needPublishOnline = true;
  }
  if (do24Hour) {
    // get NTP time every days (re-activate Task)
    vTaskResume( xHandleNTP );
  }
  if (newRTC) {
    // when NTP updated, process it
    hub.processDateTime(RL_ID_BROADCAST);
    portENTER_CRITICAL_ISR(&ntpMux);
    newRTC = false;
    portEXIT_CRITICAL_ISR(&ntpMux);
  }

  // Upload firmware into target ATtiny
  if (needUploadFilename != "")
  {
#ifdef UPDI_TX
    String msg = "Error";
    //
    DEBUGf("Start upload\n");
    if (avrProg.targetPower(true))
    {
      uint16_t signature = avrProg.readSignature();
      if (signature != 0 && signature != 0xFFFF)
      {
        DEBUGf("Signature %s\n", avrProg.g_updi.device->shortname);
        DEBUGln("Erasing target");
        if (avrProg.eraseChip())
        {
          DEBUGln(needUploadFilename);
          if (needUploadFilename.startsWith("1;"))
          { // from Github (create temporary file)
            String url = "https://raw.githubusercontent.com/PM04290/LoRa2HA/main/firmware/" + getValue(needUploadFilename, ';', 1);
            needUploadFilename = "/tmp.hex";
            downloadFile(url, needUploadFilename);
          } else
          { // from SPIFFS
            needUploadFilename = "/hex/" + getValue(needUploadFilename, ';', 1);
          }
          DEBUGln(needUploadFilename);
          int nbc = 0;
          if ((nbc = avrProg.HEXfileToImage(needUploadFilename.c_str())) > 0)
          {
            DEBUGf("%d bytes to send\n", nbc);
            if (avrProg.flashImage(512, nbc))
            {
              msg = "Succefull upload done";
              DEBUGln(msg);
            } else {
              msg = "Fail to write target flash";
              DEBUGln(msg);
            }
          }
          SPIFFS.remove("/tmp.hex");
        } else
        {
          msg = "Failed to erase flash";
          DEBUGln(msg);
        }
        avrProg.targetPower(false);
      } else
      {
        msg = "No target attached";
        DEBUGln(msg);
      }
    } else
    {
      msg = "Failed to connect to target";
      DEBUGln(msg);
    }
    needUploadFilename = "";
    notifyUploadMessage(msg);

#else
    notifyUploadMessage("Upload not allowed on this HUB model");
#endif
  }
  processLoRa();
  hub.processMQTT();
}
