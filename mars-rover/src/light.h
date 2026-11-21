// Left/right BH1750 ambient light sensors (continuous high-resolution mode), lux.
#pragma once

namespace light {

void begin();                       // Wire must already be started; logs if a sensor is missing
bool ready();                       // both sensors responded at begin()
bool read(float& left, float& right);  // latest lux from each; false if either read fails

}  // namespace light
