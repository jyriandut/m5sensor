#include <M5Atom.h>
#include <Preferences.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <ModbusIP_ESP8266.h>
#include <sys/types.h>
#include <cmath>
#include "crgb.h"
#include "esp32-hal.h"
#include "led_blinker.h"
#include "utils.h"
#include "config.h"
#include "wifi_manager.h"
#include "storage.h"
#include "pressure.h"
#include "http_server.h"


IPAddress SERVER_IP(192,168,10,21);
const uint16_t SERVER_PORT = 502;   // Modbus TCP default
const uint8_t  UNIT_ID     = 1;     // UR usually ignores, but 1 is fine

uint16_t hregs[4] = {0}; // example read buffer

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

Preferences prefs;
LedBlinker led_blinker;
utils::Timer timer{1000, millis()};

utils::Timer pressureTimer{100, millis()};

utils::StateMachine state{State::NOOP};
storage::WifiCredentials wifi_creds;

IPAddress modbusServer(10, 8, 0, 1);
const uint16_t modbusPort = 502;
ModbusIP mb;
const uint16_t unitId = 1;

void setPixel(LedRGB rgb) {
  M5.dis.drawpix(0, CRGB(rgb.r, rgb.g, rgb.b));
}

void setup() {
  state.change_to(State::START_SETUP);
  M5.begin(true, false, true);
  M5.dis.clear();
  led_blinker.init(setPixel);

  led_blinker.set_blink(COLOR_ORANGE,COLOR_BLACK);
  
  if (!LittleFS.begin(true)) {
    Serial.println("[ERROR]: Error has occurred with serial filesystem");
    return;
  }

  if (!storage::has_wifi_credentials()) {
    Serial.println("No credentials, entering provisioning mode");
    state.change_to(State::PROVISIONING_MODE);
  } else {
    Serial.println("Found credentials, entering operations mode");
    storage::load_wifi_credentials(wifi_creds);
    
    Serial.printf("SSID: %s, Pass: %s", wifi_creds.ssid.c_str(), wifi_creds.pass.c_str());
    state.change_to(State::OPERATION_MODE);
  }

  switch (state.state) {
  case PROVISIONING_MODE:
    wifi_manager::init_ap_wifi();
    
    http_server::init_ap_http_server(server, led_blinker);
    delay(100);
    state.change_to(State::PM_CONNECT_WAIT);
    led_blinker.set_blink(COLOR_ORANGE, COLOR_BLACK);
    break;
  case OPERATION_MODE: {
    storage::load_wifi_credentials(wifi_creds);
    auto connected =
        wifi_manager::init_sta_wifi(wifi_creds.ssid, wifi_creds.pass, led_blinker);    
    
    if (connected) {
      state.change_to(State::OP_CONNECT_WAIT);
      http_server::init_client_http_server(server, ws);
      pressure::initPressureReader();
      mb.client();
      Serial.println("Modbus TCP client ready");
    } else {
      Serial.println("Unable to connect to wifi networkd. Deleting preferences and restarting");
      storage::clear_wifi_credentials();
      delay(1000);
      state.change_to(State::PM_CONNECT_WAIT);
      utils::system_restart();
    }
    break;
  }
  default: {
    Serial.println("Default state");
    break;
  }
  }

  Serial.println("[INFO]: M5 App Setup Done");
}

void loop() {
  M5.update();
  led_blinker.tick();
  
  switch (state.state) {
  case PM_CONNECT_WAIT:
    if (timer.ready()) {
      Serial.println("One tick every 1 second");
    }
    break;
  case OP_CONNECT_WAIT:

    if (!mb.isConnected(modbusServer)) {
      Serial.println("Connecting to modbus");
      mb.connect(modbusServer);
      return;
    }

    if (M5.Btn.isPressed()) {
      Serial.println("PRESSED");
      mb.writeCoil(modbusServer, 0, true, nullptr, unitId);
    } else {
      mb.writeCoil(modbusServer, 0, false, nullptr, unitId);
    }
    
    mb.task();
    
    if (timer.ready()) {
      Serial.println(WiFi.localIP());
    }
    mb.Coil(3, M5.Btn.wasPressed());
    if (timer.ready()) {
      
    }
    if (pressureTimer.ready()) {
      float pressure_read = pressure::getPressure();
      auto message = String(pressure_read);
      // here we send the data to the websocket. On the other hand, we need to connect to the same socket
      ws.textAll(message);
      uint16_t pressureModbus = static_cast<uint16_t>(pressure_read);
      Serial.printf("Scaled pressure reads: %d", pressureModbus);
      mb.writeHreg(modbusServer, 0, pressureModbus, nullptr, unitId);
    }
    delay(10);
    break;
  default:
    if (timer.ready()) {
      Serial.println("DEFAULT loop.");
    }
    break;
  }
  
}
