rl_packets _packet;

class Element
{
  public:
    Element(uint8_t id, int16_t EEPadr, float delta, const __FlashStringHelper* name, const __FlashStringHelper* unit, int16_t divider) {
      _id = id;
      _eepadr = EEPadr;
      _elementType = rl_element_t::E_CUSTOM;
      _dataType = rl_data_t::D_NUM;
      _delta = delta * divider;
      _divider = divider;
      _name = name;
      _unit = unit;
      if (_eepadr > 0) {
        // default value is BYTE
        _curValue = EEPROM.read(_eepadr);
      } else {
        _curValue = -999999;
      }
      _sentValue = -999999;
      _options = nullptr;
      _mini = 0;
      _maxi = 255;
    }
    uint8_t getID() {
      return _id;
    }
    virtual int32_t getValue() {
      return _curValue;
    }
    virtual uint8_t getBool() {
      return _curValue != 0;
    }
    virtual float getFloat() {
      return (float)_curValue / _divider;
    }
    virtual const char* getText() {
      // must be overrided
      return "?";
    }
    virtual void setValue(int32_t newValue) {
      _curValue = newValue;
    }
    virtual void setBool(uint8_t newValue) {
      setValue((int32_t)newValue);
    }
    virtual void setFloat(float newValue) {
      setValue((int32_t)(newValue * _divider));
    }
    virtual void setText(const char* newValue) {
      // must be overrided
    }
    virtual void Process() {
      // must be overrided
      DEBUGln(F("process not overrided"));
    }
    void publishConfig() {
      rl_configBase_t cnfb;
      memset(&cnfb, 0, sizeof(cnfb));
      cnfb.childID = _id;
      cnfb.deviceType = (uint8_t)_elementType;
      cnfb.dataType = (uint8_t)_dataType;
      // config text contain Name on 8 first char and unit on 5 last
      uint8_t len = strlen_P(reinterpret_cast<const char*>(_name));
      strncpy_P(cnfb.name, reinterpret_cast<const char*>(_name), min(len, sizeof(cnfb.name)));
      RLcomm.publishConfig(hubid, uid, (rl_configs_t*)&cnfb, C_BASE);
      //
      if (_unit && strlen_P(reinterpret_cast<const char*>(_unit))) {
        rl_configText_t cnft;
        cnft.childID = _id;
        len = strlen_P(reinterpret_cast<const char*>(_unit));
        strncpy_P(cnft.text, reinterpret_cast<const char*>(_unit), min(len, sizeof(cnft.text)));
        RLcomm.publishConfig(hubid, uid, (rl_configs_t*)&cnft, C_UNIT);
      }
      //
      if (_elementType == E_SELECT && _options) {
        rl_configText_t cnft;
        memset(&cnft, 0, sizeof(cnft));
        cnft.childID = _id;
        len = strlen_P(reinterpret_cast<const char*>(_options));
        strncpy_P(cnft.text, reinterpret_cast<const char*>(_options), min(len, sizeof(cnft.text)));
        RLcomm.publishConfig(hubid, uid, (rl_configs_t*)&cnft, C_OPTS);
      }
      //
      if (_elementType == E_INPUTNUMBER) {
        rl_configNums_t cnfn;
        memset(&cnfn, 0, sizeof(cnfn));
        cnfn.childID = _id;
        cnfn.divider = _divider;
        cnfn.mini = _mini;
        cnfn.maxi = _maxi;
        cnfn.step = 1;
        RLcomm.publishConfig(hubid, uid, (rl_configs_t*)&cnfn, C_NUMS);
      }
      //
      memset(&cnfb, 0, sizeof(cnfb));
      cnfb.childID = _id;
      cnfb.deviceType = (uint8_t)_elementType;
      cnfb.dataType = (uint8_t)_dataType;
      RLcomm.publishConfig(hubid, uid, (rl_configs_t*)&cnfb, C_END);
    }
    uint32_t Send() {
      if (abs(_sentValue - _curValue) > _delta)
      {
        _sentValue = _curValue;
        switch ((int)_elementType) {
          case rl_element_t::E_BINARYSENSOR:
          case rl_element_t::E_SWITCH:
          case rl_element_t::E_NUMERICSENSOR:
          case rl_element_t::E_INPUTNUMBER:
            DEBUG(_name); DEBUG(" > "); DEBUGln(_curValue);
            publishValue();
            break;
          case rl_element_t::E_SELECT:
          case rl_element_t::E_TEXTSENSOR:
            DEBUG(_name); DEBUG(" > "); DEBUGln(getText());
            publishText(getText());
            break;
        }
        return _curValue;
      }
      return false;
    }
  protected:
    void publishPacket() {
      _packet.current.destinationID = hubid;
      _packet.current.senderID = uid;
      _packet.current.childID = _id;
      _packet.current.sensordataType = (_elementType << 3) + _dataType;
      RLcomm.publishPaquet(&_packet);
    }
    void publishValue() {
      _packet.current.data.num.value = _curValue;
      _packet.current.data.num.divider = _divider;
      publishPacket();
    }
    void publishText(const char* text) {
      uint8_t l = strlen(text);
      if (l > sizeof(_packet.current.data.text)) l = sizeof(_packet.current.data.text);
      memset(_packet.current.data.text, 0, sizeof(_packet.current.data.text));
      strncpy(_packet.current.data.text, text, l);
      publishPacket();
    }
    uint8_t _id;
    int16_t _eepadr;
    rl_element_t _elementType;
    rl_data_t _dataType;
    int32_t _curValue;
    int32_t _sentValue;
    int16_t _divider;
    int16_t _delta;
    int16_t _mini;
    int16_t _maxi;
    const __FlashStringHelper* _name;
    const __FlashStringHelper* _unit;
    const __FlashStringHelper* _options;
};

