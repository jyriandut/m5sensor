#pragma once
#include <Arduino.h>
#include <ESPAsyncWebServer.h>

class WebsocketModule {
 public:
  explicit WebsocketModule(AsyncWebSocket& ws) : ws_(ws) {}

  void set_enabled(bool enabled) { enabled_ = enabled; }
  bool enabled() const { return enabled_; }

  void send(const String& message) {
    if (enabled_) {
      ws_.textAll(message);
    }
  }

 private:
  AsyncWebSocket& ws_;
  bool enabled_ = true;
};
