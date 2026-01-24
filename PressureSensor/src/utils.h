#pragma once
#include "esp_system.h"
#include "esp_wifi.h"
#include <Arduino.h>
#include <cstdint>
#include <time.h>

enum State {
  NOOP,
  START_SETUP,
  PROVISIONING_MODE,
  OPERATION_MODE,
  PM_CONNECT_WAIT,
  OP_CONNECT_WAIT
};

namespace utils {
  struct Timer {
    uint32_t interval;
    uint32_t last_trigger;

    bool ready() {
      auto now = millis();
      if (now - last_trigger >= interval) {
        last_trigger = now;
        return true;
      }
      return false;
    }
  };
 
  struct StateMachine {
    State state {NOOP};

    void change_to(State next) {
      if (next == state) return;
      state = next;
      Serial.printf("→ State changed to %d\n", state);
    }
  };

  inline void system_restart() {
    Serial.println("Rebooting in 2 seconds ...");
    delay(2000);
    esp_restart();
  }

  inline void readMacAddress(){
    uint8_t baseMac[6];
    esp_err_t ret = esp_wifi_get_mac(WIFI_IF_STA, baseMac);
    if (ret == ESP_OK) {
      Serial.printf("%02x:%02x:%02x:%02x:%02x:%02x\n", baseMac[0], baseMac[1],
                    baseMac[2], baseMac[3], baseMac[4], baseMac[5]);
    } else {
      Serial.println("Failed to read MAC address");
    }
  }

  inline String get_short_device_id() {
    uint8_t mac[6];    
    esp_err_t ret = esp_wifi_get_mac(WIFI_IF_STA, mac);
    if (ret == ESP_OK) {
      char buf[7]; // 6 hex + null
      sprintf(buf, "%02X%02X%02X", mac[3], mac[4], mac[5]);
      return String(buf); // e.g. "FF1234"
    }
    return String();
  }

  inline uint64_t now_ms_utc() {
    struct timeval tv;
    gettimeofday(&tv,
                 nullptr); // tv_sec = seconds, tv_usec = microseconds (UTC)
    uint64_t ms =
        (uint64_t)tv.tv_sec * 1000ULL + (uint64_t)tv.tv_usec / 1000ULL;
    return ms;
}
  
} // namespace utils

