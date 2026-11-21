#include "light.h"
#include <Arduino.h>
#include <Wire.h>
#include "config.h"
#include "netlog.h"

namespace light {

namespace {

bool up = false;

bool start(uint8_t addr) {
  Wire.beginTransmission(addr);
  Wire.write(0x01);  // power on
  if (Wire.endTransmission() != 0) return false;
  Wire.beginTransmission(addr);
  Wire.write(0x10);  // continuous high-resolution mode, 120 ms per measurement
  return Wire.endTransmission() == 0;
}

bool readOne(uint8_t addr, float& lux) {
  if (Wire.requestFrom(addr, (uint8_t)2) != 2) return false;
  uint16_t raw = (uint16_t)Wire.read() << 8;
  raw |= Wire.read();
  lux = raw / 1.2f;
  return true;
}

}  // namespace

void begin() {
  bool l = start(LIGHT_ADDR_LEFT), r = start(LIGHT_ADDR_RIGHT);
  up = l && r;
  netlog::out.printf("[light] BH1750 left %s, right %s\n", l ? "ready" : "missing", r ? "ready" : "missing");
}

bool ready() { return up; }

bool read(float& left, float& right) {
  return up && readOne(LIGHT_ADDR_LEFT, left) && readOne(LIGHT_ADDR_RIGHT, right);
}

}  // namespace light
