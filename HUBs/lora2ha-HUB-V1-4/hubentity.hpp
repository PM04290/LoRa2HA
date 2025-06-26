#pragma once

enum EntityCategory {
  CategoryAuto = 0,
  CategoryConfig,
  CategoryDiagnostic
};

class CEntity {
  public:
    explicit CEntity(uint8_t adr, const char* devName, JsonVariant JSconf)
    {
      _devAdr = adr;
      _devName = devName;
      _nextEntity = nullptr;
      _JSconf = JSconf;
      _state = nullptr;
      DEBUGf("NEW   entity %s\n", getLabel());
    }
    ~CEntity()
    {
      DEBUGf("Delete entity %s\n", getLabel());
      cleanConfig();
    }
    CEntity* Next()
    {
      return _nextEntity;
    }
    void setNext(CEntity* ent)
    {
      _nextEntity = ent;
    }
    void setAttr(String attr, String newValue, uint8_t idx)
    {
      // if name changed, HA configuration will be erased
      // it need to be reconfigured after
      if (attr == "label" && strcmp(newValue.c_str(), _JSconf["label"].as<const char*>()) != 0)
      {
        cleanConfig();
      }
      if (isValidInt(newValue))
      {
        _JSconf[attr] = newValue.toInt();
      } else if (isValidFloat(newValue))
      {
        _JSconf[attr] = serialized(String(newValue.toFloat(), 3));
      } else
      {
        if (idx) {
          String tmp = _JSconf[attr];
          _JSconf[attr] = tmp + "," + newValue;
        } else {
          _JSconf[attr] = newValue;
        }
      }
    }
    uint8_t getId()
    {
      return _JSconf["id"].as<int>();
    }
    const char* getLabel()
    {
      return _JSconf["label"].as<const char*>();
    }
    rl_element_t getElementType()
    {
      return (rl_element_t)_JSconf["sensortype"].as<int>();
    }
    rl_data_t getDataType()
    {
      return (rl_data_t)_JSconf["datatype"].as<int>();
    }
    const char* getClass()
    {
      return _JSconf["class"].as<const char*>();
    }
    const char* getUnit()
    {
      return _JSconf["unit"].as<const char*>();
    }
    EntityCategory getCategory()
    {
      EntityCategory ec = EntityCategory::CategoryAuto;
      if (_JSconf["category"].is<int>()) {
        ec = (EntityCategory)_JSconf["category"].as<int>();
      }
      return ec;
    }
    int getExpire()
    {
      return _JSconf["expire"] | 0L;;
    }
    int32_t getMini()
    {
      int32_t mini(LONG_MIN);
      if (_JSconf["min"].is<int>())
        mini = _JSconf["min"].as<int>();
      return mini;
    }
    int32_t getMaxi()
    {
      int32_t maxi(LONG_MAX);
      if (_JSconf["max"].is<int>())
        maxi = _JSconf["max"].as<int>();
      return maxi;
    }
    float getCoefA()
    {
      float coefA = 1.;
      if (_JSconf["coefa"].is<float>())
        coefA = _JSconf["coefa"];
      return coefA;
    }
    float getCoefB()
    {
      float coefB = 0.;
      if (_JSconf["coefb"].is<float>())
        coefB = _JSconf["coefb"];
      return coefB;
    }
    const char* getSelectOptions()
    {
      return _JSconf["options"].as<const char*>();
    }
    int32_t getNumberMin()
    {
      int32_t imin(LONG_MIN);
      if (_JSconf["imin"].is<int>())
        imin = _JSconf["imin"].as<int>();
      return imin;
    }
    int32_t getNumberMax()
    {
      int32_t imax(LONG_MAX);
      if (_JSconf["imax"].is<int>())
        imax = _JSconf["imax"].as<int>();
      return imax;
    }
    int16_t getNumberDiv()
    {
      int32_t idiv(1);
      if (_JSconf["idiv"].is<int>())
        idiv = _JSconf["idiv"].as<int>();
      return idiv;
    }
    void setNumberConfig(int32_t mini, int32_t maxi, int16_t divider)
    {
      _JSconf["imin"] = mini;
      _JSconf["imax"] = maxi;
      _JSconf["idiv"] = divider;
      DEBUGf("   %d : mM=%d %d %d\n", _JSconf["id"].as<int>(), mini, maxi, divider);
    }
    bool setState(int32_t newState)
    {
      if (!_state)
      {
        _state = new int32_t(newState);
      }
      *reinterpret_cast<int32_t*>(_state) = newState;
      publishState();
      return true;
    }

