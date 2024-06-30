//

class Binary : public Element
{
  public:
    Binary(uint8_t pin, uint8_t childID, const __FlashStringHelper* name, uint8_t inverted)
      : Element(childID, EEP_NONE, 0, name, nullptr, 1) {
      _curValue = -999999;
      _pin = pin;
      _inverted = inverted;
      pinMode(_pin, INPUT_PULLUP);
      // mandatory
      _elementType = rl_element_t::E_BINARYSENSOR;
      _dataType = rl_data_t::D_BOOL;
    }
    void Process() override {
      setValue(_inverted ? !digitalRead(_pin) : digitalRead(_pin));
    }
  private:
    uint8_t _pin;
    uint8_t _inverted;
};
