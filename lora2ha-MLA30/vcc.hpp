
class VCC : public MLsensor
{
  public:
    VCC(uint8_t childID) : MLsensor(childID) {
      oldvBat = 0;
      // mandatory
      _deviceType = rl_device_t::S_NUMERICSENSOR;
      _dataType = rl_data_t::V_FLOAT;
    }
    void begin() override {
    }
    virtual uint32_t Send()
    {
      mVCC = readVcc();
      if (mVCC != oldvBat)
      {
        oldvBat = mVCC;
        DEBUGln(F(" > VCC"));
        publishFloat(mVCC, 1000, 1);
      }
      return mVCC;
    }
  protected:
    // Measure internal VCC
    long readVcc() {
      // Read 1.1V reference against AVcc
      // set the reference to Vcc and the measurement to the internal 1.1V reference
#if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
      ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#elif defined (__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
      ADMUX = _BV(MUX5) | _BV(MUX0) ;
#else
      ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#endif
      delay(2); // Wait for Vref to settle
      ADCSRA |= _BV(ADSC); // Convert
      while (bit_is_set(ADCSRA, ADSC));
      long result = ADCL;
      result |= ADCH << 8;
      result = 1126400L / result; // Back-calculate AVcc in mV
      return result;
    }
  private:
    uint16_t oldvBat;
};
