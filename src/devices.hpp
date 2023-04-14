#pragma once

#include <RLDconfig.h>

#define DEVICES_UNKNOWN_ID 254

class Child {
  public:
    explicit Child(const char* baseuid, uint8_t address, uint8_t id, const char* lbl, rl_device_t st, rl_data_t dt, const char* devclass, const char* unit);
    uint8_t getId();
    char* getLabel();
    char* getClass();
    char* getUnit();
    rl_device_t getSensorType();
    rl_data_t getDataType();
    void doPacketV1ForHA(rl_packetV1_t p);
    void doPacketV2ForHA(rl_packetV2_t p);
    void doPacketForHA(rl_packet_t p);
    HABaseDeviceType* getHADevice();
    // HA -> LoRa commands
    void switchToLoRa(bool state);
    void lightStateToLoRa(bool state);
    void lightBrightnessToLoRa(uint8_t brightness);
    void lightColorTemperature(uint16_t temperature);
    void lightRGBColor(HALight::RGBColor color);
    void coverToLoRa(HACover::CoverCommand cmd);
  protected:
    uint8_t _address;
    uint8_t _id;
    char* _label;
    char* _HAuid;
    rl_device_t _sensorType;
    rl_data_t _dataType;
    char* _devclass;
    char* _unit;
    HABaseDeviceType* _HAdevice;
};

class Device {
  public:
    explicit Device(uint8_t address, const char* name);
    Child* addChild(Child* ch);
    uint8_t getAddress();
    char* getName();
    int getNbChild();
    Child* getChild(int n);
    Child* getChildById(uint8_t id);
  protected:
    uint8_t _address;
    char* _name;
    Child** _childList;
    int _nbChild;
};

class HubDev {
  public:
    explicit HubDev();
    uint8_t getNbDevice();
    Device* addDevice(Device* dev);
    Device* getDevice(int n);
    Child* getChildById(uint8_t address, uint8_t id);
    Child* getChildByHAbase(HABaseDeviceType* HAdevice);
  protected:
    Device** _deviceList;
    int _nbDevice;
};

HubDev hub;

//************************************************
void onSwitchCommand(bool state, HASwitch* sender)
{
  Child* ch = hub.getChildByHAbase(sender);
  if (ch)
  {
    ch->switchToLoRa(state);
  }
}

void onLightStateCommand(bool state, HALight* sender)
{
  Child* ch = hub.getChildByHAbase(sender);
  if (ch)
  {
    ch->lightStateToLoRa(state);
  }
}

void onLightBrightnessCommand(uint8_t brightness, HALight* sender) {
  Child* ch = hub.getChildByHAbase(sender);
  if (ch)
  {
    ch->lightBrightnessToLoRa(brightness);
  }
}

void onLightColorTemperatureCommand(uint16_t temperature, HALight* sender)
{
  Child* ch = hub.getChildByHAbase(sender);
  if (ch)
  {
    ch->lightColorTemperature(temperature);
  }
}

void onLightRGBColorCommand(HALight::RGBColor color, HALight* sender)
{
  Child* ch = hub.getChildByHAbase(sender);
  if (ch)
  {
    ch->lightRGBColor(color);
  }
}

void onCoverCommand(HACover::CoverCommand cmd, HACover* sender)
{
  Child* ch = hub.getChildByHAbase(sender);
  if (ch)
  {
    ch->coverToLoRa(cmd);
  }
}

//************************************************
HubDev::HubDev()
{
  _nbDevice = 0;
  _deviceList = nullptr;
}

uint8_t HubDev::getNbDevice()
{
  return _nbDevice;
}

Device* HubDev::getDevice(int n)
{
  if (n < _nbDevice)
  {
    return _deviceList[n];
  }
  return nullptr;
}

Device* HubDev::addDevice(Device * dev)
{
  _deviceList = (Device**)realloc(_deviceList, (_nbDevice + 1) * sizeof(Device*));
  if (_deviceList == nullptr)
  {
    DEBUGln("erreur realloc");
    return nullptr;
  }
  _deviceList[_nbDevice] = dev;
  _nbDevice++;
  return dev;
}

Child* HubDev::getChildById(uint8_t address, uint8_t id)
{
  for (uint8_t d = 0; d < _nbDevice; d++)
  {
    Device* dev = _deviceList[d];
    if (dev->getAddress() == address)
    {
      return dev->getChildById(id);
    }
  }
  return nullptr;
}

