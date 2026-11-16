// TB6612FNG dual motor driver: left = channel A, right = channel B.
#pragma once

namespace motors {

void begin();                      // pins set up, driver stays in standby until set() is called
void set(float left, float right);  // -1..1 (negative = reverse); enables the driver
void stop();                       // coast, then put the driver in standby

}  // namespace motors
