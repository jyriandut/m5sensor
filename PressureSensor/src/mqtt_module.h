#pragma once
#include <Arduino.h>
#include <MQTT.h>
#include "WiFiClient.h"

class MqttModule {
 public:
  MqttModule(MQTTClient& client, WiFiClient& wifi, const char* host, uint16_t port, const char* topic)
      : client_(client), wifi_(wifi), host_(host), port_(port), topic_(topic) {}

  void set_enabled(bool enabled) { enabled_ = enabled; }
  bool enabled() const { return enabled_; }

  bool begin(const String& client_id) {
    if (!enabled_) {
      return false;
    }
    client_.begin(host_, port_, wifi_);
    while (!client_.connect(client_id.c_str())) {
      delay(1000);
    }
    return true;
  }

  void loop() {
    if (enabled_) {
      client_.loop();
    }
  }

  void publish_pressure(const String& payload) {
    if (enabled_) {
      client_.publish(topic_, payload.c_str());
    }
  }

  bool publish_raw(const String& topic, const char* payload, size_t length) {
    if (!enabled_) {
      return false;
    }
    return client_.publish(topic.c_str(), payload, length);
  }

 private:
  MQTTClient& client_;
  WiFiClient& wifi_;
  const char* host_;
  uint16_t port_;
  const char* topic_;
  bool enabled_ = true;
};
