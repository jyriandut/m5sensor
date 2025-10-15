#include <M5Atom.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <WebServer.h>
#include <LittleFS.h>
#include <cstdint>
#include <sys/types.h>
#include <vector>
#include "WiFiType.h"
#include "crgb.h"
#include "esp32-hal.h"
#include "esp_wifi_types.h"
#include "led_blinker.h"

#define NET_CFG "netcfg"
#define AP_WIFI_SSID "M5Stack_Atom"
#define AP_WIFI_PASS "66666666"
#define DEFAULT_LED_COLOR "#00FF00"

#define COLOR_GREEN {0, 200, 0}
#define COLOR_BLUE {0, 0, 200}
#define COLOR_ORANGE {120, 255, 0}
#define COLOR_BLACK {0,0,0}

WebServer server(80);
Preferences prefs;

typedef struct {
  String ssid;
  String pass;
  String token; // optional auth token for future use
} NetCfg;

// first startup:
// enable AP_STA mode for wifi
// wait for connection
// connection received
// scanning networks

enum State {
  NOOP,
  START_SETUP,
  PROVISIONING_MODE,
  OPERATION_MODE,
  PM_CONNECT_WAIT,
  OP_CONNECT_WAIT
};

static State state = NOOP;

static void change_state(State next) {
  if (next == state) return;
  state = next;
  Serial.printf("→ State changed to %d\n", state);
}

LedBlinker ledBlinker;

NetCfg netCfg {}; 

namespace storage {
  bool saveNetCfg(const NetCfg& cfg) {
    Preferences prefs;
    if (!prefs.begin(NET_CFG, false)) {
      Serial.printf("ERROR: Failed to open Preferences " NET_CFG);
      return false;
    }

    prefs.putString("ssid",  cfg.ssid);
    prefs.putString("pass",  cfg.pass);
    prefs.putString("token", cfg.token);
    prefs.end();
    return true;
  }

  bool clearCredentials() {
    Serial.printf("Deleting preferences: " NET_CFG);
    Preferences prefs;
    if (!prefs.begin(NET_CFG, true)) {
      Serial.printf("ERROR: Couldn't load Preferences: " NET_CFG);
      return false;
    }
    return prefs.clear();
  }
  
  bool hasCredentials() {
    Preferences prefs;
    bool result = true;
    if (!prefs.begin(NET_CFG, true)) {
      Serial.printf("ERROR: Couldn't load Preferences: " NET_CFG);
      result = false;
    }
    if (!prefs.isKey("ssid")) {
      Serial.println("ssid key not found in preferences");
      result = false;
    }
    else if (prefs.getString("ssid").equals(String())) {
      Serial.println("ssid key value empty");
      result = false;
    }
    prefs.end();
    return result;
  }
  
  bool loadNetCfg(NetCfg& cfg) {
    Preferences prefs;

    if (!prefs.begin(NET_CFG, false)) {
      Serial.printf("ERROR: Couldn't load Preferences: " NET_CFG);
      return false;
    }

    cfg.ssid = prefs.getString("ssid", "");
    Serial.printf("prefs.getString ssid %s \n", cfg.ssid.c_str());
    
    cfg.pass  = prefs.getString("pass", "");
    cfg.token = prefs.getString("token", "");
    prefs.end();
    return true;
  }
}

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

namespace json {
  void sendJson(const JsonDocument &doc, int code = 200) {
    String out;
    serializeJson(doc, out);
  
    server.send(code, "application/json", out);
  }
}

namespace network {
  typedef struct {
    String ssid;
    int32_t rssi;
    wifi_auth_mode_t auth_mode;
  } NetworkData;

  void scanWiFiNetworks(std::vector<network::NetworkData> &networks) {
    Serial.println("Scanning WiFi networks");
    int n = WiFi.scanNetworks();
    Serial.println("Scan done");

    if (n == 0) {
      Serial.println("No networks found");
    } else {
      Serial.print(n);
      Serial.println(" networks found");
      for (int i = 0; i < n; ++i) {

        const NetworkData n = {
          WiFi.SSID(i),
          WiFi.RSSI(i),
          WiFi.encryptionType(i)
        };
        
        networks.push_back(n);
      }
    }
  }

  void initAPWiFi() {
    WiFi.mode(WIFI_MODE_APSTA);
    bool ok = WiFi.softAP(AP_WIFI_SSID, AP_WIFI_PASS);
    Serial.println("\nWIFI ACCESS POINT (fallback)");
    // BUGFIX: Changed second parameter from AP_WIFI_SSID to AP_WIFI_PASS (was printing SSID twice)
    Serial.printf("SSID: %s  PASS: %s\n", AP_WIFI_SSID, AP_WIFI_PASS);
    Serial.printf("AP IP: %s\n", WiFi.softAPIP().toString().c_str());
  }
}

namespace api {
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
    ledBlinker.set_solid(rgb);
    JsonDocument out;
    out["color"] = led::rgbToHex(ledBlinker.colorSolid);
  
