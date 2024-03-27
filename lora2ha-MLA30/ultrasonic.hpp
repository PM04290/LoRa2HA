/* Used with ultrasonic sensor
*/

// if filter needed
//#include "src/kalman.hpp"

class Ultrasonic : public MLsensor
{
  public:
    Ultrasonic(uint8_t childID) : MLsensor(childID) {
      _oldDistance = -1;
      // mandatory
      _deviceType = rl_device_t::S_NUMERICSENSOR;
      _dataType = rl_data_t::V_NUM;
    }
    void begin () override {
      pinMode(PIN_IO_1, OUTPUT);
      pinMode(PIN_IO_2, INPUT);
    }
    // Send lora packet if Delta measured since last sent
    // Return Distance sent (or false if not Delsta)
    uint32_t Send(int delta = 10) override {
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
      // too much difference between 2 measure
      //distance = _KalmanUS.update( distance );
      
      if (abs(distance_mm - _oldDistance) > delta)
      {
        _oldDistance = distance_mm;
        DEBUG(F(" >Dist")); DEBUGln(distance_mm);
        publishNum(distance_mm);
        return distance_mm;
      }
      return false;
    }
  protected:
    int _oldDistance;
    //KalmanFilter _KalmanUS;
};
