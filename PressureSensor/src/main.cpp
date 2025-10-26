#include <M5Atom.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <WebServer.h>
#include <LittleFS.h>
#include <ModbusTCP.h>
#include <cstdint>
#include <sys/types.h>
#include <vector>
#include "crgb.h"
#include "esp32-hal.h"
#include "led_blinker.h"
#include "utils.h"
#include "config.h"
#include "wifi_manager.h"
#include "storage.h"
#include "pressure.h"


// --- UR robot Modbus-TCP server ---
IPAddress SERVER_IP(192,168,10,21);
const uint16_t SERVER_PORT = 502;   // Modbus TCP default
const uint8_t  UNIT_ID     = 1;     // UR usually ignores, but 1 is fine

ModbusTCP mb;            // client
uint16_t hregs[4] = {0}; // example read buffer

WebServer server(80);
Preferences prefs;
LedBlinker led_blinker;
utils::Timer timer{1000, millis()};
utils::StateMachine state{State::NOOP};
storage::WifiCredentials wifi_creds;

namespace led {
  
  LedRGB hexToRgb(String hex) {
    if (hex.length() != 7 || hex.charAt(0) != '#') return LedRGB {0, 0, 0};

    uint8_t r = strtoul(hex.substring(1,3).c_str(), nullptr, 16);
    uint8_t g = strtoul(hex.substring(3,5).c_str(), nullptr, 16);
    uint8_t b = strtoul(hex.substring(5, 7).c_str(), nullptr, 16);
    
    return LedRGB {r, g, b};
  }

  String rgbToHex(LedRGB rgb) {
    char hexColor[8];
    snprintf(hexColor, sizeof(hexColor), "#%02X%02X%02X", rgb.r, rgb.g, rgb.b);
    return String(hexColor);
  }
}

namespace api {
  void sendJson(const JsonDocument &doc, int code = 200) {
    String out;
    serializeJson(doc, out);
  
    server.send(code, "application/json", out);
  }
  
  void handlePostLed() {
    if (server.method() != HTTP_POST) {
      server.send(405, "text/plain", "Method Not Allowed");
      return;
    }

    if (!server.hasArg("plain")) {
      server.send(400, "text/plain", "Missing body");
      return;
    }

    JsonDocument payload;

    auto err = deserializeJson(payload, server.arg("plain"));
    if (err) { server.send(400, "text/plain", "Invalid JSON"); return; }

    const char *hex = payload["color"];
    
    if (!hex) {
      server.send(400, "text/plain", "Missing 'color'");
      return;
    }
  
    String h = String(hex);
    if (h.length() != 7 || h[0] != '#') {
      server.send(400, "text/plain", "Expect color like \"#RRGGBB\"");
      return;
    }

    LedRGB rgb = led::hexToRgb(hex);
    led_blinker.set_solid(rgb);
    JsonDocument out;
    out["color"] = led::rgbToHex(led_blinker.colorSolid);
  
    String s;
    serializeJson(out, s);
    server.send(200, "application/json", s);
  }

  void handleGetLed() {
    JsonDocument doc;

    doc["color"] = led::rgbToHex(led_blinker.colorSolid);

    sendJson(doc);
  }

  void handleGetWifi() {
    std::vector<wifi_manager::NetworkData> networks;
    wifi_manager::scan_wifi_networks(networks);
    JsonDocument doc;
    JsonObject obj = doc.to<JsonObject>();
    JsonArray arr = obj["networks"].to<JsonArray>();
    
    Serial.printf("Found %d networks.\n", networks.size());
    for (auto n : networks) {
      JsonObject x = arr.add<JsonObject>();
      x["ssid"] = n.ssid;
      x["rssi"] = n.rssi;
    }
    
    storage::load_wifi_credentials(wifi_creds);
    doc["ssid"] = wifi_creds.ssid;
    doc["pass"] = wifi_creds.pass;
    doc["token"] = wifi_creds.token;
    
    sendJson(doc);
  }

