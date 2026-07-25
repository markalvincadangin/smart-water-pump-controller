#pragma once

// Called from main setup/loop when SENSOR_NODE_OTA is enabled (see platformio.ini).
void ota_wifi_setup();
void ota_wifi_loop();
