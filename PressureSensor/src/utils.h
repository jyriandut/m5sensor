#pragma once
#include "esp_system.h"
#include <Arduino.h>
#include <cstdint>

namespace utils {
  // usage Timer timer{1000, 0}
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


  inline void system_restart() {
    Serial.println("Rebooting in 2 seconds ...");
    delay(2000);
    esp_restart();
  }
  
} // namespace utils

