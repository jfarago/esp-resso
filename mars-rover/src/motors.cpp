#include "motors.h"
#include <Arduino.h>
#include "config.h"

namespace motors {

static const uint8_t PWM_BITS = 8;

static void drive(int in1, int in2, int pwm, float v) {
  v = constrain(v, -1.0f, 1.0f);
  digitalWrite(in1, v > 0 ? HIGH : LOW);
  digitalWrite(in2, v < 0 ? HIGH : LOW);
  ledcWrite(pwm, (uint32_t)(fabsf(v) * ((1 << PWM_BITS) - 1) + 0.5f));
}

void begin() {
  pinMode(PIN_MOT_STBY, OUTPUT);
  digitalWrite(PIN_MOT_STBY, LOW);
  const int ins[] = {PIN_MOT_AIN1, PIN_MOT_AIN2, PIN_MOT_BIN1, PIN_MOT_BIN2};
  for (int p : ins) {
    pinMode(p, OUTPUT);
    digitalWrite(p, LOW);
  }
  ledcAttach(PIN_MOT_PWMA, MOTOR_PWM_HZ, PWM_BITS);
  ledcAttach(PIN_MOT_PWMB, MOTOR_PWM_HZ, PWM_BITS);
  ledcWrite(PIN_MOT_PWMA, 0);
  ledcWrite(PIN_MOT_PWMB, 0);
}

void set(float left, float right) {
  drive(PIN_MOT_AIN1, PIN_MOT_AIN2, PIN_MOT_PWMA, MOTOR_LEFT_INVERT ? -left : left);
  drive(PIN_MOT_BIN1, PIN_MOT_BIN2, PIN_MOT_PWMB, MOTOR_RIGHT_INVERT ? -right : right);
  digitalWrite(PIN_MOT_STBY, HIGH);
}

void stop() {
  drive(PIN_MOT_AIN1, PIN_MOT_AIN2, PIN_MOT_PWMA, 0);
  drive(PIN_MOT_BIN1, PIN_MOT_BIN2, PIN_MOT_PWMB, 0);
  digitalWrite(PIN_MOT_STBY, LOW);
}

}  // namespace motors
