#pragma once
#include <PubSubClient.h> // available in library manager

WiFiClient wifimqttclient;
PubSubClient mqtt(wifimqttclient);

const char HexMap[] PROGMEM = {"0123456789abcdef"};

boolean isValidFloat(String str)
{
  if (str.startsWith("-"))
    str.remove(0, 1);
  for (byte i = 0; i < str.length(); i++)
  {
    if (!isDigit(str.charAt(i)) && (str.charAt(i) != '.'))
    {
      return false;
    }
  }
  return str.length() > 0;
}

boolean isValidInt(String str)
{
  if (str.startsWith("-"))
    str.remove(0, 1);
  for (byte i = 0; i < str.length(); i++)
  {
    if (!isDigit(str.charAt(i)))
    {
      return false;
    }
  }
  return str.length() > 0;
}

void byteArrayToStr(char* dst, const byte* src, const uint16_t length)
{
  for (uint8_t i = 0; i < length; i++) {
    dst[i * 2] = pgm_read_byte(&HexMap[((char)src[i] & 0XF0) >> 4]);
    dst[i * 2 + 1] = pgm_read_byte(&HexMap[((char)src[i] & 0x0F)]);
  }
  dst[length * 2] = 0;
}
char* byteArrayToStr(const byte* src, const uint16_t length)
{
  char* dst = new char[(length * 2) + 1]; // include null terminator
  byteArrayToStr(dst, src, length);
  return dst;
}

