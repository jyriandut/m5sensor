#pragma once
#include <cstdint>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "ESPAsyncWebServer.h"
#include "led_blinker.h"


namespace http_server {
  using PressureReader = uint16_t (*)();
  void init_ap_http_server(AsyncWebServer& server, LedBlinker& led_blinker);
  bool init_client_http_server(AsyncWebServer& server, AsyncWebSocket& ws, PressureReader pressure_reader);
}
