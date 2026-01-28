#pragma once
#include "pressure.h"

class PressureModule {
 public:
  void set_enabled(bool enabled) { enabled_ = enabled; }
  bool enabled() const { return enabled_; }

  void begin() {
    if (enabled_) {
      pressure::initPressureReader();
    }
  }

  uint16_t read_uint() {
    if (!enabled_) {
      return 0;
    }
    return pressure::get_pressure_uint();
  }

  float read_float() {
    if (!enabled_) {
      return 0.0f;
    }
    return pressure::getPressure();
  }

 private:
  bool enabled_ = true;
};
