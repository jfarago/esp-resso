#include "net.h"
#include <Arduino.h>
#include <ESPmDNS.h>
#include <WiFi.h>
#include "config.h"
#include "secrets.h"

namespace net {

static bool wasConnected = false;
static bool mdnsUp = false;
static uint32_t nextAttemptMs = 0;
static uint32_t backoffMs = WIFI_RETRY_MIN_MS;

static void attempt(uint32_t now) {
  Serial.printf("[net] connecting to \"%s\" (next retry in %lu s)\n", WIFI_SSID,
                (unsigned long)(backoffMs / 1000));
  WiFi.disconnect(false, false);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  nextAttemptMs = now + backoffMs;
  backoffMs = min(backoffMs * 2, WIFI_RETRY_MAX_MS);
}

void begin() {
  WiFi.persistent(false);  // credentials are compile-time; don't wear flash
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(MDNS_HOSTNAME);
  WiFi.setAutoReconnect(false);  // reconnects are driven by loop() with backoff
  attempt(millis());
}

void loop(uint32_t now) {
  bool up = WiFi.status() == WL_CONNECTED;
  if (up && !wasConnected) {
    Serial.printf("[net] connected, IP %s, RSSI %d dBm\n", WiFi.localIP().toString().c_str(),
                  WiFi.RSSI());
    backoffMs = WIFI_RETRY_MIN_MS;
    if (!mdnsUp && MDNS.begin(MDNS_HOSTNAME)) {
      mdnsUp = true;
      Serial.printf("[net] mDNS %s.local\n", MDNS_HOSTNAME);
    }
  } else if (!up && wasConnected) {
    Serial.println("[net] Wi-Fi lost");
    nextAttemptMs = now + 1000;  // first retry quickly, then back off
  }
  wasConnected = up;
  if (!up && (int32_t)(now - nextAttemptMs) >= 0) attempt(now);
}

bool connected() { return wasConnected; }

}  // namespace net
