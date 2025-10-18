#pragma once
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
  
} // namespace utils

