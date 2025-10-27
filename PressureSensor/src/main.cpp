#include <M5Atom.h>
#include <Preferences.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <ModbusTCP.h>
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

//#define ENABLE_MODBUS

#ifdef ENABLE_MODBUS
IPAddress SERVER_IP(192,168,10,21);
const uint16_t SERVER_PORT = 502;   // Modbus TCP default
const uint8_t  UNIT_ID     = 1;     // UR usually ignores, but 1 is fine
ModbusTCP mb;            // client
uint16_t hregs[4] = {0}; // example read buffer
#endif


AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

Preferences prefs;
LedBlinker led_blinker;
utils::Timer timer{1000, millis()};

utils::Timer pressureTimer{1000, millis()};

utils::StateMachine state{State::NOOP};
storage::WifiCredentials wifi_creds;

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
#ifdef ENABLE_MODBUS
      mb.server();
      mb.addHreg(0, 0, 1);
      mb.connect(SERVER_IP, SERVER_PORT);
      Serial.println("Modbus TCP client ready");
#endif
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
#ifdef ENABLE_MODBUS
    mb.task();
#endif
    if (timer.ready()) {
      Serial.println(WiFi.localIP());
#ifdef ENABLE_MODBUS
      if (!mb.isConnected(SERVER_IP)) {
        Serial.println("Reconnecting…");
        mb.connect(SERVER_IP, SERVER_PORT);
        
        return;
      }
      if (M5.Btn.wasPressed()) {
        float pressure_read = pressure::getPressure();
        uint16_t res = static_cast<uint16_t>(std::round(pressure_read));
        mb.Hreg(0, res);
      }
#endif
    }
    if (pressureTimer.ready()) {
      float pressure_read = pressure::getPressure();
      auto message = String(pressure_read);
      // here we send the data to the websocket. On the other hand, we need to connect to the same socket
      ws.textAll(message);
    }

    break;
  default:
    if (timer.ready()) {
      Serial.println("DEFAULT loop.");
    }
    break;
  }
  
}
