/*
  Designed for ESP32's series
  Radio module : SX1278 (Ra-01 / Ra-02)

  Wifi AP
    - default ssid : lora2ha
    - default password : 012345678
    - default address : 192.164.4.1 (lora2ha0.local)

  TODO	
    Ajout carte LilyGO T-Internet POE
    Exemple Somfy RTS : https://github.com/rstrouse/ESPSomfy-RTS
	
*/
#include <EEPROM.h>
#include "FS.h"
#include "SPIFFS.h"
#include "config.h"
#include <RadioLink.h>
#define ARDUINOJSON_ENABLE_NAN 1
#include <ArduinoJson.h>
#include <HAintegration.h>

#ifndef ESP32
#error Only designed for ESP32
#endif

//      PCB      MLH1          MLH2            MLH3         MLH4
// LoRa       Lolin ESP32   OLIMEX-POE      WT32-ETH01     ESP32
// pins        (S2 Mini)                                  MiniKit
// SCK             7            14              14           18
// MISO            9            15              15           19
// MOSI           11             2               2           23
// NSS             5             5              12            5
// NRST           12             4              4             4
// DIO0            3            36              35            2

RadioLinkClass RLcomm;

JsonDocument docJson;

typedef struct
{
  uint8_t version;
  int lqi;
  rl_packets packets;
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
    packetTable[idxWriteTable].version = 4;
    packetTable[idxWriteTable].lqi = RLcomm.lqi();
    if (len == RL_PACKETV1_SIZE) packetTable[idxWriteTable].version = 1;
    if (len == RL_PACKETV2_SIZE) packetTable[idxWriteTable].version = 2;
    if (len == RL_PACKETV3_SIZE) packetTable[idxWriteTable].version = 3;
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
      notifyLogPacket(&p.packets.current, p.lqi);
    }
	/* for ping testing
    if (p.packets.current.senderID == 99)
    {
      RLcomm.publishNum(99, 0, 2, S_NUMERICSENSOR, p.lqi);
      delay(100);
      return;
    }
	*/
    Child* ch = hub.getChildById(p.packets.current.senderID, p.packets.current.childID);
    if (ch != nullptr)
    {
      if (p.version == 1)
      {
        ch->doPacketV1ForHA(p.packets.v1);
      } else if (p.version == 2)
      {
        ch->doPacketV2ForHA(p.packets.v2);
      } else if (p.version == 3)
      {
        ch->doPacketV3ForHA(p.packets.v3);
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
    /* to initialize EEPROM with config.h defines
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
      UIDcode = code - '0';
      AP_ssid[7] = code;
      RadioFreq = EEPROM.readUShort(1);
      if (RadioFreq == 0xFFFF) RadioFreq = 435;
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
      DEBUGln(RadioFreq);
    }
  }

  if (RLcomm.begin(RadioFreq * 1E6, onLoRaReceive, NULL, 20))
  {
    DEBUGf("LoRa ok at %dMHz\n", RadioFreq);
  } else {
    DEBUGln("LoRa ERROR");
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
#if defined(ARDUINO_LOLIN_S2_MINI)
    device.setModel("LoRa2HA (MLH1)");
#elif defined(ARDUINO_ESP32_POE_ISO)
    device.setModel("LoRa2HA (MLH2)");
#elif defined(ARDUINO_WT32_ETH01)
    device.setModel("LoRa2HA (MLH3)");
#elif defined(ARDUINO_D1_MINI32)
    device.setModel("LoRa2HA (MLH4)");
#else
    device.setModel("LoRa2HA");
#endif
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
  if (curtime > oldtime + 1000 || curtime < oldtime)
  {
    ntick++;
    if (ntick >= 1800) { // 30min
      // TODO heartbeat
      ntick = 0;
    }
    oldtime = curtime;
  }
  mqtt.loop();
  processDevice();

#ifdef USE_ETHERNET
#ifdef PIN_ETH_LINK
  // detect Ethernet connected
  if ((eth_allowed == false) && (digitalRead(PIN_ETH_LINK) == LOW))
  {
    eth_allowed = true;
    ESP.restart(); // TODO
  }
  // detect Ethernet disconnected
  if ((eth_allowed == true) && (digitalRead(PIN_ETH_LINK) == HIGH))
  {
    eth_allowed = false;
    ESP.restart(); // TODO
  }
#endif
#endif
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
      DEBUGf("@ %d : %s\n", address, name);
      uint8_t rlversion = deviceItem["rlversion"].as<int>();
      if ((address > 0) && (dev = hub.addDevice(new Device(address, name, rlversion))))
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
            int expire = childItem["expire"] | 0L;
            int32_t mini(LONG_MIN);
            if (childItem["min"].is<int>())
              mini = childItem["min"].as<int>();
            int32_t maxi(LONG_MAX);
            if (childItem["max"].is<int>())
              maxi = childItem["max"].as<int>();
            float coefA = childItem["coefa"] | (float)1.0;
            float coefB = childItem["coefb"] | (float)0.0;
            dev->addChild(new Child(dev, address, id, lbl, st, dt, childItem["class"].as<const char*>(), childItem["unit"].as<const char*>(), expire, mini, maxi, coefA, coefB));
          }
        }
      }
    }
  }
  docJson.clear();
  DEBUGln("Config file loaded");
  return true;
}
