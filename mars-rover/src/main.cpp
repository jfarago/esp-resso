#include <Arduino.h>
#include <battery.h>
#include "config.h"

// XIAO ESP32-C6 user LED (GPIO15) is active-low; LED_BUILTIN comes from the board variant.
static const uint8_t LED_ON = LOW;
static const uint8_t LED_OFF = HIGH;

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LED_OFF);

  battery::Config bc;
  bc.pin = PIN_VBAT;
  bc.divider = VBAT_DIVIDER;
  bc.cal = VBAT_CAL;
  bc.samples = VBAT_SAMPLES;
  bc.sampleMs = VBAT_SAMPLE_MS;
  bc.smoothAlpha = VBAT_SMOOTH_ALPHA;
  bc.logMs = VBAT_LOG_MS;
  bc.presentMinV = VBAT_PRESENT_MIN_V;
  battery::begin(bc);
}

void loop() {
  battery::update(millis());
  digitalWrite(LED_BUILTIN, LED_ON);
  delay(500);
  digitalWrite(LED_BUILTIN, LED_OFF);
  delay(500);
}