String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = { 0, -1 };
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++)
  {
    if (data.charAt(i) == separator || i == maxIndex)
    {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }
  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

#include "hubdevice.hpp"

class CHub {
  public:
    explicit CHub()
    {
      _deviceFirst = nullptr;
      _oldMQTTconnected = false;
    }
    CDevice* newDevice(JsonVariant JSconf)
    {
      CDevice* newdev = new CDevice(JSconf);
      if (_deviceFirst == nullptr)
      {
        _deviceFirst = newdev;
      } else
      {
        CDevice* dev = _deviceFirst;
        while (dev)
        {
          if (dev->Next() == nullptr)
          {
            dev->setNext(newdev);
            return newdev;
          }
          dev = dev->Next();
        }
      }
      return newdev;
    }
    void delDevice(uint8_t address)
    {
      CDevice* dev = getDeviceByAddress(address);
      if (dev)
      {
        if (dev == _deviceFirst)
        {
          _deviceFirst = dev->Next();
        } else
        {
          CDevice* wDev = _deviceFirst;
          while (wDev && (wDev->Next() != dev))
          {
            wDev = wDev->Next();
          }
          if (wDev)
          {
            wDev->setNext(dev->Next());
          }
        }
        delete dev;
        //
        JsonArray devs = _docJson["dev"];
        for (JsonArray::iterator it = devs.begin(); it != devs.end(); ++it)
        {
          if ((*it)["address"] == address) {
            devs.remove(it);
          }
        }
      }
      saveConfig();
    }
    CDevice* getDeviceByAddress(uint8_t address)
    {
      CDevice* dev = _deviceFirst;
      while (dev)
      {
        if (dev->getAddress() == address)
        {
          return dev;
        }
        dev = dev->Next();
      }
      return nullptr;
    }
    CEntity* getEntityById(uint8_t address, uint8_t id)
    {
      CDevice* dev = getDeviceByAddress(address);
      if (dev)
      {
        return dev->getEntityById(id);
      }
      return nullptr;
    }
    CDevice* walkDevice(CDevice* dev)
    {
      if (dev) {
        return dev->Next();
      }
      return _deviceFirst;
    }
    bool endPairing(uint8_t address)
    {
      CDevice* dev = getDeviceByAddress(address);
      if (dev)
      {
        rl_configParam_t cnfp;
        memset(&cnfp, 0, sizeof(cnfp));
        RLcomm.publishConfig(address, UIDcode, (rl_configs_t*)&cnfp, C_END);
        dev->setPairing(false);
        return true;
      }
      return false;
    }
    void MQTTconnect()
    {
      DEBUGf("MQTT start connect(%s,%s,%s)\n", AP_ssid, mqtt_user, mqtt_pass);
      mqtt.setServer(mqtt_host, mqtt_port);
      mqtt.setCallback(MQTTcallback);
      mqtt.setBufferSize(1024);
      if (mqtt.connect(AP_ssid, mqtt_user, mqtt_pass))
      {
        DEBUGf("MQTT connect\n");
      } else {
        DEBUGf("MQTT NOT connected to %s:%d [%d]\n", mqtt_host, mqtt_port, mqtt.state());
      }
    }
    bool MQTTreconnect()
    {
      if (mqtt.connect(AP_ssid, mqtt_user, mqtt_pass))
      {
        DEBUGf("MQTT reconnect\n");
        return true;
      } else {
        DEBUGf("MQTT fail to reconnected to %s:%d [%d]\n", mqtt_host, mqtt_port, mqtt.state());
      }
      return false;
    }
    void processMQTT()
    {
      static uint32_t lastTimeRetry = 0;
      if (mqtt.connected())
      {
        if (mqtt.loop())
        {
          if (!_oldMQTTconnected)
          {
            DEBUGln("MQTT connected");
            _oldMQTTconnected = true;
            publishConfig();
          }
          if (needPublishOnline) {
            publishOnline();
          }
        }
      } else {
        if (_oldMQTTconnected)
        {
          DEBUGln("MQTT disconnected");
          _oldMQTTconnected = false;
          needPublishOnline = true;
        }
        uint32_t now = millis();
        if (now - lastTimeRetry > 5000)
        {
          lastTimeRetry = now;
          if (MQTTreconnect())
          {
            lastTimeRetry = 0;
          }
        }
      }
    }
    void onMessageMQTT(char* topic, char* payload)
    {
      if (String(topic).endsWith("/hub/watchdog/command"))
      {
        Watchdog = max(1, atoi(payload) % 60);
        EEPROM.writeByte(EEPROM_DATA_WDOG, Watchdog);
        EEPROM.commit();
        publishConfigAvail();
        publishWatchdog();
        needPublishOnline = true;
        return;
      }
      CDevice* dev = nullptr;
      while ((dev = walkDevice(dev)))
      {
        dev->onMessageMQTT(topic, payload);
      }
    }
    void processDateTime(uint8_t destination)
    {
      if (destination == RL_ID_BROADCAST && !newRTC) return;
      DEBUGf("broadcast DateTime\n");
      uint8_t raw[MAX_PACKET_DATA_LEN];
      memset(raw, 0, sizeof(raw));
      raw[0] = rtc.year() - 2000;
      raw[1] = rtc.month();
      raw[2] = rtc.day();
      raw[3] = rtc.hour();
      raw[4] = rtc.minute();
      raw[5] = rtc.second();
      RLcomm.publishRaw(destination, UIDcode, RL_ID_DATETIME, raw, MAX_PACKET_DATA_LEN);
    }
    void publishConfigAvail()
    {
      String payload;
      String topic;
      size_t Lres;
      // HUB configuration
      JsonDocument docJSon;
      JsonObject Jconfig = docJSon.to<JsonObject>();
      JsonObject device = Jconfig["device"].to<JsonObject>();
      device = fillPublishConfigDevice(device);

      Jconfig[HAObjectID] =  String(AP_ssid) + "_hub_connection_state";
      Jconfig[HAUniqueID] =  String(HAuniqueId) + "_hub_connection_state";
      Jconfig[HAName] = "Connection state";
      Jconfig[HADeviceClass] = "connectivity";
      Jconfig[HAEntityCategory] = "diagnostic";
      Jconfig[HAStateTopic] = String(HAlora2ha) + "/" + String(HAuniqueId) + "/hub/availability/" + String(HAState);
      Jconfig[HAExpireAfter] =  (Watchdog * 60) + 10; // 10min 10s, must publish state every 10 min (not retained)
      topic = String(HAConfigRoot) + "/" + String(HAComponentBinarySensor) + "/" + String(HAuniqueId) + "_HUB/connection_state/config";

      Lres = serializeJson(docJSon, payload);

      mqtt.beginPublish(topic.c_str(), Lres, true);
      mqtt.write((const uint8_t*)(payload.c_str()), Lres);
      mqtt.endPublish();
    }
    void publishConfigWatchdog()
    {
      String payload;
      String topic;
      size_t Lres;
      // HUB configuration
      JsonDocument docJSon;
      JsonObject Jconfig = docJSon.to<JsonObject>();
      JsonObject device = Jconfig["device"].to<JsonObject>();
      device = fillPublishConfigDevice(device);

      Jconfig[HAObjectID] =  String(AP_ssid) + "_hub_watchdog";
      Jconfig[HAUniqueID] =  String(HAuniqueId) + "_hub_watchdog";
      Jconfig[HAName] = "Watchdog timer";
      Jconfig[HAStateTopic] = String(HAlora2ha) + "/" + String(HAuniqueId) + "/hub/watchdog/" + String(HAState);
      Jconfig[HACommandTopic] = String(HAlora2ha) + "/" + String(HAuniqueId) + "/hub/watchdog/" + String(HACommand);
      Jconfig[HAEntityCategory] = "config";
      Jconfig[HAMin] = 1;
      Jconfig[HAMax] = 60;
      Jconfig[HAIcon] = "mdi:timer-alert";

      topic = String(HAConfigRoot) + "/" + String(HAComponentNumber) + "/" + String(HAuniqueId) + "_HUB/watchdog/config";

      Lres = serializeJson(docJSon, payload);
      mqtt.beginPublish(topic.c_str(), Lres, true);
      mqtt.write((const uint8_t*)(payload.c_str()), Lres);
      mqtt.endPublish();

      //
      publishWatchdog();
    }
    void publishConfig()
    {
      publishConfigAvail();
      publishConfigWatchdog();
      // Devices configuration
      CDevice* dev = nullptr;
      while ((dev = walkDevice(dev)))
      {
        dev->publishConfig();
      }
      String command = String(HAlora2ha) + "/" + String(HAuniqueId) + "/#";
      DEBUGf("mqtt subscribe : %s\n", command.c_str());
      mqtt.subscribe(command.c_str());
    }
    void publishOnline()
    {
      String topic = String(HAlora2ha) + "/" + String(HAuniqueId) + "/hub/availability/" + String(HAState);
      String payload = "ON";
      DEBUGln("Publish Online");
      mqtt.beginPublish(topic.c_str(), payload.length(), false);
      mqtt.write((const uint8_t*)(payload.c_str()), payload.length());
      mqtt.endPublish();
      needPublishOnline = false;
    }
    void publishWatchdog()
    {
      String topic = String(HAlora2ha) + "/" + String(HAuniqueId) + "/hub/watchdog/" + String(HAState);
      String payload = String(Watchdog);
      mqtt.beginPublish(topic.c_str(), payload.length(), true);
      mqtt.write((const uint8_t*)(payload.c_str()), payload.length());
      mqtt.endPublish();
    }

    void pairingConfig(rl_packet_t* cp)
    {
      CDevice* dev = getDeviceByAddress(cp->senderID);
      CEntity* ent = getEntityById(cp->senderID, cp->data.configs.base.childID);
      rl_conf_t cnfIdx = (rl_conf_t)(cp->sensordataType & 0x07);
      if (cnfIdx == C_BASE)
      { // Basic configuration
        rl_configBase_t* cnfb = (rl_configBase_t*)&cp->data.configs.base;
        rl_element_t st = (rl_element_t)cnfb->deviceType;
        rl_data_t dt = (rl_data_t)cnfb->dataType;
        DEBUGf("Conf B %d %s\n", cp->senderID, cnfb->name);
        if (cnfb->childID == RL_ID_CONFIG)
        { // process Device config
          if (dev) {
            // remove existing device
            delDevice(cp->senderID);
            dev = nullptr;
          }
          JsonVariant deviceItem = _docJson["dev"].as<JsonArray>().add<JsonVariant>();
          deviceItem["address"] = cp->senderID;
          deviceItem["name"] = String(cnfb->name);
          deviceItem["model"] = "?";
          //
          dev = newDevice(deviceItem);
          if (dev) {
            DEBUGf("Added Device %d %s\n", cp->senderID, cnfb->name);
            dev->setPairing(true);
            notifyDeviceForm(cp->senderID);
          } else
          {
            DEBUGf("** Error adding new Device %d\n", cp->senderID);
            return;
          }
        } else
        { // process Entity config
          if (!dev)
          {
            DEBUGf("** Error, unable to find Device %d for Child %d\n", cp->senderID, cnfb->childID);
            return;
          }
          ent = dev->getEntityById(cnfb->childID);
          size_t srcMax = sizeof(cnfb->name);
          char cName[srcMax + 1];
          strncpy(cName, cnfb->name, srcMax);
          cName[srcMax] = 0;
          if (ent)
          { // modify Child config
            ent->setAttr("label", cName, 0);
          } else
          { // add new Child
            EntityCategory ec = EntityCategory::CategoryAuto;
            if (st == E_SELECT || st == E_INPUTNUMBER) {
              ec = EntityCategory::CategoryConfig;
            }
            JsonVariant entityItem = dev->addEmptyEntity();
            entityItem["id"] = cnfb->childID;
            entityItem["label"] = String(cName);
            entityItem["sensortype"] = (int)st;
            entityItem["datatype"] = (int)dt;
            entityItem["unit"] = "";
            entityItem["class"] = "";
            entityItem["category"] = ec;

            ent = dev->newEntity(entityItem);
            if (!ent)
            {
              DEBUGf("Error adding new Entity %d on Device %d\n", cnfb->childID, cp->senderID);
              return;
            }
          }
        }
      }
      if (cnfIdx == C_UNIT && dev && ent)
      {
        rl_configText_t* cnft = (rl_configText_t*)&cp->data.configs.text;
        uint8_t childID = cnft->childID;
        size_t srcMax = sizeof(cnft->text);
        char txt[srcMax + 1];
        strncpy(txt, cnft->text, srcMax);
        txt[srcMax] = 0;
        DEBUGf("Conf U %d %s\n", childID, txt);
        ent->setAttr("unit", txt, 0);
      }
      if (cnfIdx == C_OPTS && dev)
      {
        rl_configText_t* cnft = (rl_configText_t*)&cp->data.configs.text;
        uint8_t childID = cnft->childID;
        //uint8_t index = cnft->index;
        size_t srcMax = sizeof(cnft->text);
        char txt[srcMax + 1];
        strncpy(txt, cnft->text, srcMax);
        txt[srcMax] = 0;
        if (childID == RL_ID_CONFIG)
        {
          DEBUGf("Conf O %d %s\n", cp->senderID, txt);
          dev->setModel(txt);
        } else {
          DEBUGf("Conf O %d %s\n", childID, txt);
          if (ent)
          {
            ent->setAttr("options", txt, cnft->index);
          }
        }
      }
      if (cnfIdx == C_NUMS && dev && ent) {
        rl_configNums_t* cnfn = (rl_configNums_t*)&cp->data.configs.nums;
        uint8_t childID = cnfn->childID;
        DEBUGf("Conf N %d %d %d %d\n", childID, cnfn->divider, cnfn->mini, cnfn->maxi);
        if (ent) {
          if (cnfn->divider == 0) cnfn->divider = 1;
          ent->setNumberConfig(cnfn->mini, cnfn->maxi, cnfn->divider);
        }
      }
      if (cnfIdx == C_END && dev)
      {
        uint8_t address = dev->getAddress();
        if (cp->data.configs.base.childID == RL_ID_CONFIG)
        {
          DEBUGf("Conf END %d\n", address);
          notifyDeviceHeader(address);
          notifyDevicePairing(address);
        } else {
          uint8_t id = cp->data.configs.base.childID;
          DEBUGf("Conf END %d %d\n", address, id);
          notifyChildLine(address, id);
        }
      }
    }
    bool loadConfig()
    {
      DEBUGln("Loading config file");
      File file = SPIFFS.open("/config.json");
      if (!file || file.isDirectory())
      {
        DEBUGln("** Failed to open config file");
        return false;
      }
      DeserializationError error = deserializeJson(_docJson, file);
      file.close();
      if (error)
      {
        DEBUGln("failed to deserialize config file");
        return false;
      }
      // walk device array
      JsonVariant devices = _docJson["dev"];
      if (devices.is<JsonArray>())
      {
        for (JsonVariant deviceItem : devices.as<JsonArray>())
        {
          if (!deviceItem.isNull())
          {
            newDevice(deviceItem);
          }
        }
      }
      DEBUGln("Config file loaded");
      return true;
    }
    void saveConfig()
    {
      String Jres;
      size_t Lres = serializeJson(_docJson, Jres);
      //DEBUGln(Jres);
      File file = SPIFFS.open("/config.json", "w");
      if (file)
      {
        file.write((byte*)Jres.c_str(), Lres);
        file.close();
        DEBUGln("Config file is saved");
      } else {
        DEBUGln("** Error saving Config file");
      }
    }
    JsonObject fillPublishConfigDevice(JsonObject device)
    {
      device[HAIdentifiers] = "hub_" + String(HAuniqueId);
      device[HAManufacturer] = "M&L";
      device[HAName] = String(AP_ssid) + " HUB";
      device[HASwVersion] = VERSION;
#if defined(ARDUINO_LOLIN_S2_MINI)
      device[HAModel] = "MLH1";
#elif defined(ARDUINO_ESP32_POE_ISO)
      device[HAModel] = "MLH2";
#elif defined(ARDUINO_WT32_ETH01)
      device[HAModel] = "MLH3";
#elif defined(ARDUINO_D1_MINI32)
      device[HAModel] = "MLH4";
#elif defined(ARDUINO_TTGO_LoRa32_v21new)
      device[HAModel] = "MLH5";
#else
      device[HAModel] = "Custom";
#endif
      return device;
    }
  private:
    CDevice* _deviceFirst;
    JsonDocument _docJson;
    bool _oldMQTTconnected;
};

CHub hub;
