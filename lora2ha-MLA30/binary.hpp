class BinaryIO : public MLsensor
{
  public:
    BinaryIO(uint8_t pin, uint8_t childID, uint8_t trigger = false, uint8_t inverted = false) : MLsensor(childID) {
      _pin = pin;
      _inverted = inverted;
      _trigger = trigger;
      _curState = 0xFF;
      _oldState = 0xFF;
      // mandatory
      if (trigger) {
        _deviceType = rl_device_t::S_TRIGGER;
      } else {
        _deviceType = rl_device_t::S_BINARYSENSOR;
      }
      _dataType = rl_data_t::V_BOOL;
    }
    void begin() override {
    }
    uint32_t Send(int delta = 10) override {
      // NOTE : there is no debounce process in this example, you
      // may put a capacitor (10nF) on entry if you want it, or make a dedicated code

      // Send digital sensor if change (Pull-up input)
      // Normaly opened sensor
      uint8_t _curState = (digitalRead(_pin) == LOW);
      if (_curState != _oldState) // no debounce !
      {
        _oldState = _curState;
        DEBUG(" > Bin "); DEBUGln(_pin);
        if (_trigger) {
          publishTrigger(_inverted ? !_curState : _curState);
        } else {
          publishBool(_inverted ? !_curState : _curState);
        }
      }
    }
  protected:
    uint8_t _pin;
    uint8_t _curState;
    uint8_t _oldState;
    uint8_t _inverted;
    uint8_t _trigger;
};
