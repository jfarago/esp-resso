// Log output mirrored to USB serial and one TCP client (`nc mars-rover.local 23`).
#pragma once
#include <Print.h>

namespace netlog {

extern Print& out;  // use like Serial: netlog::out.printf(...), netlog::out.println(...)

void begin();  // call after Serial.begin()
void loop();   // accepts a client once Wi-Fi is up; call every loop

}  // namespace netlog
