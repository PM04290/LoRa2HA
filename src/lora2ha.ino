/*
  Designed for ESP32 Dev Kit
  Radio module : SX1278 (Ra-01)

  Wifi AP
    - default ssid : lora2ha
    - default password : 012345678
    - default address : 192.164.4.1 (lora2ha0.local)
*/
#include <EEPROM.h>
#include "FS.h"
#include "SPIFFS.h"
#include "config.h"
#include <RadioLink.h>
#include <ArduinoJson.h>
#include <HAintegration.h>

#ifndef ESP32
#error Only designed for ESP32
#endif

// LoRa         ESP32     OLIMEX-POE
// pins                   (no SD used)
// SCK  :         18          14
// MISO :         19          15
// MOSI :         23          2
// NSS  :         5           5
// NRST :         4           4
// DIO0 :         2           36

RadioLinkClass RLcomm;

StaticJsonDocument<JSON_MAX_SIZE> docJson;

typedef struct
{
  uint8_t version;
  int lqi;
  union {
    rl_packet_t current;
    rl_packetV1_t v1;
    rl_packetV2_t v2;
  } packets;
} packet_version;

#include "devices.hpp"
#include "network.hpp"

WiFiClient client;
HADevice device;
HAMqtt mqtt(client, device, 12); // 12 trigger MAX

uint32_t curtime, oldtime = 0;
byte ntick = 0;

#define MAX_PACKET 10
packet_version packetTable[MAX_PACKET];
byte idxReadTable = 0;
byte idxWriteTable = 0;

void onMqttMessage(const char* topic, const uint8_t* payload, uint16_t length)
{
  char* pl = (char*)payload;
  pl[length] = 0;
}

void onMqttConnected()
{
  DEBUGln("Mqtt connected");
}

void onMqttDisconnected()
{
  DEBUGln("Mqtt disconnected");
}

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
  if (p->destinationID == 0)
  {
    packetTable[idxWriteTable].packets.current = *p;
    packetTable[idxWriteTable].version = 3;
    packetTable[idxWriteTable].lqi = RLcomm.lqi();
    if (len == (MAX_PACKET_DATA_LEN_V1 + 4)) packetTable[idxWriteTable].version = 1;
    if (len == (MAX_PACKET_DATA_LEN_V2 + 4)) packetTable[idxWriteTable].version = 2;
    idxWriteTable++;
    if (idxWriteTable >= MAX_PACKET)
    {
      idxWriteTable = 0;
    }
    if (idxWriteTable == idxReadTable)
    {
      idxReadTable++;
      if (idxReadTable >= MAX_PACKET)
      {
        idxReadTable = 0;
      }
    }
  }
}

void processDevice()
{
  packet_version p;
  if (getPacket(&p))
  {
    rl_packet_t* cp = &p.packets.current;
    DEBUGf("V%d[%d%%] %d <= %d:%d = %d\n", p.version, p.lqi, cp->destinationID, cp->senderID, cp->childID, cp->data.num.value);
    if (logPacket)
    {
      notifyLogPacket(&p.packets.current);
    }
    Child* ch = hub.getChildById(p.packets.current.senderID, p.packets.current.childID);
    if (ch != nullptr)
    {
      if (p.version == 1)
      {
        ch->doPacketV1ForHA(p.packets.v1);
      } else if (p.version == 2)
      {
        ch->doPacketV2ForHA(p.packets.v2);
      } else
      {
        ch->doPacketForHA(p.packets.current);
      }
    }
  }
}

