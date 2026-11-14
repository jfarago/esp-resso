#include "netlog.h"
#include <Arduino.h>
#include <WiFi.h>
#include "config.h"
#include "net.h"

namespace netlog {

namespace {

// Keeps the last LOG_BACKLOG_BYTES written so a new client sees recent history.
class Tee : public Print {
 public:
  size_t write(uint8_t c) override { return write(&c, 1); }
  size_t write(const uint8_t* buf, size_t len) override {
    Serial.write(buf, len);
    for (size_t i = 0; i < len; i++) {
      ring[head] = buf[i];
      head = (head + 1) % LOG_BACKLOG_BYTES;
      if (count < LOG_BACKLOG_BYTES) count++;
    }
    if (client && client.connected()) client.write(buf, len);
    return len;
  }

  void attach(WiFiClient c) {
    if (client) client.stop();
    client = c;
    size_t start = (head + LOG_BACKLOG_BYTES - count) % LOG_BACKLOG_BYTES;
    for (size_t i = 0; i < count; i++) client.write(ring[(start + i) % LOG_BACKLOG_BYTES]);
  }

 private:
  uint8_t ring[LOG_BACKLOG_BYTES];
  size_t head = 0, count = 0;
  WiFiClient client;
};

Tee tee;
WiFiServer server(LOG_TCP_PORT);
bool serverUp = false;

}  // namespace

Print& out = tee;

void begin() {}

void loop() {
  if (!serverUp) {
    if (!net::connected()) return;
    server.begin();
    serverUp = true;
    out.printf("[netlog] listening on port %d\n", LOG_TCP_PORT);
  }
  WiFiClient c = server.accept();
  if (c) tee.attach(c);
}

}  // namespace netlog
