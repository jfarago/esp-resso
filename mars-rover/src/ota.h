// Over-the-air firmware updates via ArduinoOTA (`pio run -e ota -t upload`).
#pragma once

namespace ota {

void loop();  // starts ArduinoOTA once Wi-Fi is up, then services it; call every loop

}  // namespace ota
