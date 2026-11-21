// Text drive commands read from the `nc mars-rover.local 23` connection. Type `?` for the list.
#pragma once
#include <stdint.h>

namespace commands {

void begin();             // loads the saved calibration
void loop(uint32_t now);  // call every loop: parses input, ends timed moves, failsafe on disconnect
bool isMoving();           // a timed move is in progress

}  // namespace commands
