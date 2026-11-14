#include "ota.h"
#include <Arduino.h>
#include <ArduinoOTA.h>
#include "config.h"
#include "netlog.h"
#include "net.h"
#include "secrets.h"

namespace ota {

static bool started = false;

static void start() {
  ArduinoOTA.setHostname(MDNS_HOSTNAME);
  ArduinoOTA.setPassword(OTA_PASSWORD);
  ArduinoOTA.setMdnsEnabled(false);  // net.cpp owns mDNS
  ArduinoOTA.onStart([]() { netlog::out.println("[ota] start"); });
  ArduinoOTA.onEnd([]() { netlog::out.println("[ota] done"); });
  ArduinoOTA.onError([](ota_error_t e) { netlog::out.printf("[ota] error %u\n", (unsigned)e); });
  ArduinoOTA.begin();
  started = true;
  netlog::out.println("[ota] ready");
}

void loop() {
  if (!started) {
    if (net::connected()) start();
    return;
  }
  ArduinoOTA.handle();
}

}  // namespace ota
