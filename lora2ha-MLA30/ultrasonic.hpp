/* Used with ultrasonic sensor
*/

// if filter needed
//#include "src/kalman.hpp"

class Ultrasonic : public MLsensor
{
  public:
    Ultrasonic(uint8_t childID, uint16_t delta) : MLsensor(childID, delta) {
      _oldDistance = -1;
      // mandatory
      _deviceType = rl_element_t::E_NUMERICSENSOR;
      _dataType = rl_data_t::D_NUM;
    }
    void begin () override {
      pinMode(PIN_IO_1, OUTPUT);
      pinMode(PIN_IO_2, INPUT);
    }
    uint32_t Send() override {
      uint8_t force = --_forceRefresh <= 0;
      
      // make 10µs pulse on ouput pulse pin
      digitalWrite(PIN_IO_1, LOW);
      delayMicroseconds(5);
      digitalWrite(PIN_IO_1, HIGH);
      delayMicroseconds(10);
      digitalWrite(PIN_IO_1, LOW);

      // measure time for response
      uint32_t duration = pulseIn(PIN_IO_2, HIGH, 100); // timeout 100ms = 17m maxi
      int distance_mm = (float)duration * (340. / 1000.) / 2.;

      // it is possible to use Kalman filter if there is
      // too much difference between 2 measures
      //distance = _KalmanUS.update( distance );
      
      if (abs(distance_mm - _oldDistance) > _delta || force)
      {
        _oldDistance = distance_mm;
        DEBUG(F(" >Dist")); DEBUGln(distance_mm);
        publishNum(distance_mm);
        _forceRefresh = FORCE_REFRESH_COUNT;
        return distance_mm;
      }
      return false;
    }
  protected:
    int _oldDistance;
    //KalmanFilter _KalmanUS;
};