  void handlePostWifi() {
    Serial.printf("Saving WIFI credentials \n");
    if (server.method() != HTTP_POST) {
      server.send(405, "text/plain", "Method Not Allowed");
      return;
    }

    if (!server.hasArg("plain")) {
      server.send(400, "text/plain", "Missing body");
      return;
    }

    JsonDocument payload;

    auto err = deserializeJson(payload, server.arg("plain"));
    if (err) { server.send(400, "text/plain", "Invalid JSON"); return; }

    const char *ssid = payload["ssid"];
    const char *pass = payload["pass"];

    Serial.printf("Saving WIFI credentials ssid: %s pass: %s\n", ssid, pass);
    
    if (!ssid) {
      server.send(400, "text/plain", "Missing 'SSID'");
      return;
    }
    if (!pass) {
      pass = "";
    }

    wifi_creds.ssid = ssid;
    wifi_creds.pass = pass;
    
    storage::clear_wifi_credentials();

    if (!storage::save_wifi_credentials(wifi_creds)) {
      server.send(400, "text/plain", "Failed to save preferences, try again.");
      return;
    }
    
    JsonDocument out;
    out["status"] = "success";
  
    String s;
    serializeJson(out, s);
    server.send(200, "application/json", s);
  }
}

namespace http_server {
  void initAPServer() {
    server.serveStatic("/", LittleFS, "/index.html");
    server.serveStatic("/app.js", LittleFS, "/app.js");
    server.serveStatic("/pico.lime.min.css", LittleFS, "/pico.lime.min.css");
    server.serveStatic("/alpine.min.js", LittleFS, "/alpine.min.js");

    server.on("/api/led", HTTP_GET, api::handleGetLed);
    server.on("/api/led", HTTP_POST, api::handlePostLed);
    server.on("/api/wifi", HTTP_GET, api::handleGetWifi);
    server.on("/api/wifi", HTTP_POST, api::handlePostWifi);

    server.onNotFound([]() {
      server.send(404, "text/plain", "Not Found");
    });
  
    server.begin();
  }

  void initClientServer() {
    server.serveStatic("/", LittleFS, "/main.html");
    server.serveStatic("/app.js", LittleFS, "/app.js");
    server.serveStatic("/pico.lime.min.css", LittleFS, "/pico.lime.min.css");
    server.serveStatic("/alpine.min.js", LittleFS, "/alpine.min.js");

    server.on("/api/wifi", HTTP_POST, api::handlePostWifi);

    server.onNotFound([]() {
      server.send(404, "text/plain", "Not Found");
    });
  
    server.begin();
  }
  
}

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
    
    http_server::initAPServer();
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
      http_server::initClientServer();
      pressure::initPressureReader();
      mb.server();
      mb.addHreg(0, 0, 1);
      mb.connect(SERVER_IP, SERVER_PORT);
      Serial.println("Modbus TCP client ready");
      // init new server
    } else {
      storage::clear_wifi_credentials();
      utils::system_restart();
    }

    break;
  }
  default: {
    // BUGFIX: Added braces for consistency with other cases
    Serial.println("Default state");
    break;
  }
  } // End of switch statement

  Serial.println("[INFO]: M5 App Setup Done");
}

static inline uint16_t scalePressureToWord(float pressure_bar) {
  int32_t scaled = lroundf(pressure_bar * 100.0f); // e.g., 1.23 bar -> 123
  if (scaled < -32768) scaled = -32768;
  if (scaled >  32767) scaled =  32767;
  return (uint16_t)(int16_t)scaled; // two's complement
}

void loop() {
  M5.update();
  server.handleClient();
  led_blinker.tick();
  
  switch (state.state) {
  case PM_CONNECT_WAIT:
    if (timer.ready()) {
      Serial.println("One tick every 1 second");
    }
    break;
  case OP_CONNECT_WAIT:
    mb.task();
    if (timer.ready()) {
      Serial.println(WiFi.localIP());
      // if (!mb.isConnected(SERVER_IP)) {
      //   Serial.println("Reconnecting…");
      //   mb.connect(SERVER_IP, SERVER_PORT);
        
      //   return;
      // }
      Serial.println("Wifi Connection was successful.");
    }

    if (M5.Btn.wasPressed()) {
      float pressure_bar = 1.23f;
      Serial.printf("Button pressed. Sending pressure: %.2f bar\n",
                    pressure_bar);
      float pressure_read = pressure::getPressure();
      uint16_t word = scalePressureToWord(pressure_read);
      mb.Hreg(0, word);
    }
    
    break;
  default:
    if (timer.ready()) {
      Serial.println("DEFAUL loop.");
    }
    break;
  }
  
}
