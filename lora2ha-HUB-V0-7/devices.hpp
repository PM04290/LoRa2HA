// https://developers.home-assistant.io/docs/core/entity/sensor/
#pragma once
#include <PubSubClient.h>

WiFiClient client;
PubSubClient mqtt(client);

enum EntityCategory {
  CategoryAuto = 0,
  CategoryConfig,
  CategoryDiagnostic
};

const char HexMap[] PROGMEM = {"0123456789abcdef"};

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

class Device;

class Child {
  public:
    explicit Child(Device* parent, uint8_t id, const char* lbl, rl_element_t st, rl_data_t dt, const char* devclass, EntityCategory ec, const char* unit, int expire, int32_t mini, int32_t maxi, float A, float B);
    uint8_t getId();
    char* getLabel();
    char* getClass();
    EntityCategory getCategory();
    char* getUnit();
    int getExpire();
    int32_t getMini();
    int32_t getMaxi();
    float getCoefA();
    float getCoefB();
    rl_element_t getElementType();
    rl_data_t getDataType();
    void doPacketForHA(rl_packet_t p);
    void setUnit(const char* unit);
    bool setState(int32_t newState);
    bool setState(bool newState);
    bool setState(float newState);
    bool setState(char* newState);
    void setSelectOptions(const char* options);
    char* getSelectOptions();
    void setNumberConfig(int32_t mini, int32_t maxi, int16_t step);
    int32_t getNumberMin();
    int32_t getNumberMax();
    int16_t getNumberDiv();
    // HA -> LoRa commands
    void switchToLoRa(int state);
    void selectToLoRa(const char* state);
    void inputToLoRa(float state);
    /*
      void lightStateToLoRa(bool state);
      void lightBrightnessToLoRa(uint8_t brightness);
      void lightColorTemperature(uint16_t temperature);
      void lightRGBColor(HALight::RGBColor color);
      void coverToLoRa(HACover::CoverCommand cmd);
      void inputnumberToLoRa(HANumeric number);*/
    void publishConfig();
    void publishState();
    void onMessageMQTT(char* topic, char* payload);
  protected:
    Device* _device;
    uint8_t _id;
    char* _label;
    char* _HAuid;
    char* _HAname;
    rl_element_t _elementType;
    rl_data_t _dataType;
    char* _devclass;
    EntityCategory _category;
    char* _unit;
    int _expire;
    int32_t _mini;
    int32_t _maxi;
    float _coefA;
    float _coefB;
    void* _state; // typed pointer : uint32_t, float, char[]
    char* _options;
    int32_t _numberMini;
    int32_t _numberMaxi;
    int16_t _numberDivider;
};

class Device {
  public:
    explicit Device(uint8_t address, const char* name, uint8_t byConf, uint8_t rlversion = 0);
    Child* addChild(Child* ch);
    uint8_t getAddress();
    char* getName();
    void setModel(const char* model);
    char* getModel();
    int getNbChild();
    uint8_t isConfLoading();
    uint8_t getRLversion();
    Child* getChild(int n);
    Child* getChildById(uint8_t id);
    uint8_t getIdxChildById(uint8_t id);
    void publishConfig();
    void setLQI(uint8_t lqi);
    void onMessageMQTT(char* topic, char* payload);
  protected:
    uint8_t _address;
    char* _name;
    char* _model;
    Child** _childList;
    int _nbChild;
    int8_t _byConf;
    uint8_t _rlversion;
};

class HubDev {
  public:
    explicit HubDev();
    uint8_t getNbDevice();
    uint8_t getIdxDeviceByAddress(uint8_t address);
    Device* addDevice(Device* dev);
    Device* getDevice(int n);
    Device* getDeviceById(uint8_t address);
    Child* getChildById(uint8_t address, uint8_t id);
    //    Child* getChildByHAbase(HABaseDeviceType* HAdevice);
    void setUniqueId(const byte* uniqueId, const uint16_t length);
    const char* getUniqueId();
    void MQTTconnect();
    void processMQTT();
    void processDateTime();
    void publishConfig();
    void publishOnline();
    void onMessageMQTT(char* topic, char* payload);
  private:
    JsonObject fillPublishConfigDevice(JsonObject device);
    void publishConfigAvail();
    void publishConfigWatchdog();
    void publishWatchdog();
  protected:
    Device** _deviceList;
    int _nbDevice;
    const char* _uniqueId;
    bool _oldMQTTconnected;
};

