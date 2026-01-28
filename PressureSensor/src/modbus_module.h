#pragma once
#include <Arduino.h>
#include <ModbusIP_ESP8266.h>

class ModbusModule {
 public:
  ModbusModule(ModbusIP& modbus, IPAddress server, uint16_t port, uint16_t unit_id)
      : modbus_(modbus), server_(server), port_(port), unit_id_(unit_id) {}

  void set_enabled(bool enabled) { enabled_ = enabled; }
  bool enabled() const { return enabled_; }

  void begin() {
    if (enabled_) {
      modbus_.client();
    }
  }

  bool ensure_connected() {
    if (!enabled_) {
      return true;
    }
    if (!modbus_.isConnected(server_)) {
      modbus_.connect(server_);
      return false;
    }
    return true;
  }

  void loop() {
    if (enabled_) {
      modbus_.task();
    }
  }

  void update_button(bool pressed, bool was_pressed) {
    if (!enabled_) {
      return;
    }
    modbus_.writeCoil(server_, 0, pressed, nullptr, unit_id_);
    modbus_.Coil(3, was_pressed);
  }

  void write_pressure(uint16_t value) {
    if (!enabled_) {
      return;
    }
    modbus_.writeHreg(server_, 0, value, nullptr, unit_id_);
  }

 private:
  ModbusIP& modbus_;
  IPAddress server_;
  uint16_t port_;
  uint16_t unit_id_;
  bool enabled_ = true;
};
