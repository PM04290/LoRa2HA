
class VCC : public MLsensor
{
  public:
    VCC(uint8_t childID) : MLsensor(childID, 5) {
      oldvBat = 0;
      // mandatory
      _deviceType = rl_device_t::S_NUMERICSENSOR;
      _dataType = rl_data_t::V_FLOAT;
    }
    void begin() override {
    }
    uint32_t Send() override
    {
      mVCC = readVcc();
      if (abs(mVCC - oldvBat) > _delta)
      {
        oldvBat = mVCC;
        DEBUG(F(" >Vcc")); DEBUGln(mVCC);
        publishFloat(mVCC, 1000, 1);
      }
      return mVCC;
    }
  protected:
    // Measure internal VCC
    uint16_t readVcc() {
      long result;
      uint8_t saveADMUX = ADMUX;
      // Read 1.1V reference against AVcc
      // set the reference to Vcc and the measurement to the internal 1.1V reference
#if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
      ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#elif defined (__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
      ADMUX = _BV(MUX5) | _BV(MUX0);
#else
      ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#endif
      ADCSRA |= (1 << ADEN); //  Enable ADC
      delay(2); // Wait for Vref to settle
      for (uint8_t n = 0; n < 8; n++) {
        ADCSRA |= _BV(ADSC); // Convert
        while (bit_is_set(ADCSRA, ADSC));
        result = ADCW;
        result = 1100L * 1023L / result; // Back-calculate AVcc in mV
      }
      ADMUX = saveADMUX;
      delay(2); // Wait for Vref to settle
      return result;
    }
  private:
    uint16_t oldvBat;
};
