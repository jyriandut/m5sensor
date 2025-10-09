#pragma once
#include <Arduino.h>
#include <cstdint>
#include <functional>
#include <stdint.h>

typedef struct {
  uint8_t r, g, b;
} LedRGB;

enum BlinkMode { LED_OFF, LED_SOLID, LED_BLINK };

struct LedBlinker {
  BlinkMode mode = LED_OFF;
  LedRGB colorSolid = { 0,0,0 };
  std::function<void(LedRGB)> setColorFn = nullptr;

  // Blink fields
  LedRGB colorOn = {0, 0, 0};
  LedRGB colorOff = {0, 0, 0};

  uint32_t periodMs = 1000;
  float duty = 0.5f;
  uint32_t onMs = 500;
  uint32_t offMs = 500;
  bool phaseOn = true;
  uint32_t lastTick = 0;
  
  void init(std::function<void(LedRGB)> fn);
  void off();
  void set_solid(LedRGB rgb);

  void recalc();
  void set_blink(LedRGB colorOn, LedRGB colorOff = {0, 0, 0},
                 uint32_t periodMs = 1000, float duty = 0.5f);
  void tick();
  
private:
  void apply(LedRGB rgb);
};