void setup()
{
  Serial.begin(115200);
  while (Serial.availableForWrite() == false)
  {
    delay(50);
  }
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
    /* to initialize EEPROM with config define
          EEPROM.writeChar(0, AP_ssid[7]);
          EEPROM.writeString(EEPROM_TEXT_OFFSET, Wifi_ssid);
          EEPROM.writeString(EEPROM_TEXT_OFFSET + (EEPROM_TEXT_SIZE * 1), Wifi_pass);
          EEPROM.writeString(EEPROM_TEXT_OFFSET + (EEPROM_TEXT_SIZE * 2), mqtt_host);
          EEPROM.writeString(EEPROM_TEXT_OFFSET + (EEPROM_TEXT_SIZE * 3), mqtt_user);
          EEPROM.writeString(EEPROM_TEXT_OFFSET + (EEPROM_TEXT_SIZE * 4), mqtt_pass);
          EEPROM.commit();
    */
    char code = EEPROM.readChar(0);
    if (code >= '0' && code <= '9')
    {
      AP_ssid[7] = code;
      strcpy(Wifi_ssid, EEPROM.readString(EEPROM_TEXT_OFFSET).c_str());
      strcpy(Wifi_pass, EEPROM.readString(EEPROM_TEXT_OFFSET + EEPROM_TEXT_SIZE * 1).c_str());
      strcpy(mqtt_host, EEPROM.readString(EEPROM_TEXT_OFFSET + EEPROM_TEXT_SIZE * 2).c_str());
      strcpy(mqtt_user, EEPROM.readString(EEPROM_TEXT_OFFSET + EEPROM_TEXT_SIZE * 3).c_str());
      strcpy(mqtt_pass, EEPROM.readString(EEPROM_TEXT_OFFSET + EEPROM_TEXT_SIZE * 4).c_str());

      DEBUGln(AP_ssid);
      DEBUGln(Wifi_ssid);
      DEBUGln(Wifi_pass);
      DEBUGln(mqtt_host);
      DEBUGln(mqtt_user);
      DEBUGln(mqtt_pass);
    }
  }

  if (RLcomm.begin(433E6, onLoRaReceive, NULL, 17))
  {
    DEBUGln("LORA OK");
  } else {
    DEBUGln("LORA ERROR");
  }
 
  initNetwork();
  initWeb();

  if (loadConfig())
  {
    byte mac[6];
    WiFi.macAddress(mac);
    // HA
    device.setUniqueId(mac, sizeof(mac));
    device.setManufacturer("M&L");
    device.setModel("LoRa2HA");
    device.setName(AP_ssid);
    device.setSoftwareVersion(VERSION);

    mqtt.setDataPrefix("lora2ha");
    mqtt.onMessage(onMqttMessage);
    mqtt.onConnected(onMqttConnected);
    mqtt.onDisconnected(onMqttDisconnected);
    if (mqtt.begin(mqtt_host, mqtt_user, mqtt_pass))
    {
      DEBUGln("Mqtt ready");
    } else {
      DEBUGln("Mqtt error");
    }
  }
}

void loop()
{
  curtime = millis();
  if (curtime > oldtime + 60000 || curtime < oldtime)
  {
    ntick++;

    if (ntick >= 30) {
      // TODO heartbeat
      ntick = 0;
    }
    oldtime = curtime;
  }
  mqtt.loop();
  processDevice();
}

bool loadConfig()
{
  DEBUGln("Loading config file");
  File file = SPIFFS.open("/config.json");
  if (!file || file.isDirectory())
  {
    DEBUGln("failed to open config file");
    return false;
  }
  Device* dev;

  DeserializationError error = deserializeJson(docJson, file);
  file.close();
  if (error)
  {
    DEBUGln("failed to deserialize config file");
    return false;
  }
  uniqueid = docJson["uniqueid"].as<String>();
  version_major = docJson["version_major"].as<String>();
  version_minor = docJson["version_minor"].as<String>();

  // walk device array
  JsonVariant devices = docJson["dev"];
  if (devices.is<JsonArray>())
  {
    for (JsonVariant deviceItem : devices.as<JsonArray>())
    {
      uint8_t address = deviceItem["address"].as<int>();
      const char* name = deviceItem["name"].as<const char*>();
      if ((address > 0) && (dev = hub.addDevice(new Device(address, name))))
      {
        // walk child array
        JsonVariant childs = deviceItem["childs"];
        if (childs.is<JsonArray>())
        {
          for (JsonVariant childItem : childs.as<JsonArray>())
          {
            uint8_t id = childItem["id"];
            const char* lbl = childItem["label"].as<const char*>();
            rl_device_t st = (rl_device_t)childItem["sensortype"].as<int>();
            rl_data_t dt = (rl_data_t)childItem["datatype"].as<int>();
            dev->addChild(new Child(dev->getName(), address, id, lbl, st, dt, childItem["class"].as<const char*>(), childItem["unit"].as<const char*>()));
          }
        }
      }
    }
  }
  docJson.clear();
  DEBUGln("Config file loaded");
  return true;
}
