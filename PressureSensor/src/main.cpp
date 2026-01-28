#include <M5Atom.h>
#include <Arduino.h>
#include <Preferences.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <ModbusIP_ESP8266.h>
#include <sys/types.h>
#include "HardwareSerial.h"
#include "WiFiClient.h"
#include "crgb.h"
#include "esp32-hal.h"
#include "led_blinker.h"
#include "utils.h"
#include "config.h"
#include "wifi_manager.h"
#include "storage.h"
#include "pressure.h"
#include "http_server.h"
#include "mqtt_module.h"
#include "modbus_module.h"
#include "http_server_module.h"
#include "websocket_module.h"
#include "pressure_module.h"
#include "serial.h"
#include <MQTT.h>
#include <ArduinoJson.h>
#include <time.h>

//#define SERIAL_WIFI_CONNECT

const char *mqttHost = "mqtt.narbot.ee";
const uint16_t mqttPort = 1883;
const char *pressureTopic = "pressure/testing";

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
MQTTClient mqttClient;

WiFiClient wifiClient;

Preferences prefs;
LedBlinker led_blinker;
utils::Timer timer{1000, static_cast<uint32_t>(millis())};

utils::Timer pressureTimer {20, static_cast<uint32_t>(millis())};

utils::StateMachine state{State::NOOP};
storage::WifiCredentials wifi_creds;

IPAddress modbusServer(10, 8, 0, 1);
const uint16_t modbusPort = 502;
ModbusIP mb;
const uint16_t unitId = 1;

const char *ntpServer = "pool.ntp.org";

String client_id;

MqttModule mqtt_module{mqttClient, wifiClient, mqttHost, mqttPort, pressureTopic};
ModbusModule modbus_module{mb, modbusServer, modbusPort, unitId};
HttpServerModule http_server_module{server, ws, led_blinker};
WebsocketModule websocket_module{ws};
PressureModule pressure_module;

void setPixel(LedRGB rgb) {
  M5.dis.drawpix(0, CRGB(rgb.r, rgb.g, rgb.b));
}

void setup() {
  state.change_to(State::START_SETUP);
  M5.begin(true, false, true);
  M5.dis.clear();
  led_blinker.init(setPixel);
  mqtt_module.set_enabled(ENABLE_MQTT);
  modbus_module.set_enabled(ENABLE_MODBUS);
  http_server_module.set_enabled(ENABLE_HTTP_SERVER);
  websocket_module.set_enabled(ENABLE_WEBSOCKET);
  pressure_module.set_enabled(ENABLE_PRESSURE);
  configTime(0, 0, ntpServer);
  led_blinker.set_blink(COLOR_ORANGE,COLOR_BLACK);

  if (!LittleFS.begin(true)) {
    Serial.println("[ERROR] Failed to initialize LittleFS \n");
    return;
  }

  if (!storage::has_wifi_credentials()) {
    Serial.println("No credentials, entering provisioning mode.\n");
    state.change_to(State::PROVISIONING_MODE);
  } else {

    Serial.println("Found credentials, entering operations mode");
    storage::load_wifi_credentials(wifi_creds);

    Serial.printf("SSID: %s, Pass: %s", wifi_creds.ssid.c_str(),
                  wifi_creds.pass.c_str());
    state.change_to(State::OPERATION_MODE);
  }

  switch (state.state) {
  case PROVISIONING_MODE:
    wifi_manager::init_ap_wifi();

//#ifndef SERIAL_WIFI_CONNECT
    Serial.println("AP WiFi Connect");
    http_server_module.begin_ap();
    delay(100);
    state.change_to(State::PM_CONNECT_WAIT);
    led_blinker.set_blink(COLOR_ORANGE, COLOR_BLACK);
// #else
//     if (serial::init_ap(led_blinker)) {
//       state.change_to(State::PM_CONNECT_WAIT);
//       led_blinker.set_blink(COLOR_ORANGE, COLOR_BLACK);
//     }
// #endif
    break;
  case OPERATION_MODE: {
    if (!storage::has_wifi_credentials()) {
      Serial.println("Invalid credentials, entering provisioning mode");
      storage::clear_wifi_credentials();
      state.change_to(State::PROVISIONING_MODE);
      return;
    }
    storage::load_wifi_credentials(wifi_creds);
    auto connected =
        wifi_manager::init_sta_wifi(wifi_creds.ssid, wifi_creds.pass, led_blinker);    
    
    if (connected) {
      if (http_server_module.enabled()) {
        bool is_server_created =
            http_server_module.begin_client(pressure_module.enabled() ? &pressure::get_pressure_uint : nullptr);
        if (!is_server_created) {
          return;
        }
      }
      state.change_to(State::OP_CONNECT_WAIT);
      pressure_module.begin();
      if (mqtt_module.enabled()) {
        Serial.print("Connecting to the mqtt server");
      }
      auto device_id = utils::get_short_device_id();
      client_id = String("M5_Atom_") + device_id;
      mqtt_module.begin(client_id);
      String topic = "devices/" + client_id + "/init";
      JsonDocument doc;
      doc["id"] = client_id;
      doc["timestamp"] = utils::now_ms_utc();
      Serial.println("Timestamp " + utils::now_ms_utc());
      char buf[1028];
      size_t n = serializeJson(doc, buf);
      Serial.println("Sending to: device/" + client_id + "/init");
      Serial.println("Message:" + String(buf));
      mqtt_module.publish_raw(topic, buf, n);
      
      modbus_module.begin();
      if (modbus_module.enabled()) {
        Serial.println("Modbus TCP client ready");
      }
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
  case PM_CONNECT_WAIT: // Provision mode loop
    if (timer.ready()) {
      Serial.println("One tick every 1 second");
    }
    break;
  case OP_CONNECT_WAIT: // Operational mode loop
    if (!modbus_module.ensure_connected()) {
      Serial.println("Connecting to modbus");
      return;
    }

    if (M5.Btn.isPressed()) {
      Serial.println("PRESSED");
      utils::readMacAddress();
    }
    modbus_module.update_button(M5.Btn.isPressed(), M5.Btn.wasPressed());
    modbus_module.loop();
    
    if (timer.ready()) {
      Serial.println(WiFi.localIP());
    }
    mqtt_module.loop();
    
    if (pressureTimer.ready() && pressure_module.enabled()) {
      float pressure_read = pressure_module.read_float();
      Serial.printf("Sending data to mqtt");

      auto message = String(pressure_read, 2);
      // here we send the data to the websocket. On the other hand, we need to connect to the same socket
      mqtt_module.publish_pressure(message);
      websocket_module.send(message);
      modbus_module.write_pressure(static_cast<uint16_t>(pressure_read));
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
