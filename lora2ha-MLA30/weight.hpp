#if defined(WITH_WEIGHT_2) && WITH_WEIGHT_2 == W_HX711
#include "src/weight_hx711.hpp"
HX711 Wsensor(PIN_IO_2, PIN_IO_1);
#endif

#if defined(WITH_WEIGHT_0) || defined(WITH_WEIGHT_1) || (defined(WITH_WEIGHT_2) && WITH_WEIGHT_0 == PIN_IO_0)
#include "src/weight_analog.hpp"
#if defined(WITH_WEIGHT_0) && WITH_WEIGHT_0 == PIN_IO_0
WeightAnalog Wsensor(PIN_IO_0);
#endif
#if defined(WITH_WEIGHT_1) && WITH_WEIGHT_1 == PIN_IO_1
WeightAnalog Wsensor(PIN_IO_1);
#endif
#if defined(WITH_WEIGHT_2) && WITH_WEIGHT_2 == PIN_IO_2
WeightAnalog Wsensor(PIN_IO_2);
#endif
#endif

class Weight : public MLsensor {
  public:
    Weight(uint8_t childID, uint8_t pin, uint16_t delta) : MLsensor(childID, delta) {
      _pin = pin;
      _oldWeight = -999;
      // mandatory
      _deviceType = rl_element_t::E_NUMERICSENSOR;
      _dataType = rl_data_t::D_FLOAT;
    }
    void begin() override {
      // Wsensor must be defined in previous included .hpp
      Wsensor.begin();
    }
    uint32_t Send() override {
      uint8_t force = --_forceRefresh <= 0;

      int32_t weight = Wsensor.readData(500); // 500ms timeout
      if (abs(weight - _oldWeight) > _delta || force)
      {
        _oldWeight = weight;
        DEBUG(F(" >Weight")); DEBUGln(weight);
        publishFloat(weight, 1000);
        _forceRefresh = FORCE_REFRESH_COUNT;
        return weight;
      }
      return false;
    }
  protected:
    uint8_t _pin;
    int32_t _oldWeight; // gramme
};
