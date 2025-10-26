#pragma once
#include <M5Atom.h>
#include <Arduino.h>

namespace pressure {
  void initPressureReader();
  float getPressure();
}
