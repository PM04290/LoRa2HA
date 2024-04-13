/*
  Designed for ESP32's series
  Radio module : SX1278 (Ra-01 / Ra-02)

  Wifi AP
    - default ssid : lora2ha
    - default password : 12345678
    - default address : 192.164.4.1 (lora2ha0.local)

  TODO
    Ajout carte LilyGO T-Internet POE
    Exemple Somfy RTS ? : https://github.com/rstrouse/ESPSomfy-RTS

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

#define MAX_PACKET 20
packet_version packetTable[MAX_PACKET];
volatile byte idxReadTable = 0;
volatile byte idxWriteTable = 0;

static bool eth_connected = false;

#include "devices.hpp"
#include "network.hpp"

WiFiClient client;
HADevice device;
HAMqtt mqtt(client, device, 50); // 50 devices max

uint32_t curtime, oldtime = 0;
byte ntick = 0;

// Callback for memory problem
void heap_caps_alloc_failed_hook(size_t requested_size, uint32_t caps, const char *function_name)
{
#ifdef DEBUG_SERIAL
  printf("!!! %s failed to allocate %d bytes with 0x%X capabilities. \n", function_name, requested_size, caps);
  printf("heap_caps_get_largest_free_block: %d\n", heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
#endif
}

TimerHandle_t mqttReconnectTimer;

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
  bool result = false;
  noInterrupts();
  if (idxReadTable != idxWriteTable)
  {
    *p = packetTable[idxReadTable];
    //
    idxReadTable++;
    if (idxReadTable >= MAX_PACKET)
    {
      idxReadTable = 0;
    }
    result = true;
  }
  interrupts();
  return result;
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
    DEBUGf("V%d[%d%%] %d <= %d:%d = %d\n", p.version, p.lqi, cp->destinationID, cp->senderID, cp->childID, cp->data.num.value);

    if (logPacket)
    {
      notifyLogPacket(cp, p.lqi);
    }

    if (cp->destinationID == UIDcode) // Only for me
    {
      if (cp->senderID == 254) // if senderID = 254, ping needed
      {
        RLcomm.publishNum(254, UIDcode, 2, p.lqi);
        return;
      }

      Child* ch = hub.getChildById(cp->senderID, cp->childID);
      if (ch != nullptr)
      {
        switch (p.version) {
          case 1:
            ch->doPacketForHA(*cp);
            break;
          default:
            ch->doPacketForHA(*cp);
            break;
        }
      } else {
        if (cp->childID == 0xFF) // config child
        {
          Device* dev = hub.getDeviceById(cp->senderID);
          //
          uint8_t childID = cp->data.config.childID;
          rl_device_t st = (rl_device_t)cp->data.config.deviceType;
          rl_data_t dt = (rl_data_t)cp->data.config.dataType;
          DEBUGf("Conf %d %d %d\n", childID, (int)st, (int)dt);
          String js;
          int d;
          int c;
          if (dev == nullptr)
          {
            d = hub.getNbDevice();
            dev = hub.addDevice(new Device(cp->senderID, ""));
            // new device address
            // add blank device
            docJson.clear();
            docJson["cmd"] = "devnotify";
            docJson["conf_child_" + String(d) + "_"] = getHTMLforDevice(d, dev);
            js = "";
            serializeJson(docJson, js);
            ws.textAll(js);
          }
          d = hub.getIdxDeviceByAddress(cp->senderID);
          Child* chd = dev->getChildById(childID);
          if (chd == nullptr)
          {
            c = dev->getNbChild();
            chd = new Child(dev, childID, "", st, dt, "", CategoryAuto, "");
            dev->addChild(chd);
            // add blank child
            docJson.clear();
            docJson["cmd"] = "childnotify";
            docJson["conf_child_" + String(d) + "_" + String(c)] = getHTMLforChild(d, c, chd);
            js = "";
            serializeJson(docJson, js);
            ws.textAll(js);
          }
        }
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
  esp_err_t error = heap_caps_register_failed_alloc_callback(heap_caps_alloc_failed_hook);
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
    char code = EEPROM.readChar(EEPROM_DATA_CODE);
    if (code >= '0' && code <= '9')
    {
      UIDcode = code - '0';
      AP_ssid[7] = code;
      RadioFreq = EEPROM.readUShort(EEPROM_DATA_FREQ);
      if (RadioFreq == 0xFFFF) RadioFreq = 433;
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

      rstCount = EEPROM.readChar(EEPROM_DATA_COUNT) + 1;
      EEPROM.writeChar(EEPROM_DATA_COUNT, rstCount);
      EEPROM.commit();
      DEBUGf("RST:%d\n", rstCount);
    }
  }

  if (RLcomm.begin(RadioFreq * 1E6, onLoRaReceive, NULL, 20))
  {
    RLcomm.setWaitOnTx(true);
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
    mqtt.onConnected(onMqttConnected);
    mqtt.onDisconnected(onMqttDisconnected);
    if (mqtt.begin(mqtt_host, mqtt_port, mqtt_user, mqtt_pass))
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
  if (curtime > ((oldtime + 1000) | 1))
  {
    oldtime = curtime;
    ntick++;
    if (ntick >= 1800) { // 30min
      // TODO heartbeat
      ntick = 0;
    }
    if (ntick % 10 == 0) { // 10 sec
    }
#ifdef PIN_ETH_LINK
    //eth_allowed = (digitalRead(PIN_ETH_LINK) == LOW);
#endif
  }
  processLoRa();
  mqtt.loop();
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

  // walk device array
  JsonVariant devices = docJson["dev"];
  if (devices.is<JsonArray>())
  {
    for (JsonVariant deviceItem : devices.as<JsonArray>())
    {
      uint8_t address = deviceItem["address"].as<int>();
      const char* name = deviceItem["name"].as<const char*>();
      uint8_t rlversion = deviceItem["rlversion"].as<int>();
      DEBUGf("@ %d : %s\n", address, name);
      if ((address > 0) && (dev = hub.addDevice(new Device(address, name, rlversion))))
      {
        // walk child array
        JsonVariant childs = deviceItem["childs"];
        if (childs.is<JsonArray>())
        {
          for (JsonVariant childItem : childs.as<JsonArray>())
          {
            uint8_t childID = childItem["id"];
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
            float coefA = 1.;
            if (childItem["coefa"].is<float>())
              coefA = childItem["coefa"];
            float coefB = 0.;
            if (childItem["coefb"].is<float>())
              coefB = childItem["coefb"];
            EntityCategory ec = EntityCategory::CategoryAuto;
            if (childItem["category"].is<int>())
              ec = (EntityCategory)childItem["category"].as<int>();
            dev->addChild(new Child(dev, childID, lbl, st, dt, childItem["class"].as<const char*>(), ec, childItem["unit"].as<const char*>(), expire, mini, maxi, coefA, coefB));
          }
        }
      }
    }
  }
  docJson.clear();
  DEBUGln("Config file loaded");
  return true;
}
