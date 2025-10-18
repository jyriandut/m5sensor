#pragma once
#include "esp_system.h"
#include <Arduino.h>
#include <cstdint>

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
  
} // namespace utils

