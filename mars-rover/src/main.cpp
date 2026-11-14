#include <Arduino.h>

// XIAO ESP32-C6 user LED (GPIO15) is active-low; LED_BUILTIN comes from the board variant.
static const uint8_t LED_ON = LOW;
static const uint8_t LED_OFF = HIGH;

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LED_OFF);
}

void loop() {
  digitalWrite(LED_BUILTIN, LED_ON);
  Serial.println("LED on");
  delay(500);
  digitalWrite(LED_BUILTIN, LED_OFF);
  Serial.println("LED off");
  delay(500);
}
