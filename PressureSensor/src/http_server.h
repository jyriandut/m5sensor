#pragma once
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "ESPAsyncWebServer.h"
#include "led_blinker.h"


namespace http_server {
  void init_ap_http_server(AsyncWebServer& server, LedBlinker& led_blinker);
  void init_client_http_server(AsyncWebServer& server, AsyncWebSocket& ws);
}