Child* HubDev::getChildByHAbase(HABaseDeviceType* HAdevice)
{
  for (uint8_t d = 0; d < hub.getNbDevice(); d++)
  {
    Device* dev = hub.getDevice(d);
    for (uint8_t c = 0; c < dev->getNbChild(); c++)
    {
      Child* ch = dev->getChild(c);
      if (ch && (ch->getHADevice() == HAdevice))
      {
        return ch;
      }
    }
  }
  return nullptr;
}

//************************************************
Device::Device(uint8_t address, const char* name)
{
  _name = (char*)malloc(strlen(name) + 1);
  strcpy(_name, name);
  _name[strlen(name)] = 0;

  _address = address;
  _nbChild = 0;
  _childList = nullptr;
}

Child* Device::addChild(Child * ch)
{
  _childList = (Child**)realloc(_childList, (_nbChild + 1) * sizeof(Child*));
  if (_childList == nullptr) {
    DEBUGln("defaut realloc");
    return nullptr;
  }
  _childList[_nbChild] = ch;
  _nbChild++;
  return ch;
}

uint8_t Device::getAddress()
{
  return _address;
}

char* Device::getName()
{
  return _name;
}

int Device::getNbChild()
{
  return  _nbChild;
}

Child* Device::getChild(int n)
{
  if (n < _nbChild)
  {
    return _childList[n];
  }
  return nullptr;
}

Child* Device::getChildById(uint8_t id)
{
  for (uint8_t c = 0; c < _nbChild; c++)
  {
    if (_childList[c]->getId() == id)
    {
      return _childList[c];
    }
  }
  return nullptr;
}

//************************************************
Child::Child(const char* baseuid, uint8_t address, uint8_t id, const char* lbl, rl_device_t st, rl_data_t dt, const char* devclass, const char* unit)
{
  _label = (char*)malloc(strlen(lbl) + 1);
  strcpy(_label, lbl);
  _HAuid = (char*)malloc(strlen(baseuid) + strlen(_label) + 2);
  strcpy(_HAuid, baseuid);
  strcat(_HAuid, "_");
  strcat(_HAuid, _label);
  _devclass = (char*)malloc(strlen(devclass) + 1);
  strcpy(_devclass, devclass);
  _unit = (char*)malloc(strlen(unit) + 1);
  strcpy(_unit, unit);
  _address = address;
  _id = id;
  _sensorType = st;
  _dataType = dt;
  _HAdevice = nullptr;
  switch (st) {
    case S_BINARYSENSOR:
      _HAdevice = new HABinarySensor(_HAuid);
      if (strlen(_devclass))
      {
        ((HABinarySensor*)_HAdevice)->setDeviceClass(_devclass);
      }
      break;
    case S_NUMERICSENSOR:
      _HAdevice = new HASensorNumber(_HAuid);
      if (strlen(_devclass))
      {
        ((HASensorNumber*)_HAdevice)->setDeviceClass(_devclass);
      }
      if (strlen(_unit))
      {
        ((HASensorNumber*)_HAdevice)->setUnitOfMeasurement(_unit);
      }
      break;
    case S_SWITCH:
      _HAdevice = new HASwitch(_HAuid);
      ((HASwitch*)_HAdevice)->onCommand(onSwitchCommand);
      break;
    case S_LIGHT:
      if (dt == V_BOOL)
      {
        _HAdevice = new HALight(_HAuid);
        ((HALight*)_HAdevice)->onStateCommand(onLightStateCommand);
      }
      if (dt == V_RAW)
      {
        _HAdevice = new HALight(_HAuid, HALight::BrightnessFeature | HALight::ColorTemperatureFeature | HALight::RGBFeature);
        ((HALight*)_HAdevice)->onStateCommand(onLightStateCommand);
        ((HALight*)_HAdevice)->onBrightnessCommand(onLightBrightnessCommand);
        ((HALight*)_HAdevice)->onColorTemperatureCommand(onLightColorTemperatureCommand);
        ((HALight*)_HAdevice)->onRGBColorCommand(onLightRGBColorCommand);
      }
      break;
    case S_COVER:
      _HAdevice = new HACover(_HAuid);
      break;
    case S_FAN:
      //TODO
      break;
    case S_HVAC:
      // TODO
      break;
    case S_SELECT:
      // TODO
      break;
    case S_TRIGGER:
      _HAdevice = new HADeviceTrigger(HADeviceTrigger::ButtonShortPressType, _HAuid);
      break;
    case S_CUSTOM:
      // TODO
      break;
    case S_TAG:
      _HAdevice = new HATagScanner(_HAuid);
      break;
    case S_TEXTSENSOR:
      _HAdevice = new HASensor(_HAuid);
      ((HASensor*)_HAdevice)->setDeviceClass(_devclass);
      ((HASensor*)_HAdevice)->setUnitOfMeasurement(_unit);
      break;
  }
  if (_HAdevice)
  {
    _HAdevice->setName(_HAuid);
  }
}

