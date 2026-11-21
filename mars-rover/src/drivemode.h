// Drive modes selected over nc: stop (default), explore (autonomous, avoids obstacles), sleep (light sleep).
#pragma once
#include <stdint.h>

namespace drivemode {

enum class Mode { Stop, Explore, Sleep };

// Switches mode (stopping the motors first). Returns false and stays put if Explore is requested
// without a working distance sensor.
bool set(Mode m, uint32_t now);
Mode current();
const char* name();

void loop(uint32_t now);  // call every loop: runs the explore behavior

}  // namespace drivemode
