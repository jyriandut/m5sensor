#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <vector>
#include "led_blinker.h"
#include "config.h"

namespace wifi_manager {
  struct NetworkData {
    String ssid;
    int32_t rssi;
    wifi_auth_mode_t auth_mode;
  };

  void init_ap_wifi();

  bool init_sta_wifi(const String ssid, const String password, LedBlinker& ledBlinker);
  
  void scan_wifi_networks(std::vector<NetworkData> &networks);
  
}
