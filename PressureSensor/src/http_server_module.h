#pragma once
#include <ESPAsyncWebServer.h>
#include "led_blinker.h"
#include "http_server.h"

class HttpServerModule {
 public:
  HttpServerModule(AsyncWebServer& server, AsyncWebSocket& ws, LedBlinker& blinker)
      : server_(server), ws_(ws), blinker_(blinker) {}

  void set_enabled(bool enabled) { enabled_ = enabled; }
  bool enabled() const { return enabled_; }

  void begin_ap() {
    if (enabled_) {
      http_server::init_ap_http_server(server_, blinker_);
    }
  }

  bool begin_client(http_server::PressureReader reader) {
    if (!enabled_) {
      return true;
    }
    return http_server::init_client_http_server(server_, ws_, reader);
  }

 private:
  AsyncWebServer& server_;
  AsyncWebSocket& ws_;
  LedBlinker& blinker_;
  bool enabled_ = true;
};
