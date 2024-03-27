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
      _oldWeight = 0;
      // mandatory
      _deviceType = rl_device_t::S_NUMERICSENSOR;
      _dataType = rl_data_t::V_FLOAT;
    }
    virtual void begin() override {
      // sensor must be defined in previous included .hpp
      Wsensor.begin();
    }
    // Send lora packet if Delta (g) measured since last sent
    // Return Weight sent (or false if not Delta)
    uint32_t Send() override {
      int32_t weight;
      weight = Wsensor.readData(500); // 500ms timeout
      if ((weight > 0) && (abs(weight - _oldWeight) > _delta))
      {
        _oldWeight = weight;
        DEBUG(F(" >Weight")); DEBUGln(weight);
        publishFloat(weight, 1000, 1);
        return weight;
      }
      return false;
    }
  protected:
    uint8_t _pin;
    uint32_t _oldWeight; // gramme
};
