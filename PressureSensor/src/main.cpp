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
#include "serial.h"
#include <MQTT.h>
#include <ArduinoJson.h>
#include <time.h>

//#define SERIAL_WIFI_CONNECT

#define TOPIC_NAME "pressure/data"

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
MQTTClient mqttClient;

WiFiClient wifiClient;

Preferences prefs;
LedBlinker led_blinker;
utils::Timer timer{1000, static_cast<uint32_t>(millis())};

utils::Timer pressureTimer {1000, static_cast<uint32_t>(millis())};

utils::StateMachine state{State::NOOP};
storage::WifiCredentials wifi_creds;

IPAddress modbusServer(10, 8, 0, 1);
const uint16_t modbusPort = 502;
ModbusIP mb;
const uint16_t unitId = 1;

const char *ntpServer = "pool.ntp.org";

String client_id;

void setPixel(LedRGB rgb) {
  M5.dis.drawpix(0, CRGB(rgb.r, rgb.g, rgb.b));
}

void setup() {
  state.change_to(State::START_SETUP);
  M5.begin(true, false, true);
  M5.dis.clear();
  led_blinker.init(setPixel);
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
    http_server::init_ap_http_server(server, led_blinker);
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
      bool is_server_created = http_server::init_client_http_server(server, ws);
      if (!is_server_created) {
        return;
      }
      state.change_to(State::OP_CONNECT_WAIT);
      pressure::initPressureReader();
      Serial.print("Connecting to the mqtt server");
      mqttClient.begin("mqtt.narbot.ee", wifiClient);
      auto device_id = utils::get_short_device_id();
      client_id = String("M5_Atom_") + device_id;
      while (!mqttClient.connect(client_id.c_str())) {
        Serial.print(".");
        delay(1000);
      }
      String topic = "devices/" + client_id + "/init";
      JsonDocument doc;
      doc["id"] = client_id;
      doc["timestamp"] = utils::now_ms_utc();
      Serial.println("Timestamp " + utils::now_ms_utc());
      char buf[1028];
      size_t n = serializeJson(doc, buf);
      Serial.println("Sending to: device/" + client_id + "/init");
      Serial.println("Message:" + String(buf));
      mqttClient.publish(topic.c_str(), buf, n);
          
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
  case PM_CONNECT_WAIT: // Provision mode loop
    if (timer.ready()) {
      Serial.println("One tick every 1 second");
    }
    break;
  case OP_CONNECT_WAIT: // Operational mode loop
    if (!mb.isConnected(modbusServer)) {
      Serial.println("Connecting to modbus");
      mb.connect(modbusServer);
      return;
    }

    if (M5.Btn.isPressed()) {
      Serial.println("PRESSED");
      utils::readMacAddress();
      mb.writeCoil(modbusServer, 0, true, nullptr, unitId);
    } else {
      mb.writeCoil(modbusServer, 0, false, nullptr, unitId);
    }
    
    mb.task();
    
    if (timer.ready()) {
      Serial.println(WiFi.localIP());
    }
    mb.Coil(3, M5.Btn.wasPressed());
    
    if (pressureTimer.ready()) {
      auto pressure_read = pressure::get_pressure_uint();
      Serial.printf("Sending data to mqtt");

      auto message = String(pressure_read);
      mqttClient.publish("pressure/testing", pressure_read);
      // here we send the data to the websocket. On the other hand, we need to connect to the same socket
      ws.textAll(message);      
      mb.writeHreg(modbusServer, 0, pressure_read, nullptr, unitId);
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
