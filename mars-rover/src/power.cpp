#include "power.h"
#include <Arduino.h>
#include <WiFi.h>
#include <esp_sleep.h>
#include "config.h"
#include "net.h"
#include "netlog.h"

namespace power {

namespace {

uint32_t lastActiveMs = 0;
bool idle = false;
bool forced = false;
uint32_t wifiWakes = 0;

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

void deepSleep(uint32_t ms) {
  netlog::loop();
  delay(300);  // let the reply reach the nc client
  Serial.flush();
  WiFi.disconnect(true);
  esp_sleep_enable_timer_wakeup((uint64_t)ms * 1000);
  esp_deep_sleep_start();
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
