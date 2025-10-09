#include "led_blinker.h"
#include <cstdint>
#include <functional>

void LedBlinker::apply(LedRGB rgb) {
  if (setColorFn) setColorFn(rgb);
}

void LedBlinker::init(std::function<void(LedRGB)> fn) {
  setColorFn = std::move(fn);
  mode = LED_OFF;
  colorSolid = LedRGB{0, 0, 0};
  apply(colorSolid);
}

void LedBlinker::off() {
  mode = LED_OFF;
  colorSolid = LedRGB{0, 0, 0};
  apply(colorSolid);
}

void LedBlinker::set_solid(LedRGB rgb) {
  mode = LED_SOLID;
  colorSolid = rgb;
  apply(rgb);
}

void LedBlinker::recalc() {
  if (periodMs < 10) periodMs = 10;
  if (duty < 0.01f) duty = 0.01f;
  if (duty > 0.99f)
    duty = 0.99f;

  onMs = (uint32_t)(periodMs * duty);
  offMs = periodMs - onMs;
}

void LedBlinker::set_blink(LedRGB on, LedRGB off,
                           uint32_t period, float d) {
  mode = LED_BLINK;
  colorOn = on;
  colorOff = off;
  periodMs = period;
  duty = d;
  recalc();
  phaseOn = true;
  lastTick = millis();
  apply(colorOn);
}

void LedBlinker::tick() {
  if (mode != LED_BLINK)
    return;

  const uint32_t now = millis();
  const uint32_t due = phaseOn ? onMs : offMs;

  if ((uint32_t)(now - lastTick) >= due) {
    phaseOn = !phaseOn;
    lastTick = now;
    apply(phaseOn ? colorOn : colorOff);
  }
  
}
