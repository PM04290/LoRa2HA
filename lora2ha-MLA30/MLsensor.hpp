rl_packets _packet;

class MLsensor
{
  public:
    MLsensor(uint8_t id, uint16_t delta) {
      _id = id;
      _deviceType = rl_device_t::S_CUSTOM;
      _dataType = rl_data_t::V_NUM;
      _delta = delta;
    }
    virtual void begin() {
    }
    virtual uint32_t Send() {
    }
    void publishConfig() {
      rl_config_t cnf;
      memset(&cnf, 0, sizeof(cnf));
      cnf.childID = _id;
      cnf.deviceType = (uint8_t)_deviceType;
      cnf.dataType = (uint8_t)_dataType;
      RLcomm.publishConfig(HUB_UID, SENSOR_ID, CHILD_PARAM, cnf);
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
      _packet.current.data.num.divider = 0;
      _packet.current.data.num.precision = 0;
      publishPacket();
    }
    void publishFloat(float floatValue, int divider, byte precision) {
      _packet.current.data.num.value = floatValue;
      _packet.current.data.num.divider = divider;
      _packet.current.data.num.precision = precision;
      publishPacket();
    }
  protected:
    uint8_t _id;
    rl_device_t _deviceType;
    rl_data_t _dataType;
    uint16_t _delta; 
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
