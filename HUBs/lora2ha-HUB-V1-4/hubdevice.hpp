#pragma once

#include "hubentity.hpp"

class CDevice {
  public:
    explicit CDevice(JsonVariant JSconf)
    {
      _nextDevice = nullptr;
      _JSconf = JSconf;
      _pairing = false;
      _firstEntity = nullptr;
      DEBUGf("NEW device %s\n", getName());
      // walk childs array
      if (JSconf["childs"].is<JsonArray>())
      {
        JsonVariant entities = JSconf["childs"];
        for (JsonVariant entityItem : entities.as<JsonArray>())
        {
          if (!entityItem.isNull())
          {
            newEntity(entityItem);
          }
        }
      }
    }
    ~CDevice()
    {
      DEBUGf("Delete device %s\n", getName());
      cleanConfig();
      while (_firstEntity)
      {
        CEntity* ent = _firstEntity;
        _firstEntity = ent->Next();
        delete ent;
      }
    }
    CDevice* Next() {
      return _nextDevice;
    }
    void setNext(CDevice* dev)
    {
      _nextDevice = dev;
    }
    uint8_t getAddress()
    {
      return _JSconf["address"].as<int>();
    }
    void setAddress(uint8_t oldAdr, uint8_t newAdr)
    {
      rl_configParam_t cnfp;
      memset(&cnfp, 0, sizeof(cnfp));
      cnfp.childID = 0; // 0 for device param
      cnfp.pInt = newAdr;
      // Send new address to Device
      RLcomm.publishConfig(oldAdr, UIDcode, (rl_configs_t*)&cnfp, C_PARAM);
      _JSconf["address"] = newAdr;
    }
    const char* getName()
    {
      return _JSconf["name"].as<const char*>();
    }
    const char* getModel()
    {
      return _JSconf["model"].as<const char*>();
    }
    void setModel(const char* model)
    {
      _JSconf["model"] = String(model);
    }
    JsonVariant addEmptyEntity()
    {
      return _JSconf["childs"].add<JsonObject>();
    }
    CEntity* newEntity(JsonVariant JSconf)
    {
      CEntity* newent = new CEntity(getAddress(), getName(), JSconf);
      if (_firstEntity == nullptr)
      {
        _firstEntity = newent;
      } else
      {
        CEntity* ent = _firstEntity;
        while (ent)
        {
          if (ent->Next() == nullptr)
          {
            ent->setNext(newent);
            return newent;
          }
          ent = ent->Next();
        }
      }
      return newent;
    }
    CEntity* getEntityById(uint8_t id)
    {
      CEntity* ent = _firstEntity;
      while (ent)
      {
        if (ent->getId() == id)
        {
          return ent;
        }
        ent = ent->Next();
      }
      return nullptr;
    }
    CEntity* walkEntity(CEntity* ent)
    {
      if (ent) {
        return ent->Next();
      }
      return _firstEntity;
    }
    bool getPairing()
    {
      return _pairing;
    }
    void setPairing(bool pairing)
    {
      _pairing = pairing;
    }
    void setLQI(int lqi)
    {
      String devUID = String(getName());
      devUID.replace(" ", "_");
      String topic = String(HAlora2ha) + "/" + String(HAuniqueId) + "/" + devUID + "/linkquality/" + String(HAState);
      String payload = String (lqi);
      mqtt.beginPublish(topic.c_str(), payload.length(), true);
      mqtt.write((const uint8_t*)(payload.c_str()), payload.length());
      mqtt.endPublish();
    }
    void onMessageMQTT(char* topic, char* payload)
    {
      CEntity* ent = nullptr;
      while ((ent = walkEntity(ent)))
      {
        ent->onMessageMQTT(topic, payload);
      }
    }
    void cleanConfig()
    {
      String devUID = String(getName());
      devUID.replace(" ", "_");
      String topic = String(HAConfigRoot) + "/" + String(HAComponentSensor) + "/" + String(HAuniqueId) + "/" + devUID + "_linkquality/config";;
      mqtt.publish(topic.c_str(), "", true);
    }
    void publishConfig()
    {
      String payload;
      String topic;
      size_t Lres;
      DEBUGf("Publish config %s\n", getName());
      // HUB configuration
      JsonDocument docJSon;
      JsonObject Jconfig = docJSon.to<JsonObject>();
      JsonObject device = Jconfig["device"].to<JsonObject>();
      String devUID = String(getName());
      devUID.replace(" ", "_");
      device[HAIdentifiers] = String(AP_ssid) + "_" + devUID;
      device[HAManufacturer] = "M&L";
      device[HAName] = String(getName());
      device[HAModel] = String(getModel());
      device[HAViaDevice] = "hub_" + String(HAuniqueId);

      Jconfig[HAUniqueID] = String(HAuniqueId) + "_" + devUID + "_linkquality";
      Jconfig[HAName] = "Link quality";
      Jconfig[HAStateTopic] = String(HAlora2ha) + "/" + String(HAuniqueId) + "/" + devUID + "/linkquality/" + String(HAState);
      Jconfig[HAEntityCategory] = "diagnostic";
      Jconfig[HAUnitOfMeasurement] = "lqi";
      Jconfig[HAIcon] = "mdi:signal";

      topic = String(HAConfigRoot) + "/" + String(HAComponentSensor) + "/" + String(HAuniqueId) + "/" + devUID + "_linkquality/config";

      Lres = serializeJson(docJSon, payload);
      mqtt.beginPublish(topic.c_str(), Lres, true);
      mqtt.write((const uint8_t*)(payload.c_str()), Lres);
      mqtt.endPublish();
      //
      CEntity* ent = nullptr;
      while ((ent = walkEntity(ent)))
      {
        ent->publishConfig();
      }
    }
  private:
    CDevice* _nextDevice;
    JsonVariant _JSconf;
    CEntity* _firstEntity;
    bool _pairing;
};