HubDev hub;


//************************************************
HubDev::HubDev()
{
  _nbDevice = 0;
  _deviceList = nullptr;
  _uniqueId = nullptr;
  _oldMQTTconnected = false;
}

uint8_t HubDev::getNbDevice()
{
  return _nbDevice;
}

uint8_t HubDev::getIdxDeviceByAddress(uint8_t address)
{
  for (uint8_t d = 0; d < _nbDevice; d++)
  {
    Device* dev = _deviceList[d];
    if (dev->getAddress() == address)
    {
      return d;
    }
  }
  return 0xFF;
}

Device* HubDev::getDevice(int n)
{
  if (n < _nbDevice)
  {
    return _deviceList[n];
  }
  return nullptr;
}

Device* HubDev::getDeviceById(uint8_t address)
{
  for (uint8_t d = 0; d < _nbDevice; d++)
  {
    Device* dev = _deviceList[d];
    if (dev->getAddress() == address)
    {
      return dev;
    }
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

void HubDev::setUniqueId(const byte* uniqueId, const uint16_t length)
{
  if (_uniqueId) {
    return; // unique ID cannot be changed at runtime once it's set
  }
  _uniqueId = byteArrayToStr(uniqueId, length);
}

const char* HubDev::getUniqueId()
{
  return _uniqueId;
}

JsonObject HubDev::fillPublishConfigDevice(JsonObject device)
{
  device[HAIdentifiers] = "hub_" + String(hub.getUniqueId());
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
#else
  device[HAModel] = "Custom";
#endif
  return device;
}

void HubDev::publishConfigAvail()
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
  Jconfig[HAUniqueID] =  String(hub.getUniqueId()) + "_hub_connection_state";
  Jconfig[HAName] = "Connection state";
  Jconfig[HADeviceClass] = "connectivity";
  Jconfig[HAEntityCategory] = "diagnostic";
  Jconfig[HAStateTopic] = String(HAlora2ha) + "/" + String(hub.getUniqueId()) + "/hub/availability/" + String(HAState);
  Jconfig[HAExpireAfter] =  (Watchdog * 60) + 10; // 10min 10s, must publish state every 10 min (not retained)
  topic = String(HAConfigRoot) + "/" + String(HAComponentBinarySensor) + "/" + String(hub.getUniqueId()) + "_HUB/connection_state/config";

  Lres = serializeJson(docJSon, payload);

  mqtt.beginPublish(topic.c_str(), payload.length(), true);
  mqtt.write((const uint8_t*)(payload.c_str()), payload.length());
  mqtt.endPublish();
}

void HubDev::publishConfigWatchdog()
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
  Jconfig[HAUniqueID] =  String(hub.getUniqueId()) + "_hub_watchdog";
  Jconfig[HAName] = "Watchdog timer";
  Jconfig[HAStateTopic] = String(HAlora2ha) + "/" + String(hub.getUniqueId()) + "/hub/watchdog/" + String(HAState);
  Jconfig[HACommandTopic] = String(HAlora2ha) + "/" + String(hub.getUniqueId()) + "/hub/watchdog/" + String(HACommand);
  Jconfig[HAEntityCategory] = "config";
  Jconfig[HAMin] = 1;
  Jconfig[HAMax] = 60;
  Jconfig[HAIcon] = "mdi:timer-alert";

  topic = String(HAConfigRoot) + "/" + String(HAComponentNumber) + "/" + String(hub.getUniqueId()) + "_HUB/watchdog/config";

  Lres = serializeJson(docJSon, payload);
  mqtt.beginPublish(topic.c_str(), payload.length(), true);
  mqtt.write((const uint8_t*)(payload.c_str()), payload.length());
  mqtt.endPublish();

  //
  publishWatchdog();
}

void HubDev::publishConfig()
{
  publishConfigAvail();
  publishConfigWatchdog();
  // Devices configuration
  for (uint8_t d = 0; d < hub.getNbDevice(); d++)
  {
    Device* dev = getDevice(d);
    if (dev)
    {
      dev->publishConfig();
    }
  }

  String command = String(HAlora2ha) + "/" + String(hub.getUniqueId()) + "/#";
  //DEBUGf("mqtt subscribe : %s\n", command.c_str());
  mqtt.subscribe(command.c_str());
}

