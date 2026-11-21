#include "net.h"
#include <Arduino.h>
#include <ESPmDNS.h>
#include <WiFi.h>
#include "config.h"
#include "netlog.h"
#include "secrets.h"

namespace net {

static bool wasConnected = false;
static bool mdnsUp = false;
static uint32_t nextAttemptMs = 0;
static uint32_t backoffMs = WIFI_RETRY_MIN_MS;
static volatile uint8_t lastDisconnectReason = 0;  // written from the Wi-Fi event task

static void attempt(uint32_t now) {
  netlog::out.printf("[net] connecting to \"%s\" (next retry in %lu s)\n", WIFI_SSID,
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
  WiFi.onEvent(
      [](arduino_event_id_t, arduino_event_info_t info) {
        lastDisconnectReason = info.wifi_sta_disconnected.reason;
      },
      ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
  attempt(millis());
}

void loop(uint32_t now) {
  bool up = WiFi.status() == WL_CONNECTED;
  if (up && !wasConnected) {
    netlog::out.printf("[net] connected, IP %s, RSSI %d dBm\n", WiFi.localIP().toString().c_str(),
                  WiFi.RSSI());
    backoffMs = WIFI_RETRY_MIN_MS;
    if (!mdnsUp && MDNS.begin(MDNS_HOSTNAME)) {
      mdnsUp = true;
      netlog::out.printf("[net] mDNS %s.local\n", MDNS_HOSTNAME);
    }
  } else if (!up && wasConnected) {
    uint8_t r = lastDisconnectReason;
    netlog::out.printf("[net] Wi-Fi lost (reason %u: %s)\n", r,
                       WiFi.disconnectReasonName((wifi_err_reason_t)r));
    nextAttemptMs = now + 1000;  // first retry quickly, then back off
  }
  wasConnected = up;
  if (!up && (int32_t)(now - nextAttemptMs) >= 0) attempt(now);
}

bool connected() { return wasConnected; }

}  // namespace net