uint8_t Child::getId()
{
  return _id;
}

char* Child::getLabel()
{
  return _label;
}

rl_device_t Child::getSensorType()
{
  return _sensorType;
}

rl_data_t Child::getDataType()
{
  return _dataType;
}

char* Child::getClass()
{
  return _devclass;
}

char* Child::getUnit()
{
  return _unit;
}

HABaseDeviceType* Child::getHADevice()
{
  return _HAdevice;
}

void Child::switchToLoRa(bool state)
{
  DEBUGf("switch to lora %s=%d\n", _label, state);
  RLcomm.publishNum(_address, 0, _id, _sensorType, state);
}

void Child::lightStateToLoRa(bool state)
{
  DEBUGf("light state to lora %s\n=%d", _label, state);
  if (_dataType == V_BOOL)
  {
    RLcomm.publishNum(_address, 0, _id, _sensorType, state);
  }
  if (_dataType == V_RAW)
  {
    uint8_t brightness = ((HALight*)_HAdevice)->getCurrentBrightness();
    uint16_t temp = ((HALight*)_HAdevice)->getCurrentColorTemperature();
    HALight::RGBColor rgb = ((HALight*)_HAdevice)->getCurrentRGBColor();
    RLcomm.publishLight(_address, 0, _id, state, brightness, temp, rgb.red, rgb.green, rgb.blue);
  }
}

void Child::lightBrightnessToLoRa(uint8_t brightness)
{
  DEBUGf("light brightness to lora %s=%d\n", _label, brightness);
  uint8_t state = ((HALight*)_HAdevice)->getCurrentState();
  uint16_t temp = ((HALight*)_HAdevice)->getCurrentColorTemperature();
  HALight::RGBColor rgb = ((HALight*)_HAdevice)->getCurrentRGBColor();
  RLcomm.publishLight(_address, 0, _id, state, brightness, temp, rgb.red, rgb.green, rgb.blue);
}

void Child::lightColorTemperature(uint16_t temperature)
{
  DEBUGf("light temperature to lora %s=%d\n", _label, temperature);
  uint8_t state = ((HALight*)_HAdevice)->getCurrentState();
  uint8_t brightness = ((HALight*)_HAdevice)->getCurrentBrightness();
  HALight::RGBColor rgb = ((HALight*)_HAdevice)->getCurrentRGBColor();
  RLcomm.publishLight(_address, 0, _id, state, brightness, temperature, rgb.red, rgb.green, rgb.blue);
}

void Child::lightRGBColor(HALight::RGBColor color)
{
  DEBUGf("light color to lora %s=%d:%d:%d\n", _label, color.red, color.green, color.blue);
  uint8_t state = ((HALight*)_HAdevice)->getCurrentState();
  uint8_t brightness = ((HALight*)_HAdevice)->getCurrentBrightness();
  uint16_t temp = ((HALight*)_HAdevice)->getCurrentColorTemperature();
  RLcomm.publishLight(_address, 0, _id, state, brightness, temp, color.red, color.green, color.blue);
}

void Child::coverToLoRa(HACover::CoverCommand cmd)
{
  uint8_t pos = ((HACover*)_HAdevice)->getCurrentPosition();
  RLcomm.publishCover(_address, 0, _id, (uint8_t)cmd, pos);
}

void Child::doPacketV1ForHA(rl_packetV1_t p)
{
  // so old, not supported
}

