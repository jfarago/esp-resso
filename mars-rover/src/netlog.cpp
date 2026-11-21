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
    lineLen = 0;
    lineOverflow = false;
    size_t start = (head + LOG_BACKLOG_BYTES - count) % LOG_BACKLOG_BYTES;
    for (size_t i = 0; i < count; i++) client.write(ring[(start + i) % LOG_BACKLOG_BYTES]);
  }

  bool connected() { return client && client.connected(); }

  // Non-blocking: collects bytes from the client; true once a full line is in buf.
  bool readLine(char* buf, size_t n) {
    while (client && client.available()) {
      int c = client.read();
      if (c == '\n') {
        bool ok = !lineOverflow;
        line[lineLen] = 0;
        if (ok) strlcpy(buf, line, n);
        lineLen = 0;
        lineOverflow = false;
        if (ok) return true;
      } else if (c >= 32 && c < 127) {  // drops \r and telnet negotiation bytes
        if (lineLen < sizeof(line) - 1) line[lineLen++] = (char)c;
        else lineOverflow = true;
      }
    }
    return false;
  }

 private:
  char line[96];
  size_t lineLen = 0;
  bool lineOverflow = false;
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

void stamp(char* buf, size_t n) {
  uint32_t s = millis() / 1000;
  snprintf(buf, n, "[%02lu:%02lu:%02lu]", (unsigned long)(s / 3600), (unsigned long)(s / 60 % 60),
           (unsigned long)(s % 60));
}

bool clientConnected() { return tee.connected(); }
bool readLine(char* buf, size_t n) { return tee.readLine(buf, n); }

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
