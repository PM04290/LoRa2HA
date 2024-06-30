#ifndef KALMAN_H
#define KALMAN_H

class KalmanFilter
{
  public:
    explicit KalmanFilter(float lastestimate) {
		last_estimate = lastestimate;
    }
    float update(float newVal) {
      kalman_gain = err_estimate / (err_estimate + err_measure);
      current_estimate = last_estimate + kalman_gain * (newVal - last_estimate);
      err_estimate =  (1.0f - kalman_gain) * err_estimate + fabsf(last_estimate - current_estimate) * q;
      last_estimate = current_estimate;
      return current_estimate;
    }
  private:
    float q = 0.03;
    float kalman_gain = 0;
    float err_measure = 1;
    float err_estimate = 2;
    float current_estimate = 0;
    float last_estimate = 0;
};

#endif // KALMAN_H