void HubDev::publishOnline()
{
  String topic = String(HAlora2ha) + "/" + String(hub.getUniqueId()) + "/hub/availability/" + String(HAState);
  String payload = "ON";
  DEBUGln("Publish Online");
  mqtt.beginPublish(topic.c_str(), payload.length(), false);
  mqtt.write((const uint8_t*)(payload.c_str()), payload.length());
  mqtt.endPublish();
  needPublishOnline = false;
}


void HubDev::publishWatchdog()
{
  String topic = String(HAlora2ha) + "/" + String(hub.getUniqueId()) + "/hub/watchdog/" + String(HAState);
  String payload = String(Watchdog);
  mqtt.beginPublish(topic.c_str(), payload.length(), true);
  mqtt.write((const uint8_t*)(payload.c_str()), payload.length());
  mqtt.endPublish();
}

void HubDev::MQTTconnect()
{
  DEBUGf("MQTT start connect(%s,%s,%s)\n", AP_ssid, mqtt_user, mqtt_pass);
  mqtt.setServer(mqtt_host, mqtt_port);
  mqtt.setCallback(MQTTcallback);
  mqtt.setBufferSize(1024);
  if (mqtt.connect(AP_ssid, mqtt_user, mqtt_pass))
  {
    DEBUGf("MQTT connect\n");
    // TODO ?
  } else {
    DEBUGf("MQTT NOT connected to %s:%d [%d]\n", mqtt_host, mqtt_port, mqtt.state());
  }
}

void HubDev::processMQTT()
{
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
  }
}

void HubDev::onMessageMQTT(char* topic, char* payload)
{
  if (String(topic).endsWith("/hub/watchdog/command"))
  {
    Watchdog = max(1, atoi(payload) % 60);
    EEPROM.writeByte(EEPROM_DATA_WDOG, Watchdog);
    EEPROM.commit();
    publishWatchdog();
    return;
  }
  for (uint8_t d = 0; d < hub.getNbDevice(); d++)
  {
    Device* dev = getDevice(d);
    if (dev)
    {
      dev->onMessageMQTT(topic, payload);
    }
  }
}

void HubDev::processDateTime()
{
  if (!newRTC) return;
  DEBUGf("broadcast DateTime\n");
  uint8_t raw[MAX_PACKET_DATA_LEN];
  memset(raw, 0, sizeof(raw));
  raw[0] = rtc.year() - 2000;
  raw[1] = rtc.month();
  raw[2] = rtc.day();
  raw[3] = rtc.hour();
  raw[4] = rtc.minute();
  raw[5] = rtc.second();
  RLcomm.publishRaw(RL_ID_BROADCAST, UIDcode, RL_ID_DATETIME, raw, MAX_PACKET_DATA_LEN);
}

