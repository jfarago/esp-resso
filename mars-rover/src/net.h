// Wi-Fi STA with reconnect backoff and mDNS.
#pragma once
#include <stdint.h>

namespace net {

void begin();
void loop(uint32_t nowMs);  // call every loop
bool connected();           // STA has an IP

}  // namespace net