    String s;
    serializeJson(out, s);
    server.send(200, "application/json", s);
  }

  void handleGetLed() {
    JsonDocument doc;

    doc["color"] = led::rgbToHex(ledBlinker.colorSolid);

    json::sendJson(doc);
  }

  void handleGetWifi() {
    std::vector<network::NetworkData> networks;
    network::scanWiFiNetworks(networks);
    JsonDocument doc;
    JsonObject obj = doc.to<JsonObject>();
    JsonArray arr = obj["networks"].to<JsonArray>();
    
    Serial.printf("Found %d networks.\n", networks.size());
    for (auto n : networks) {
      JsonObject x = arr.add<JsonObject>();
      x["ssid"] = n.ssid;
      x["rssi"] = n.rssi;
    }
    
    storage::loadNetCfg(netCfg);
    doc["ssid"] = netCfg.ssid;
    doc["pass"] = netCfg.pass;
    doc["token"] = netCfg.token;
    
    json::sendJson(doc);
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

    netCfg.ssid = ssid;
    netCfg.pass = pass;
    
    storage::clearCredentials();

    if (!storage::saveNetCfg(netCfg)) {
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
  
}

void setPixel(LedRGB rgb) {
  M5.dis.drawpix(0, CRGB(rgb.r, rgb.g, rgb.b));
}


void setup() {
  change_state(START_SETUP);
  M5.begin(true, false, true);
  M5.dis.clear();
  ledBlinker.init(setPixel);

  ledBlinker.set_blink(COLOR_ORANGE,COLOR_BLACK);
  
  if (!LittleFS.begin(true)) {
    Serial.println("[ERROR]: Error has occurred with serial filesystem");
    return;
  }

  if (!storage::hasCredentials()) {
    Serial.println("No credentials, entering provisioning mode");
    change_state(State::PROVISIONING_MODE);
  } else {
    Serial.println("Found credentials, entering operations mode");
    storage::loadNetCfg(netCfg);
    
    Serial.printf("SSID: %s, Pass: %s", netCfg.ssid.c_str(), netCfg.pass.c_str());
    change_state(State::OPERATION_MODE);
  }

  switch (state) {
  case State::PROVISIONING_MODE:
    network::initAPWiFi();
    http_server::initAPServer();
    delay(100);
    change_state(State::PM_CONNECT_WAIT);
    ledBlinker.set_blink(COLOR_ORANGE, COLOR_BLACK);
    break;
  case State::OPERATION_MODE: {
    // BUGFIX: Added braces to allow variable declarations in switch case
    storage::loadNetCfg(netCfg);
    
    // BUGFIX: Set WiFi mode to STA (Station) before connecting
    // Previously this was missing, which could cause connection issues
    WiFi.mode(WIFI_MODE_STA);
    
    // BUGFIX: Disable auto-reconnect to allow our timeout logic to work
    // Without this, ESP32 keeps trying to reconnect indefinitely
    WiFi.setAutoReconnect(false);
    
    change_state(State::PM_CONNECT_WAIT);
    ledBlinker.set_blink(COLOR_BLUE, COLOR_BLACK, 1000);
    ledBlinker.tick();
    
    WiFi.begin(netCfg.ssid, netCfg.pass);
    Serial.printf("Connecting to WiFi network %s\n", netCfg.ssid.c_str());

    // BUGFIX: Added timeout to prevent infinite loop if WiFi fails
    // Waits max 20 seconds (40 attempts × 500ms)
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 40) {
      Serial.printf("Connecting... %u\n", WiFi.status());
      delay(500);
      ledBlinker.tick();
      attempts++;
    }
    
    // BUGFIX: Check if connection succeeded or timed out
    if (WiFi.status() == WL_CONNECTED) {
      // Successfully connected to WiFi
      Serial.println("Connected to WiFi network");
      change_state(State::OP_CONNECT_WAIT);
      ledBlinker.set_solid(COLOR_GREEN);
    } else {
      // BUGFIX: Connection failed - clear bad credentials and fall back to AP mode
      // This allows user to reconfigure WiFi instead of being stuck
      Serial.println("Failed to connect, falling back to AP mode");
      storage::clearCredentials();
      network::initAPWiFi();
      http_server::initAPServer();
      change_state(State::PM_CONNECT_WAIT);
      ledBlinker.set_blink(COLOR_ORANGE, COLOR_BLACK);
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

u_long lastTime = millis();


void loop() {
  u_long now = millis();
  u_long dt = now - lastTime;
  if (dt > 1000) {
      lastTime = now;
  }

  M5.update();
  server.handleClient();
  ledBlinker.tick();
  
  switch (state) {
  case PM_CONNECT_WAIT:
    if (dt > 1000) {
      Serial.println("One tick every 1 second");
    }
    break;
  case OP_CONNECT_WAIT:
    if (dt > 1000) {
      Serial.println("Wifi Connection was successful.");
    }
    break;
  default:
    if (dt > 1000) {
      Serial.println("DEFAUL loop.");
    }
    
    break;
  }
  
}