//************************************************
Device::Device(uint8_t address, const char* name, uint8_t byConf, uint8_t rlversion)
{
  _name = (char*)malloc(strlen(name) + 1);
  strcpy(_name, name);
  _name[strlen(name)] = 0;
  _model = nullptr;
  _address = address;
  _nbChild = 0;
  _childList = nullptr;
  _byConf = byConf;
  _rlversion = rlversion;
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

void Device::setModel(const char* model)
{
  _model = (char*)malloc(strlen(model) + 1);
  strcpy(_model, model);
  _model[strlen(model)] = 0;
}

char* Device::getModel()
{
  return _model;
}

int Device::getNbChild()
{
  return  _nbChild;
}

uint8_t Device::isConfLoading()
{
  return _byConf;
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

uint8_t Device::getIdxChildById(uint8_t id)
{
  for (uint8_t c = 0; c < _nbChild; c++)
  {
    if (_childList[c]->getId() == id)
    {
      return c;
    }
  }
  return 0;
}

uint8_t Device::getRLversion()
{
  return _rlversion;
}

void Device::publishConfig()
{
  if (_byConf != 1) return;

  String payload;
  String topic;
  size_t Lres;
  // HUB configuration
  JsonDocument docJSon;
  JsonObject Jconfig = docJSon.to<JsonObject>();
  JsonObject device = Jconfig["device"].to<JsonObject>();
  String devUID = String(_name);
  devUID.replace(" ", "_");
  device[HAIdentifiers] = String(AP_ssid) + "_" + devUID;
  device[HAManufacturer] = "M&L";
  device[HAName] = String(_name);
  if (_model) {
    device[HAModel] = String(_model);
  }
  device[HAViaDevice] = "hub_" + String(hub.getUniqueId());

  Jconfig[HAUniqueID] = String(hub.getUniqueId()) + "_" + devUID + "_linkquality";
  Jconfig[HAName] = "Link quality";
  Jconfig[HAStateTopic] = String(HAlora2ha) + "/" + String(hub.getUniqueId()) + "/" + devUID + "/linkquality/" + String(HAState);
  Jconfig[HAEntityCategory] = "diagnostic";
  Jconfig[HAUnitOfMeasurement] = "lqi";
  Jconfig[HAIcon] = "mdi:signal";

  topic = String(HAConfigRoot) + "/" + String(HAComponentSensor) + "/" + String(hub.getUniqueId()) + "/" + devUID + "_linkquality/config";

  Lres = serializeJson(docJSon, payload);
  mqtt.beginPublish(topic.c_str(), payload.length(), true);
  mqtt.write((const uint8_t*)(payload.c_str()), payload.length());
  mqtt.endPublish();

  for (uint8_t c = 0; c < _nbChild; c++)
  {
    Child* ch = getChild(c);
    if (ch)
    {
      ch->publishConfig();
    }
  }
}

void Device::setLQI(uint8_t lqi)
{
  String devUID = String(_name);
  devUID.replace(" ", "_");
  String topic = String(HAlora2ha) + "/" + String(hub.getUniqueId()) + "/" + devUID + "/linkquality/" + String(HAState);
  String payload = String (lqi);
  mqtt.beginPublish(topic.c_str(), payload.length(), true);
  mqtt.write((const uint8_t*)(payload.c_str()), payload.length());
  mqtt.endPublish();
}

void Device::onMessageMQTT(char* topic, char* payload)
{
  for (uint8_t c = 0; c < _nbChild; c++)
  {
    Child* ch = getChild(c);
    if (ch)
    {
      ch->onMessageMQTT(topic, payload);
    }
  }
}

//************************************************
Child::Child(Device* parent, uint8_t id, const char* lbl, rl_element_t st, rl_data_t dt, const char* devclass, EntityCategory ec, const char* unit, int expire = 0, int32_t mini = LONG_MIN, int32_t maxi = LONG_MAX, float A = 1., float B = 0.)
{
  _device = parent;
  _id = id;
  _label = (char*)malloc(strlen(lbl) + 1);
  strcpy(_label, lbl);
  _HAuid = (char*)malloc(strlen(_device->getName()) + strlen(_label) + 2);
  strcpy(_HAuid, _device->getName());
  strcat(_HAuid, "_");
  strcat(_HAuid, _label);
  for (int n = 0; n < strlen(_HAuid); n++) {
    if (_HAuid[n] == ' ') _HAuid[n] = '_';
  }
  _elementType = st;
  _dataType = dt;
  _devclass = (char*)malloc(strlen(devclass) + 1);
  strcpy(_devclass, devclass);
  _category = ec;
  _unit = (char*)malloc(strlen(unit) + 1);
  strcpy(_unit, unit);
  _expire = expire;
  _mini = mini;
  _maxi = maxi;
  _coefA = A;
  _coefB = B;
  _HAname = nullptr;
  _options = nullptr;
  _numberMini = 0;
  _numberMaxi = 255;
  _numberDivider = 1;

  DEBUGf("   %d : %s (%s) st[%d] dt{%d] HA=%s %s %f %f\n", _id, _label, _unit, (int)_elementType, (int)_dataType, _HAuid, _devclass, _coefA, _coefB);

  _HAname = (char*)malloc(strlen(_device->getName()) + strlen(_label) + 2);
  strcpy(_HAname, _device->getName());
  strcat(_HAname, " ");
  strcat(_HAname, _label);

  _state = nullptr;
}

uint8_t Child::getId()
{
  return _id;
}

char* Child::getLabel()
{
  return _label;
}

rl_element_t Child::getElementType()
{
  return _elementType;
}

rl_data_t Child::getDataType()
{
  return _dataType;
}

char* Child::getClass()
{
  return _devclass;
}

EntityCategory Child::getCategory()
{
  return _category;
}

char* Child::getUnit()
{
  return _unit;
}

int Child::getExpire()
{
  return _expire;
}

int32_t Child::getMini()
{
  return _mini;
}

int32_t Child::getMaxi()
{
  return _maxi;
}

float Child::getCoefA()
{
  return _coefA;
}
float Child::getCoefB()
{
  return _coefB;
}

void Child::setUnit(const char* unit)
{
  _unit = (char*)realloc(_unit, strlen(unit) + 1);
  strcpy(_unit, unit);
}

void Child::switchToLoRa(int state)
{
  DEBUGf("switch to lora V%d (%02d/%02d) %s=%d\n", _device->getRLversion(), _device->getAddress(), _id, _label, state);
  RLcomm.publishSwitch(_device->getAddress(), UIDcode, _id, state, _device->getRLversion());
}
void Child::selectToLoRa(const char* state)
{
  DEBUGf("select to lora V%d (%02d/%02d) %s=%s\n", _device->getRLversion(), _device->getAddress(), _id, _label, state);
  RLcomm.publishText(_device->getAddress(), UIDcode, _id, state, _device->getRLversion());
}
void Child::inputToLoRa(float state)
{
  DEBUGf("input to lora V%d (%02d/%02d) %s=%f\n", _device->getRLversion(), _device->getAddress(), _id, _label, state);
  RLcomm.publishFloat(_device->getAddress(), UIDcode, _id, state * 1000, 1000, _device->getRLversion());
}

bool Child::setState(int32_t newState)
{
  if (!_state)
  {
    _state = new int32_t(newState);
  }
  *reinterpret_cast<int32_t*>(_state) = newState;
  publishState();
  return true;
}

bool Child::setState(bool newState)
{
  return setState((int32_t)newState);
}

bool Child::setState(float newState)
{
  if (!_state)
  {
    _state = new float(newState);
  }
  *reinterpret_cast<float*>(_state) = newState;
  publishState();
  return true;
}

bool Child::setState(char* newState)
{
  if (!_state)
  {
    _state = malloc(RL_PACKET_SIZE + 1);
  }
  strcpy(reinterpret_cast<char*>(_state), newState);
  publishState();
  return true;
}

void Child::setSelectOptions(const char* options)
{
  if (_options) {
    delete _options;
    _options = nullptr;
  }
  _options = (char*)malloc(strlen(options) + 1);
  strcpy(_options, options);
  DEBUGf("   %d : OPT=%s\n", _id, _options);
}

char* Child::getSelectOptions()
{
  return _options;
}

void Child::setNumberConfig(int32_t mini, int32_t maxi, int16_t divider)
{
  _numberMini = mini;
  _numberMaxi = maxi;
  _numberDivider = divider;
  DEBUGf("   %d : mM=%d %d %d\n", _id, _numberMini, _numberMaxi, _numberDivider);
}
int32_t Child::getNumberMin()
{
  return _numberMini;
}
int32_t Child::getNumberMax()
{
  return _numberMaxi;
}
int16_t Child::getNumberDiv()
{
  return _numberDivider;
}

void Child::doPacketForHA(rl_packet_t p)
{
  byte sender = p.senderID;
  byte child = p.childID;
  rl_element_t st = (rl_element_t)((p.sensordataType >> 3) & 0x1F);
  rl_data_t dt = (rl_data_t)(p.sensordataType & 0x07);
  float fval;
  String sval;
  char tag[9] = {0};
  char tmp[RL_PACKET_SIZE];
  DEBUGf("Do packet from %d / %d : %s = %d (%02X)\n", sender, child, _HAuid, p.data.num.value, p.sensordataType);

  if (st != _elementType) // TODO
  {
    DEBUGf("**ERR Different sensor type %d / %d\n", (int)st, (int)_elementType);
  }

  switch ((int)_elementType)
  {
    case E_BINARYSENSOR:
      DEBUGf("Binary Sensor : %d\n", (int)(p.data.num.value > 0));
      setState(p.data.num.value > 0);
      break;
    case E_NUMERICSENSOR:
      DEBUGf("Numeric Sensor : %d\n", p.data.num.value);
      if (dt == D_FLOAT)
      {
        fval = p.data.num.value;
        if (p.data.num.divider != 0)
        {
          fval = (float)p.data.num.value / (float)p.data.num.divider;
        }
        fval = fval * _coefA + _coefB;
        if (fval >= _mini && fval <= _maxi)
        {
          setState(fval);
        }
      }
      else
      {
        if (p.data.num.value >= _mini && p.data.num.value <= _maxi)
        {
          setState(p.data.num.value);
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
      // TODO
      break;
    case E_FAN:
      // TODO
      break;
    case E_HVAC:
      // TODO
      break;
    case E_SELECT:
      memset(tmp, 0, sizeof(tmp));
      strncpy(tmp, p.data.text, sizeof(p.data.text));
      setState(tmp);
      break;
    case E_TRIGGER:
      DEBUGf("Trigger : %d\n", p.data.num.value);
      // Only binary trigger
      setState(p.data.num.value > 0);
      break;
    case E_CUSTOM:
      // TODO
      break;
    case E_TAG:
      byteArrayToStr(tag, p.data.rawByte, 4);
      //TODO ((HATagScanner*)_HAdevice)->tagScanned(tag);
      break;
    case E_TEXTSENSOR:
      memset(tmp, 0, sizeof(tmp));
      strncpy(tmp, p.data.text, sizeof(p.data.text));
      setState(tmp);
      break;
    case E_INPUTNUMBER:
      DEBUGf("Input Number : %d\n", p.data.num.value);
      if (dt == D_FLOAT)
      {
        fval = p.data.num.value;
        if (p.data.num.divider != 0)
        {
          fval = (float)p.data.num.value / (float)p.data.num.divider;
        }
        fval = fval * _coefA + _coefB;
        if (fval >= _mini && fval <= _maxi)
        {
          setState(fval);
        }
      }
      else
      {
        if (p.data.num.value >= _mini && p.data.num.value <= _maxi)
        {
          setState(p.data.num.value);
        }
      }
      break;
  }
}

void Child::publishConfig()
{
  JsonDocument docJSon;
  JsonObject Jconfig = docJSon.to<JsonObject>();
  JsonObject device = Jconfig["device"].to<JsonObject>();
  String devUID = String(_device->getName());
  devUID.replace(" ", "_");
  device[HAIdentifiers] = String(AP_ssid) + "_" + devUID;
  device[HAManufacturer] = "M&L";
  device[HAName] = String(_device->getName());
  if (_device->getModel()) {
    device[HAModel] = String(_device->getModel());
  }
  device[HAViaDevice] = "hub_" + String(hub.getUniqueId());

  String topic = String(HAConfigRoot) + "/";
  Jconfig[HAName] = String(_label);
  Jconfig[HAUniqueID] = String(hub.getUniqueId()) + "_" + String(_HAuid);
  Jconfig[HAStateTopic] = String(HAlora2ha) + "/" + String(hub.getUniqueId()) + "/" + String(_HAuid) + "/" + String(HAStateTopic);
  switch (_elementType)
  {
    case E_BINARYSENSOR:
      topic += "binary_sensor";
      if (_devclass && strlen(_devclass)) {
        Jconfig[HADeviceClass] = _devclass;
      }
      break;
    case E_NUMERICSENSOR:
      topic += "sensor";
      if (_devclass && strlen(_devclass)) {
        Jconfig[HADeviceClass] = _devclass;
      }
      if (_unit && strlen(_unit)) {
        Jconfig[HAUnitOfMeasurement] = _unit;
      }
      if (strcmp(_devclass, "energy") == 0)
      {
        Jconfig["state_class"] = "total_increasing";
      }
      break;
    case E_SWITCH:
      topic += "switch";
      Jconfig[HACommandTopic] = String(HAlora2ha) + "/"  + String(hub.getUniqueId()) + "/" + String(_HAuid) + "/command_topic";
      break;
    case E_LIGHT:
      topic += "light";
      Jconfig[HACommandTopic] = String(HAlora2ha) + "/" + String(hub.getUniqueId()) + "/" + String(_HAuid) + "/command_topic";
      break;
    case E_COVER:
      topic += "cover";
      Jconfig[HACommandTopic] = String(HAlora2ha) + "/"  + String(hub.getUniqueId()) + "/" + String(_HAuid) + "/command_topic";
      break;
    case E_FAN:
      topic += "fan";
      Jconfig[HACommandTopic] = String(HAlora2ha) + "/"  + String(hub.getUniqueId()) + "/" + String(_HAuid) + "/command_topic";
      break;
    case E_HVAC:
      topic += "hvac";
      break;
    case E_SELECT:
      topic += "select";
      if (_options) {
        char tmp[10] = {0};
        char* p = _options;
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
      Jconfig[HACommandTopic] = String(HAlora2ha) + "/"  + String(hub.getUniqueId()) + "/" + String(_HAuid) + "/command_topic";
      break;
    case E_TRIGGER:
      topic += "device_automation";
      Jconfig["atype"] = "trigger";
      Jconfig["type"] = "button_short_press";
      Jconfig["subtype"] = _HAuid;
      Jconfig["topic"] = String(HAlora2ha) + "/"  + String(hub.getUniqueId()) + "/button_short_press_" + String(_HAuid) + "/topic";
      break;
    case E_CUSTOM:
      // TODO
      break;
    case E_TAG:
      topic += "tag";
      break;
    case E_TEXTSENSOR:
      topic += "sensor";
      break;
    case E_INPUTNUMBER:
      topic += "number";
      Jconfig["min"] = serialized(String((float)getNumberMin() / getNumberDiv(), 3));
      Jconfig["max"] = serialized(String((float)getNumberMax() / getNumberDiv(), 3));
      Jconfig["step"] = serialized(String(1. / getNumberDiv(), 1));
      Jconfig[HACommandTopic] = String(HAlora2ha) + "/"  + String(hub.getUniqueId()) + "/" + String(_HAuid) + "/command_topic";
      break;
  }
  if (_category == CategoryDiagnostic)
  {
    Jconfig[HAEntityCategory] = "diagnostic";
  }
  if (_category == CategoryConfig)
  {
    Jconfig[HAEntityCategory] = "config";
  }
  if (_elementType == E_TRIGGER)
  {
    topic += "/" + String(hub.getUniqueId()) + "/button_short_press_" + String(_HAuid);
  } else {
    topic += "/" + String(hub.getUniqueId()) + "/" + String(_HAuid) + "/config";
  }
  //DEBUGln(topic);

  String payload;
  size_t Lres = serializeJson(docJSon, payload);
  //DEBUGln(payload);

  //mqtt.publish(topic.c_str(), (const uint8_t*)payload.c_str(), payload.length(), true);
  mqtt.beginPublish(topic.c_str(), payload.length(), true);
  mqtt.write((const uint8_t*)(payload.c_str()), payload.length());
  mqtt.endPublish();
}

void Child::publishState()
{
  String topic = "";
  String payload = "";

  topic = String(HAlora2ha) + "/" + String(hub.getUniqueId()) + "/" + String(_HAuid) + "/" + String(HAStateTopic);
  switch (_elementType)
  {
    case E_BINARYSENSOR:
      payload = *reinterpret_cast<int32_t*>(_state) > 0 ? HAStateOn : HAStateOff;
      break;
    case E_NUMERICSENSOR:
      if (_dataType == D_FLOAT)
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
      break;
    case E_FAN:
      break;
    case E_HVAC:
      break;
    case E_SELECT:
      payload = String(reinterpret_cast<char*>(_state));
      break;
    case E_TRIGGER:
      break;
    case E_CUSTOM:
      break;
    case E_TAG:
      break;
    case E_TEXTSENSOR:
      payload = String(reinterpret_cast<char*>(_state));
      break;
    case E_INPUTNUMBER:
      if (_dataType == D_FLOAT)
      {
        payload = String(*reinterpret_cast<float*>(_state));
      } else {
        payload = String(*reinterpret_cast<int32_t*>(_state));
      }
      break;
  }
  if (payload != "")
  {
    DEBUGf("Publish : %s\n", payload.c_str());
    mqtt.beginPublish(topic.c_str(), payload.length(), true);
    mqtt.write((const uint8_t*)(payload.c_str()), payload.length());
    mqtt.endPublish();
  }
}

void Child::onMessageMQTT(char* topic, char* payload)
{
  String command = String(HAlora2ha) + "/" + String(hub.getUniqueId()) + "/" + String(_HAuid) + "/" + String(HACommandTopic);
  //DEBUGf("%s\n%s\n", topic, command.c_str());
  if (String(topic) == command)
  {
    switch (_elementType)
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
      case E_CUSTOM:
        break;
      case E_TAG:
        break;
      case E_TEXTSENSOR:
        break;
      case E_INPUTNUMBER:
        inputToLoRa(String(payload).toFloat());
        break;
    }
  }
}

// ******************************************
