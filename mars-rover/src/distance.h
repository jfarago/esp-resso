#pragma once

namespace distance {

void begin();     // starts I2C and the VL53L0X; logs and stays disabled if the sensor is missing
bool ready();     // sensor initialised
int readMm();     // single-shot range in mm, or -1 on timeout / out of range

}  // namespace distance
