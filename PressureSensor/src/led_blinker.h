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

namespace led {
  inline LedRGB hexToRgb(String hex) {
    if (hex.length() != 7 || hex.charAt(0) != '#') return LedRGB {0, 0, 0};

    uint8_t r = strtoul(hex.substring(1,3).c_str(), nullptr, 16);
    uint8_t g = strtoul(hex.substring(3,5).c_str(), nullptr, 16);
    uint8_t b = strtoul(hex.substring(5, 7).c_str(), nullptr, 16);
    
    return LedRGB {r, g, b};
  }

  inline String rgbToHex(LedRGB rgb) {
    char hexColor[8];
    snprintf(hexColor, sizeof(hexColor), "#%02X%02X%02X", rgb.r, rgb.g, rgb.b);
    return String(hexColor);
  }
  
}