    bool setState(bool newState)
    {
      return setState((int32_t)newState);
    }

    bool setState(float newState)
    {
      if (!_state)
      {
        _state = new float(newState);
      }
      *reinterpret_cast<float*>(_state) = newState;
      publishState();
      return true;
    }

    bool setState(char* newState)
    {
      if (!_state)
      {
        _state = malloc(RL_PACKET_SIZE + 1);
      }
      strcpy(reinterpret_cast<char*>(_state), newState);
      publishState();
      return true;
    }
    void setDeviceAddress(uint8_t address)
    {
      _devAdr = address;
    }
    void switchToLoRa(int state)
    {
      DEBUGf("switch to lora (%02d/%02d) %s=%d\n", _devAdr, getId(), getLabel(), state);
      RLcomm.publishSwitch(_devAdr, UIDcode, getId(), state);
    }
    void selectToLoRa(const char* state)
    {
      DEBUGf("select to lora (%02d/%02d) %s=%s\n", _devAdr, getId(), getLabel(), state);
      RLcomm.publishText(_devAdr, UIDcode, getId(), state);
    }
    void inputToLoRa(float state)
    {
      DEBUGf("input to lora (%02d/%02d) %s=%f\n", _devAdr, getId(), getLabel(), state);
      RLcomm.publishFloat(_devAdr, UIDcode, getId(), state * 1000, 1000);
    }
    void buttonToLoRa()
    {
      DEBUGf("button to lora (%02d/%02d) %s\n", _devAdr, getId(), getLabel());
      RLcomm.publishText(_devAdr, UIDcode, getId(), "!");
    }
    void coverToLoRa(const char* state)
    {
      DEBUGf("cover to lora (%02d/%02d) %s=%s\n", _devAdr, getId(), getLabel(), state);
      RLcomm.publishText(_devAdr, UIDcode, getId(), state);
    }
    String getHAtopic()
    {
      String topic = String(HAConfigRoot) + "/";
      switch (getElementType())
      {
        case E_BINARYSENSOR:
          topic += "binary_sensor";
          break;
        case E_NUMERICSENSOR:
          topic += "sensor";
          break;
        case E_SWITCH:
          topic += "switch";
          break;
        case E_LIGHT:
          topic += "light";
          break;
        case E_COVER:
          topic += "cover";
          break;
        case E_FAN:
          topic += "fan";
          break;
        case E_HVAC:
          topic += "hvac";
          break;
        case E_SELECT:
          topic += "select";
          break;
        case E_TRIGGER:
          topic += "device_automation";
          break;
        case E_EVENT:
          topic += "event";
          break;
        case E_TAG:
          topic += "tag";
          // TODO
          break;
        case E_TEXTSENSOR:
          topic += "sensor";
          break;
        case E_INPUTNUMBER:
          topic += "number";
          break;
        case E_CUSTOM:
          break;
        case E_DATE:
          break;
        case E_TIME:
          break;
        case E_DATETIME:
          break;
        case E_BUTTON:
          topic += "button";
          break;
        case E_CONFIG:
          break;
        default:
          break;
      }
      if (getElementType() == E_TRIGGER)
      {
        topic += "/" + String(HAuniqueId) + "/action_single_" + strHAuid() + "/config";
      } else {
        topic += "/" + String(HAuniqueId) + "/" + strHAuid() + "/config";
      }
      return topic;
    }
    void cleanConfig()
    {
      String topic = getHAtopic();
      DEBUGln(topic);
      mqtt.publish(topic.c_str(), "", true);
    }
    void publishConfig()
    {
      rl_element_t elType = getElementType();
      JsonDocument docJSon;
      JsonObject Jconfig = docJSon.to<JsonObject>();
      JsonObject device = Jconfig["device"].to<JsonObject>();

      String devUID = String(_devName);
      devUID.replace(" ", "_");

      device[HAIdentifiers] = String(AP_ssid) + "_" + devUID;
      device[HAManufacturer] = "M&L";
      device[HAName] = String(_devName);
      device[HAViaDevice] = "hub_" + String(HAuniqueId);

      Jconfig[HAName] = String(getLabel());
      Jconfig[HAUniqueID] = String(HAuniqueId) + "_" + strHAuid();
      if (elType != E_TRIGGER)
      {
        Jconfig[HAStateTopic] = strHAprefix() + "/" + String(HAStateTopic);
      }
      switch (elType)
      {
        case E_BINARYSENSOR:
          if (strlen(getClass())) {
            Jconfig[HADeviceClass] = getClass();
          }
          break;
        case E_NUMERICSENSOR:
          if (strlen(getClass())) {
            Jconfig[HADeviceClass] = getClass();
          }
          if (strlen(getUnit())) {
            Jconfig[HAUnitOfMeasurement] = getUnit();
          }
          if (strcmp(getClass(), "energy") == 0)
          {
            Jconfig["state_class"] = "total_increasing";
          }
          break;
        case E_SWITCH:
          Jconfig[HACommandTopic] = strHAprefix() + "/" + String(HACommandTopic);
          break;
        case E_LIGHT:
          Jconfig[HACommandTopic] = strHAprefix() + "/" + String(HACommandTopic);
          break;
        case E_COVER:
          Jconfig[HACommandTopic] = strHAprefix() + "/" + String(HACommandTopic);
          Jconfig[HAStateOpen] = "open";
          Jconfig[HAStateOpening] = "opening";
          Jconfig[HAStateClosed] = "closed";
          Jconfig[HAStateClosing] = "closing";
          Jconfig[HAStateStopped] = "stopped";
          Jconfig[HAPayloadOpen] = "OPEN";
          Jconfig[HAPayloadClose] = "CLOSE";
          Jconfig[HAPayloadStop] = "STOP";
          break;
        case E_FAN:
          Jconfig[HACommandTopic] = strHAprefix() + "/" + String(HACommandTopic);
          break;
        case E_HVAC:
          break;
        case E_SELECT:
          if (strlen(getSelectOptions())) {
            char tmp[20] = {0};
            const char* p = getSelectOptions();
            uint8_t n = 0;
            while (*p) {
              if (*p == ',') {
                Jconfig["options"].add(tmp);
                n = 0;
              } else {
                tmp[n++] = *p;
              }
              p++;
            }
            Jconfig["options"].add(tmp);
          }
          Jconfig[HACommandTopic] = strHAprefix() + "/" + String(HACommandTopic);
          break;
        case E_TRIGGER:
          Jconfig["atype"] = "trigger";
          Jconfig["payload"] = String(getLabel()); //"single";
          Jconfig["subtype"] = String(getLabel()); //"single";
          Jconfig["topic"] = strHAprefix() + "/action";
          Jconfig["type"] = "action";
          break;
        case E_EVENT:
          break;
        case E_TAG:
          // TODO
          break;
        case E_TEXTSENSOR:
          break;
        case E_INPUTNUMBER:
          Jconfig["min"] = serialized(String((float)getNumberMin() / getNumberDiv(), 3));
          Jconfig["max"] = serialized(String((float)getNumberMax() / getNumberDiv(), 3));
          Jconfig["step"] = serialized(String(1. / getNumberDiv(), 1));
          if (strlen(getUnit())) {
            Jconfig[HAUnitOfMeasurement] = getUnit();
          }
          Jconfig[HACommandTopic] = strHAprefix() + "/" + String(HACommandTopic);
          break;
        case E_CUSTOM:
          break;
        case E_DATE:
          break;
        case E_TIME:
          break;
        case E_DATETIME:
          break;
        case E_BUTTON:
          Jconfig[HACommandTopic] = strHAprefix() + "/" + String(HACommandTopic);
          break;
        case E_CONFIG:
          DEBUGf("E_CONFIG not available %s\n", getLabel());
          break;
        default:
          DEBUGf("Incorrect sensorType %s\n", getLabel());
          break;
      }
      if (getCategory() == CategoryDiagnostic)
      {
        Jconfig[HAEntityCategory] = "diagnostic";
      }
      if (getCategory() == CategoryConfig)
      {
        Jconfig[HAEntityCategory] = "config";
      }
      String topic = getHAtopic();
      //DEBUGln(topic);

      String payload;
      size_t Lres = serializeJson(docJSon, payload);
      //DEBUGln(payload);
      //mqtt.publish(topic.c_str(), (const uint8_t*)payload.c_str(), payload.length(), true);
      mqtt.beginPublish(topic.c_str(), Lres, true);
      mqtt.write((const uint8_t*)(payload.c_str()), Lres);
      mqtt.endPublish();
    }
    void publishState()
    {
      String topic = "";
      String payload = "";
      bool retained = true;
      rl_element_t elType = getElementType();
      rl_data_t elData = getDataType();

      topic = strHAprefix();
      (elType == E_TRIGGER) ? topic += "/action" : topic += "/" + String(HAStateTopic);

      switch (elType)
      {
        case E_BINARYSENSOR:
          payload = *reinterpret_cast<int32_t*>(_state) > 0 ? HAStateOn : HAStateOff;
          break;
        case E_NUMERICSENSOR:
          if (elData == D_FLOAT)
          {
            payload = String(*reinterpret_cast<float*>(_state));
          } else {
            payload = String(*reinterpret_cast<int32_t*>(_state));
          }
          break;
        case E_SWITCH:
          payload = *reinterpret_cast<int32_t*>(_state) > 0 ? HAStateOn : HAStateOff;
          break;
        case E_LIGHT:
          break;
        case E_COVER:
          payload = String(reinterpret_cast<char*>(_state));
          break;
        case E_FAN:
          break;
        case E_HVAC:
          break;
        case E_SELECT:
          payload = String(reinterpret_cast<char*>(_state));
          break;
        case E_TRIGGER:
          payload = String(getLabel());
          retained = false;
          break;
        case E_EVENT:
          payload = "press";
          break;
        case E_TAG:
          break;
        case E_TEXTSENSOR:
          payload = String(reinterpret_cast<char*>(_state));
          break;
        case E_INPUTNUMBER:
          if (elData == D_FLOAT)
          {
            payload = String(*reinterpret_cast<float*>(_state));
          } else {
            payload = String(*reinterpret_cast<int32_t*>(_state));
          }
          break;
        case E_CUSTOM:
          break;
        case E_DATE:
          break;
        case E_TIME:
          break;
        case E_DATETIME:
          break;
        case E_BUTTON:
          break;
        case E_CONFIG:
          break;
      }
      if (payload != "")
      {
        DEBUGf("Publish : %s\n", payload.c_str());
        mqtt.beginPublish(topic.c_str(), payload.length(), retained);
        mqtt.write((const uint8_t*)(payload.c_str()), payload.length());
        mqtt.endPublish();
      }
    }
    void doPacketForHA(rl_packet_t p)
    {
      byte sender = p.senderID;
      byte child = p.childID;
      rl_element_t st = (rl_element_t)((p.sensordataType >> 3) & 0x1F);
      rl_data_t dt = (rl_data_t)(p.sensordataType & 0x07);
      float fval;
      String sval;
      char tag[9] = {0};
      char tmp[RL_PACKET_SIZE];
      size_t srcMax = sizeof(p.data.text);
      if (dt == D_TEXT)
      {
        DEBUGf("Do packet from %d / %d = %s (%02X)\n", sender, child, p.data.text, p.sensordataType);
      } else {
        DEBUGf("Do packet from %d / %d = %d (%02X)\n", sender, child, p.data.num.value, p.sensordataType);
      }
      rl_element_t elType = getElementType();
      rl_data_t elData = getDataType();
      if (st != elType)
      {
        DEBUGf("**ERR Different sensor type %d / %d\n", (int)st, (int)elType);
      }

      switch (elType)
      {
        case E_BINARYSENSOR:
          DEBUGf("Binary Sensor : %d\n", (int)(p.data.num.value > 0));
          setState(p.data.num.value > 0);
          break;
        case E_NUMERICSENSOR:
          fval = p.data.num.value;
          if ((dt == D_FLOAT) && (p.data.num.divider != 0))
          {
            fval = (float)p.data.num.value / (float)p.data.num.divider;
            DEBUGf("Numeric Sensor : %d/%d\n", p.data.num.value, p.data.num.divider);
          } else
          {
            DEBUGf("Numeric Sensor : %d\n", p.data.num.value);
          }
          fval = fval * getCoefA() + getCoefB();
          if (fval >= getMini() && fval <= getMaxi())
          {
            if (elData == D_FLOAT)
            {
              setState(fval);
            } else
            {
              setState((int32_t)fval);
            }
          }
          break;
        case E_SWITCH:
          setState(p.data.num.value > 0);
          break;
        case E_LIGHT:
          // TODO
          break;
        case E_COVER:
          memset(tmp, 0, sizeof(tmp));
          strncpy(tmp, p.data.text, srcMax);
          tmp[srcMax] = 0;
          setState(tmp);
          break;
        case E_FAN:
          // TODO
          break;
        case E_HVAC:
          // TODO
          break;
        case E_SELECT:
          memset(tmp, 0, sizeof(tmp));
          strncpy(tmp, p.data.text, srcMax);
          tmp[srcMax] = 0;
          setState(tmp);
          break;
        case E_TRIGGER:
          DEBUGf("Trigger : %d\n", p.data.num.value);
          // Only binary trigger
          setState(p.data.num.value > 0);
          break;
        case E_EVENT:
          break;
        case E_TAG:
          byteArrayToStr(tag, p.data.rawByte, 4);
          //TODO ((HATagScanner*)_HAdevice)->tagScanned(tag);
          break;
        case E_TEXTSENSOR:
          memset(tmp, 0, sizeof(tmp));
          strncpy(tmp, p.data.text, srcMax);
          tmp[srcMax] = 0;
          setState(tmp);
          break;
        case E_INPUTNUMBER:
          fval = p.data.num.value;
          if ((dt == D_FLOAT) && (p.data.num.divider != 0))
          {
            DEBUGf("Input Number : %d/%d\n", p.data.num.value, p.data.num.divider);
            fval = (float)p.data.num.value / (float)p.data.num.divider;
          } else {
            DEBUGf("Input Number : %d\n", p.data.num.value);
          }
          fval = fval * getCoefA() + getCoefB();
          if (fval >= getMini() && fval <= getMaxi())
          {
            if (elData == D_FLOAT)
            {
              setState(fval);
            } else
            {
              setState((int32_t)fval);
            }
          }
          break;
        case E_CUSTOM:
          break;
        case E_DATE:
          break;
        case E_TIME:
          break;
        case E_DATETIME:
          break;
        case E_BUTTON:
          break;
        case E_CONFIG:
          break;
      }
    }
    void onMessageMQTT(char* topic, char* payload)
    {
      String command = strHAprefix() + "/" + String(HACommandTopic);
      //DEBUGf("%s\n%s\n", topic, command.c_str());
      if (String(topic) == command)
      {
        switch (getElementType())
        {
          case E_BINARYSENSOR:
            break;
          case E_NUMERICSENSOR:
            break;
          case E_SWITCH:
            if (strcmp(payload, "ON") == 0)
            {
              switchToLoRa(1);
            }
            if (strcmp(payload, "OFF") == 0)
            {
              switchToLoRa(0);
            }
            break;
          case E_LIGHT:
            break;
          case E_COVER:
            coverToLoRa(payload);
            break;
          case E_FAN:
            break;
          case E_HVAC:
            break;
          case E_SELECT:
            selectToLoRa(payload);
            break;
          case E_TRIGGER:
            break;
          case E_EVENT:
            break;
          case E_TAG:
            break;
          case E_TEXTSENSOR:
            break;
          case E_INPUTNUMBER:
            inputToLoRa(String(payload).toFloat());
            break;
          case E_CUSTOM:
            break;
          case E_DATE:
            break;
          case E_TIME:
            break;
          case E_DATETIME:
            break;
          case E_BUTTON:
            buttonToLoRa();
            break;
          case E_CONFIG:
            break;
        }
      }
    }
    String strHAuid()
    {
      String devUID = String(_devName);
      devUID.replace(" ", "_");
      String eltUID = String(getLabel());
      eltUID.replace(" ", "_");
      return devUID + "_" + eltUID;
    }
    String strHAprefix()
    {
      return String(HAlora2ha) + "/" + String(HAuniqueId) + "/" + strHAuid();
    }

  private:
    uint8_t _devAdr;
    const char* _devName;
    CEntity* _nextEntity;
    JsonVariant _JSconf;
    void* _state; // typed pointer : uint32_t, float, char[]
};
