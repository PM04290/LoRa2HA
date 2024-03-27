// if filter needed
//#include "src/kalman.hpp"

// Measure lux with Photoresistor

#define REF_RESISTANCE      100000
#define L1 2000      // forte lumière
#define R1 100000
#define L2 10        // faible lumière
#define R2 20000000

class Photoresistor : public MLsensor
{
  public:
    Photoresistor(uint8_t childID, uint8_t pin, uint16_t delta) : MLsensor(childID, delta) {
      _pin = pin;
      _coef_m = NAN;
      _coef_b = NAN;
      oldLux = 0;
      // mandatory
      _deviceType = rl_device_t::S_NUMERICSENSOR;
      _dataType = rl_data_t::V_NUM;
      if (_pin <= PIN_IO_2) {
        pinMode(_pin, INPUT);
      }
    }
    void begin() override {
      if (isnan(_coef_m)) {
        _coef_m = (log10(L2) - log10(L1)) / (log10(R2) - log10(R1));
        _coef_b =  log10(L1) - (_coef_m * log10(R1));
      }
    }
    uint32_t Send() override
    {
      uint16_t valLux = ADCtoLux(analogRead(_pin));
      if (abs(valLux - oldLux) > _delta)
      {
        oldLux = valLux;
        DEBUG(" >Photo"); DEBUGln(valLux);
        publishNum(valLux);
        return valLux;
      }
      return false;
    }
  protected:
    uint32_t ADCtoLux(int ldrRaw)
    {
      // for LDR on GND
      uint32_t mvLDR = (ldrRaw * (long)mVCC) / 1023;
      uint32_t ldrResistance = (mvLDR) * REF_RESISTANCE / (mVCC - mvLDR);

      float lux = pow(10, _coef_b) * pow(ldrResistance, _coef_m);

      // if filter needed
      //lux = _kalman.update( lux );

      return lux;
    }
  private:
    uint8_t _pin;
    float _coef_m;
    float _coef_b;
    int16_t oldLux;
    //KalmanFilter _kalman;
};
