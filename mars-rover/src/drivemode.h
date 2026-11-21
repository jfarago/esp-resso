// Drive modes selected over nc: stop (default), explore (autonomous, avoids obstacles), seek (find the
// brightest spot to recharge), sleep (light sleep).
#pragma once
#include <stdint.h>

namespace drivemode {

enum class Mode { Stop, Explore, Seek, Sleep };

// Switches mode (stopping the motors first). Returns false and stays put if Explore/Seek is requested
// without the sensors it needs (distance, plus both light sensors for Seek).
bool set(Mode m, uint32_t now);
Mode current();
const char* name();
float seekArc();             // steering strength, see SEEK_ARC
void setSeekArc(float arc);
bool active();  // Explore or Seek: driving on its own

void loop(uint32_t now);  // call every loop: runs the explore behavior

}  // namespace drivemode
