rl_packets _packet;

#define FORCE_REFRESH_COUNT 120 // force refresh every 2 Hours (depend of sleep time, 120 for 1mn sleep)

class MLsensor
{
  public:
    MLsensor(uint8_t id, uint16_t delta) {
      _id = id;
      _deviceType = rl_element_t::E_CUSTOM;
      _dataType = rl_data_t::D_NUM;
      _delta = delta;
      _forceRefresh = FORCE_REFRESH_COUNT;
    }
    virtual void begin() {
    }
    virtual uint32_t Send() {
    }
    void publishConfig() {
      rl_configs_t cnf;
      memset(&cnf, 0, sizeof(cnf));
      cnf.base.childID = _id;
      cnf.base.deviceType = (uint8_t)_deviceType;
      cnf.base.dataType = (uint8_t)_dataType;
      // TODO : Name
      RLcomm.publishConfig(HUB_UID, SENSOR_ID, &cnf, C_BASE);
    }
    void publishPacket() {
      _packet.current.destinationID = HUB_UID;
      _packet.current.senderID = SENSOR_ID;
      _packet.current.childID = _id;
      _packet.current.sensordataType = (_deviceType << 3) + _dataType;
      RLcomm.publishPaquet(&_packet);
    }
    void publishBool(uint8_t boolValue) {
      _packet.current.data.num.value = boolValue;
      publishPacket();
    }
    void publishTrigger(uint8_t boolValue) {
      _packet.current.data.num.value = boolValue;
      publishPacket();
    }
    void publishNum(long longValue) {
      _packet.current.data.num.value = longValue;
      _packet.current.data.num.divider = 1;
      publishPacket();
    }
    void publishFloat(float floatValue, int divider) {
      _packet.current.data.num.value = floatValue;
      _packet.current.data.num.divider = divider;
      publishPacket();
    }
  protected:
    uint8_t _id;
    rl_element_t _deviceType;
    rl_data_t _dataType;
    uint16_t _delta;
    int16_t _forceRefresh;
};

MLsensor** _sensorList;
uint8_t _sensorCount = 0;

MLsensor* ML_addSensor(MLsensor* newSensor) {
  _sensorList = (MLsensor**)realloc(_sensorList, (_sensorCount + 1) * sizeof(MLsensor*));
  if (_sensorList == nullptr) {
    return nullptr;
  }
  _sensorList[_sensorCount] = newSensor;
  _sensorCount++;
  return _sensorList[_sensorCount - 1];
}
void ML_begin() {
  for (uint8_t i = 0; i < _sensorCount; i++)
  {
    ((MLsensor*)_sensorList[i])->begin();
  }
}
void ML_SendSensors() {
  for (uint8_t i = 0; i < _sensorCount; i++)
  {
    ((MLsensor*)_sensorList[i])->Send();
  }
}
void ML_PublishConfigSensors() {
  for (uint8_t i = 0; i < _sensorCount; i++)
  {
    ((MLsensor*)_sensorList[i])->publishConfig();
  }
}