Element** _elementList;
uint8_t _elementCount = 0;

Element* ML_addElement(Element* newElement) {
  _elementList = (Element**)realloc(_elementList, (_elementCount + 1) * sizeof(Element*));
  if (_elementList == nullptr) {
    return nullptr;
  }
  _elementList[_elementCount] = newElement;
  _elementCount++;
  return _elementList[_elementCount - 1];
}
void ML_ProcessElements() {
  for (uint8_t i = 0; i < _elementCount; i++)
  {
    ((Element*)_elementList[i])->Process();
  }
}
void ML_SendElements() {
  for (uint8_t i = 0; i < _elementCount; i++)
  {
    ((Element*)_elementList[i])->Send();
  }
}
void ML_PublishConfigElements() {
  rl_configs_t cnf;
  uint8_t len;
  memset(&cnf, 0, sizeof(cnf));
  cnf.base.childID = RL_ID_CONFIG; // for device config
  cnf.base.deviceType = E_CUSTOM;
  cnf.base.dataType = D_TEXT;
  // config text contain Name
  len = strlen(deviceName);
  strncpy(cnf.base.name, deviceName, min(len, sizeof(cnf.base.name)));
  RLcomm.publishConfig(hubid, uid, &cnf, C_BASE);
  // config text contain Model
  memset(&cnf.text.text, 0, sizeof(cnf.text.text));
  len = strlen(deviceModel);
  strncpy(cnf.text.text, deviceModel, min(len, sizeof(cnf.text.text)));
  RLcomm.publishConfig(hubid, uid, &cnf, C_OPTS);
  // end of conf
  cnf.text.text[0] = 0;
  RLcomm.publishConfig(hubid, uid, &cnf, C_END);
  //
  for (uint8_t i = 0; i < _elementCount; i++)
  {
    ((Element*)_elementList[i])->publishConfig();
  }
}
Element* ML_getElementByID(uint8_t id)
{
  for (uint8_t i = 0; i < _elementCount; i++)
  {
    if (((Element*)_elementList[i])->getID() == id)
      return ((Element*)_elementList[i]);
  }
  return nullptr;
}
