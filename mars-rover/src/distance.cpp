#include "distance.h"
#include <Arduino.h>
#include <VL53L0X.h>
#include <Wire.h>
#include "config.h"
#include "netlog.h"

namespace distance {

namespace {
VL53L0X sensor;
bool up = false;
float buffer = DIST_BUFFER_IN;
}  // namespace

void begin() {
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, I2C_HZ);
  sensor.setTimeout(DIST_TIMEOUT_MS);
  up = sensor.init();
  netlog::out.println(up ? "[distance] VL53L0X ready" : "[distance] VL53L0X not found");
}

bool ready() { return up; }

int readMm() {
  if (!up) return -1;
  uint16_t mm = sensor.readRangeSingleMillimeters();
  if (sensor.timeoutOccurred()) return -1;
  return mm >= OUT_OF_RANGE_MM ? OUT_OF_RANGE_MM : mm;
}

float clearanceIn(int mm) { return (mm - DIST_SENSOR_OFFSET_MM) / 25.4f; }
float bufferIn() { return buffer; }
void setBufferIn(float in) { buffer = in; }

}  // namespace distance
