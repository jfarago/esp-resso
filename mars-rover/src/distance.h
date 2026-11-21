#pragma once

namespace distance {

void begin();     // starts I2C and the VL53L0X; logs and stays disabled if the sensor is missing
bool ready();     // sensor initialised
constexpr int OUT_OF_RANGE_MM = 8190;  // readMm() value when nothing is in range

int readMm();     // single-shot range in mm (OUT_OF_RANGE_MM if nothing in range), or -1 on failure/timeout

// Space between the rover's front and the obstacle, in inches (negative if the reading is below the offset).
float clearanceIn(int mm);

float bufferIn();           // clearance to keep from obstacles
void setBufferIn(float in);

}  // namespace distance