void Child::doPacketV2ForHA(rl_packetV2_t p)
{
  byte sender = p.senderID;
  byte child = p.childID;
  DEBUGf("Do packet(V2) from %d / %d : %s ( %d)\n", sender, child, _HAuid, p.data.num.value);
  rl_device_t st = (rl_device_t)((p.sensordataType >> 3) & 0x1F);
  rl_data_t dt;
  float fval;
  String sval;
  char tag[9] = {0};
  switch (st)
  {
    case S_BINARYSENSOR:
      ((HABinarySensor*)_HAdevice)->setState(p.data.num.value > 0);
      break;
    case S_NUMERICSENSOR:
      dt = (rl_data_t)(p.sensordataType & 0x07);
      if (dt == V_FLOAT)
      {
        fval = p.data.num.value;
        if (p.data.num.divider != 0)
        {
          fval = (float)p.data.num.value / (float)p.data.num.divider;
        }
        ((HASensorNumber*)_HAdevice)->setPrecision(p.data.num.precision % 4); // %4 for precision 0 to 3
        ((HASensorNumber*)_HAdevice)->setValue(fval);
      }
      else
      {
        ((HASensorNumber*)_HAdevice)->setValue(p.data.num.value);
      }
      break;
    case S_SWITCH:
      ((HASwitch*)_HAdevice)->setState(p.data.num.value > 0);
      break;
    case S_LIGHT:
      ((HALight*)_HAdevice)->setState(p.data.num.value > 0);
      break;
    case S_COVER:
      ((HACover*)_HAdevice)->setState((HACover::CoverState)p.data.num.value);
      break;
    case S_FAN:
      // TODO
      break;
    case S_HVAC:
      // TODO
      break;
    case S_SELECT:
      // TODO
      break;
    case S_TRIGGER:
      dt = (rl_data_t)(p.sensordataType & 0x07);
      sval = String(p.data.num.value);
      if (dt == V_TEXT)
      {
        sval = String(p.data.text);
      }
      ((HADeviceTrigger*)_HAdevice)->trigger(sval.c_str());
      break;
    case S_CUSTOM:
      // TODO
      break;
    case S_TAG:
      HAUtils::byteArrayToStr(tag, p.data.rawByte, 4);
      ((HATagScanner*)_HAdevice)->tagScanned(tag);
      break;
  }
}

void Child::doPacketForHA(rl_packet_t p)
{
  byte sender = p.senderID;
  byte child = p.childID;
  DEBUGf("Do packet from %d / %d : %s (%d)\n", sender, child, _HAuid, p.data.num.value);
  rl_device_t st = (rl_device_t)((p.sensordataType >> 3) & 0x1F);
  rl_data_t dt;
  float fval;
  String sval;
  char tag[9] = {0};
  switch (st)
  {
    case S_BINARYSENSOR:
      ((HABinarySensor*)_HAdevice)->setState(p.data.num.value > 0);
      break;
    case S_NUMERICSENSOR:
      dt = (rl_data_t)(p.sensordataType & 0x07);
      if (dt == V_FLOAT)
      {
        fval = p.data.num.value;
        if (p.data.num.divider != 0)
        {
          fval = (float)p.data.num.value / (float)p.data.num.divider;
        }
        ((HASensorNumber*)_HAdevice)->setPrecision(p.data.num.precision % 4); // %4 for precision 0 to 3
        ((HASensorNumber*)_HAdevice)->setValue(fval);
      }
      else
      {
        ((HASensorNumber*)_HAdevice)->setValue(p.data.num.value);
      }
      break;
    case S_SWITCH:
      ((HASwitch*)_HAdevice)->setState(p.data.num.value > 0);
      break;
    case S_LIGHT:
      if (dt == V_BOOL)
      {
        ((HALight*)_HAdevice)->setState(p.data.num.value > 0);
      }
      if (dt == V_RAW)
      {
        ((HALight*)_HAdevice)->setState(p.data.light.state);
        ((HALight*)_HAdevice)->setBrightness(p.data.light.brightness);
        ((HALight*)_HAdevice)->setColorTemperature(p.data.light.temperature);
        ((HALight*)_HAdevice)->setRGBColor(HALight::RGBColor(p.data.light.red, p.data.light.green, p.data.light.blue) );
      }
      break;
    case S_COVER:
      if (dt == V_NUM)
      {
        ((HACover*)_HAdevice)->setState((HACover::CoverState)p.data.num.value);
      }
      if (dt == V_RAW)
      {
        ((HACover*)_HAdevice)->setState((HACover::CoverState)p.data.cover.state);
        ((HACover*)_HAdevice)->setPosition(p.data.cover.position);
      }
      break;
    case S_FAN:
      // TODO
      break;
    case S_HVAC:
      // TODO
      break;
    case S_SELECT:
      // TODO
      break;
    case S_TRIGGER:
      dt = (rl_data_t)(p.sensordataType & 0x07);
      sval = String(p.data.num.value);
      if (dt == V_TEXT)
      {
        sval = String(p.data.text);
      }
      ((HADeviceTrigger*)_HAdevice)->trigger(sval.c_str());
      break;
    case S_CUSTOM:
      // TODO
      break;
    case S_TAG:
      HAUtils::byteArrayToStr(tag, p.data.rawByte, 4);
      ((HATagScanner*)_HAdevice)->tagScanned(tag);
      break;
  }
}

// ******************************************
