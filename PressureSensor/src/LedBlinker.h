// LedBlinker.h
#pragma once
#include <Arduino.h>
#include <functional>

#ifndef RGB24
// Helper: make 0xRRGGBB from r,g,b
#define RGB24(r,g,b) ( ((uint32_t)(r) << 16) | ((uint32_t)(g) << 8) | ((uint32_t)(b)) )
#endif

class LedBlinker {
public:
  enum Mode { OFF, SOLID, BLINK };

  explicit LedBlinker(std::function<void(uint32_t)> setColorFn)
  : _setColor(std::move(setColorFn)) {}

  // ---- High-level APIs ----
  void off() {
    _mode = OFF;
    _colorSolid = 0x000000;
    _apply(0x000000);
  }

  void setSolid(uint32_t rgb) {
    _mode = SOLID;
    _colorSolid = rgb;
    _apply(_colorSolid);
  }

  // Blink between colorOn and colorOff (default off) with periodMs and duty [0..1]
  void setBlink(uint32_t colorOn,
                uint32_t colorOff = 0x000000,
                uint32_t periodMs = 1000,
                float duty = 0.5f) {
    _mode = BLINK;
    _colorOn  = colorOn;
    _colorOff = colorOff;
    setPeriod(periodMs);
    setDuty(duty);
    _phaseOn = true;
    _last   = millis();
    _apply(_colorOn);
  }

  void setPeriod(uint32_t periodMs) {
    if (periodMs < 10) periodMs = 10;
    _period = periodMs;
    _recalcPhaseDurations();
  }

  void setDuty(float duty) { // 0..1
    if (duty < 0.01f) duty = 0.01f;
    if (duty > 0.99f) duty = 0.99f;
    _duty = duty;
    _recalcPhaseDurations();
  }

  void start() {
    if (_mode == BLINK) {
      _phaseOn = true;
      _last = millis();
      _apply(_colorOn);
    } else if (_mode == SOLID) {
      _apply(_colorSolid);
    } else {
      _apply(0x000000);
    }
  }

  void stop() { off(); }

  // Call often (e.g., each loop()). Non-blocking.
  void tick() {
    if (_mode != BLINK) return;

    const uint32_t now = millis();
    const uint32_t due = _phaseOn ? _onMs : _offMs;
    if ((uint32_t)(now - _last) >= due) {
      _phaseOn = !_phaseOn;
      _last = now;
      _apply(_phaseOn ? _colorOn : _colorOff);
    }
  }

  Mode mode() const { return _mode; }

private:
  void _recalcPhaseDurations() {
    _onMs  = (uint32_t)(_period * _duty);
    _offMs = _period - _onMs;
    if (_onMs == 0)  _onMs  = 1;
    if (_offMs == 0) _offMs = 1;
  }

  inline void _apply(uint32_t rgb) {
    if (_setColor) _setColor(rgb);
  }

  std::function<void(uint32_t)> _setColor;

  Mode _mode = OFF;

  // SOLID
  uint32_t _colorSolid = 0x000000;

  // BLINK
  uint32_t _colorOn  = 0x000000;
  uint32_t _colorOff = 0x000000;
  uint32_t _period   = 1000; // ms
  float    _duty     = 0.5f;
  uint32_t _onMs     = 500;
  uint32_t _offMs    = 500;
  bool     _phaseOn  = true;
  uint32_t _last     = 0;
};
