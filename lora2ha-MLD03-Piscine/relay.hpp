//
class EltRelay : public Element
{
  public:
    EltRelay(uint8_t pin, uint8_t childID, const __FlashStringHelper* name)
      : Element(childID, EEP_NONE, 0, name, nullptr, 1) {
      _pin = pin;
      // mandatory
      _elementType = rl_element_t::E_SWITCH;
      _dataType = rl_data_t::D_BOOL;
      pinMode(_pin, OUTPUT);
    }
    int32_t getValue() override {
      return digitalRead(_pin);
    }
    uint8_t getBool() override {
      return digitalRead(_pin);
    }
    void setValue(int32_t newValue) override {
      if (newValue != _curValue) {
        digitalWrite(_pin, newValue > 0);
        _curValue = newValue;
        DEBUG(_name); DEBUG(" = "); DEBUGln(newValue > 0 ? "ON" : "OFF");
      }
    }
    void setBool(uint8_t newValue) override {
      setValue((uint32_t)newValue);
    }
    void Process() override {
      // nothing to do for relay
    }
  private:
    uint8_t _pin;
};
