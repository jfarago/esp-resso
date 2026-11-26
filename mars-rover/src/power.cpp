#include "power.h"
#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>
#include <driver/gpio.h>
#include <esp_sleep.h>
#include "config.h"
#include "distance.h"
#include "drivemode.h"
#include "light.h"
#include "net.h"
#include "netlog.h"

namespace power {

namespace {

uint32_t lastActiveMs = 0;
bool idle = false;
bool forced = false;
uint32_t wifiWakes = 0;
bool sensorsPowered = false;

}  // namespace

void begin() {
  // MAX_MODEM wakes for every Nth beacon (listen interval, SDK default 3 = ~300 ms) instead of every
  // DTIM beacon. Costs up to that much latency on the first nc/OTA packet after idle.
  WiFi.setSleep(WIFI_PS_MAX_MODEM);
  lastActiveMs = millis();
}

void setForced(bool f) {
  forced = f;
  lastActiveMs = millis();  // leaves time to flush the reply before sleeping
}

bool sleeping() { return idle; }

uint32_t takeWifiWakes() {
  uint32_t n = wifiWakes;
  wifiWakes = 0;
  return n;
}

namespace {

void sensorsOff() {
  // Let go of the I2C pins first so they can't feed the unpowered sensors.
  Wire.end();
  pinMode(PIN_I2C_SDA, INPUT);
  pinMode(PIN_I2C_SCL, INPUT);
  pinMode(PIN_PERIPH_PWR, OUTPUT);
  digitalWrite(PIN_PERIPH_PWR, LOW);
  gpio_sleep_sel_dis((gpio_num_t)PIN_PERIPH_PWR);  // stay driven LOW during light sleep
  gpio_hold_en((gpio_num_t)PIN_PERIPH_PWR);        // and through deep sleep
  sensorsPowered = false;
}

}  // namespace

void sensorsWake() {
  if (sensorsPowered) {
    if ((distance::ready() && light::ready()) || drivemode::active()) return;
    sensorsOff();
    delay(PERIPH_PWR_SETTLE_MS);
  }
  gpio_hold_dis((gpio_num_t)PIN_PERIPH_PWR);
  pinMode(PIN_PERIPH_PWR, OUTPUT);
  gpio_set_drive_capability((gpio_num_t)PIN_PERIPH_PWR, GPIO_DRIVE_CAP_3);
  gpio_sleep_sel_dis((gpio_num_t)PIN_PERIPH_PWR);  // keep driving HIGH during light sleep
  digitalWrite(PIN_PERIPH_PWR, HIGH);
  delay(PERIPH_PWR_SETTLE_MS);
  sensorsPowered = true;
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, I2C_HZ);
  distance::begin();
  light::begin();
  delay(LIGHT_FIRST_READ_MS);  // a BH1750 reads 0 until its first measurement finishes
}

void deepSleepQuiet(uint32_t ms) {
  sensorsOff();
  esp_sleep_enable_timer_wakeup((uint64_t)ms * 1000);
  esp_deep_sleep_start();
}

void deepSleep(uint32_t ms) {
  netlog::loop();
  delay(300);  // let the reply reach the nc client
  Serial.flush();
  WiFi.disconnect(true);
  deepSleepQuiet(ms);
}

void loop(uint32_t now, bool busy) {
  // Without Wi-Fi there is nothing to wake for (and the reconnect timer needs the CPU), so stay awake.
  uint32_t idleMs = forced ? POWER_WAKE_WINDOW_MS : POWER_IDLE_MS;
  if (busy || !net::connected()) {
    if (idle) {
      char ts[16];
      netlog::stamp(ts, sizeof(ts));
      netlog::out.printf("%s [power] active, light sleep off\n", ts);
    }
    idle = false;
    lastActiveMs = now;
    return;
  }
  if (now - lastActiveMs < idleMs) return;
  if (!idle) {
    idle = true;
    char ts[16];
    netlog::stamp(ts, sizeof(ts));
    netlog::out.printf("%s [power] %s, light sleep (wake on Wi-Fi or every %lu s)\n", ts,
                       forced ? "sleep mode" : "idle", (unsigned long)(POWER_SLEEP_TIMER_MS / 1000));
  }

  if (sensorsPowered) sensorsOff();  // stay off across wakes; sensorsWake() restores them on demand

  Serial.flush();
  esp_sleep_enable_timer_wakeup((uint64_t)POWER_SLEEP_TIMER_MS * 1000);
  esp_sleep_enable_wifi_wakeup();
  esp_light_sleep_start();

  esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
  if (cause == ESP_SLEEP_WAKEUP_WIFI) {
    // Traffic for us (maybe a new nc connection): stay awake long enough for lwIP/loop() to see it.
    lastActiveMs = millis() - idleMs + POWER_WAKE_WINDOW_MS;
    wifiWakes++;
  }
}

}  // namespace power
