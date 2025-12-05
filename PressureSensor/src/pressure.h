#pragma once
#include <M5Atom.h>
#include <Arduino.h>
#include <cstdint>

namespace pressure {
  void initPressureReader();
  float getPressure();
  uint16_t get_pressure_uint();
}
