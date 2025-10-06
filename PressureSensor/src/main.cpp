#include <M5Atom.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <WebServer.h>
#include <LittleFS.h>

#define NET_CFG "netcfg"
#define DEFAULT_WIFI_SSID "M5Stack_Atom"
#define DEFAULT_WIFI_PASS "66666666"

WebServer server(80);
Preferences prefs;

typedef struct {
  String ssid;
  String pass;
  String token; // optional auth token for future use
} NetCfg;

typedef struct {
  uint8_t r, g, b;
} Rgb;

static Rgb currentColor{ 0x00, 0xFF, 0x00 };

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

  bool loadNetCfg(NetCfg& cfg) {
    Preferences prefs;

    if (!prefs.begin(NET_CFG, false)) {
      Serial.printf("ERROR: Couldn't load Preferences: " NET_CFG);
      return false;
    }

    cfg.ssid = prefs.getString("ssid", DEFAULT_WIFI_SSID);
    Serial.printf("prefs.getString ssid %s \n", cfg.ssid.c_str());
    
    cfg.pass  = prefs.getString("pass",  DEFAULT_WIFI_PASS);
    cfg.token = prefs.getString("token", "");
    prefs.end();
    return true;
  }
}

namespace led {
  
  Rgb hexToRgb(String hex) {
    if (hex.length() != 7 || hex.charAt(0) != '#') return Rgb {};

    uint8_t r = strtoul(hex.substring(1,3).c_str(), nullptr, 16);
    uint8_t g = strtoul(hex.substring(3,5).c_str(), nullptr, 16);
    uint8_t b = strtoul(hex.substring(5, 7).c_str(), nullptr, 16);
    
    return Rgb {r, g, b};
  }

  String rgbToHex(Rgb rgb) {
    char hexColor[8];
    snprintf(hexColor, sizeof(hexColor), "#%02X%02X%02X", rgb.r, rgb.g, rgb.b);
    return String(hexColor);
  }
  
  void setLedHex(const String& hex) {
    Rgb rgb = hexToRgb(hex);
    M5.dis.drawpix(0, CRGB(rgb.r, rgb.g, rgb.b));
    currentColor = rgb;
  }
  
}

namespace json {
  void sendJson(const JsonDocument &doc, int code = 200) {
    String out;
    serializeJson(doc, out);
  
    server.send(code, "application/json", out);
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

    led::setLedHex(h);

    JsonDocument out;

    out["color"] = led::rgbToHex(currentColor);
  
    String s;
    serializeJson(out, s);
    server.send(200, "application/json", s);
  }

  void handleGetLed() {
    JsonDocument doc;

    doc["color"] = led::rgbToHex(currentColor);

    json::sendJson(doc);
  }

  void handleGetWifi() {
    JsonDocument doc;
    NetCfg cfg {};

    storage::loadNetCfg(cfg);
    doc["ssid"] = cfg.ssid;
    doc["pass"] = cfg.pass;
    doc["token"] = cfg.token;
    
    json::sendJson(doc);
  }
  
}

void initServer() {
    server.serveStatic("/", LittleFS, "/index.html");
    server.serveStatic("/index.js", LittleFS, "/index.js");
    server.serveStatic("/pico.lime.min.css", LittleFS, "/pico.lime.min.css");
    server.serveStatic("/alpine.min.js", LittleFS, "/alpine.min.js");

    server.on("/api/led", HTTP_GET, api::handleGetLed);
    server.on("/api/led", HTTP_POST, api::handlePostLed);
    server.on("/api/wifi", HTTP_GET, api::handleGetWifi);

    // Fallback
    server.onNotFound([]() {
      server.send(404, "text/plain", "Not Found");
    });
  
    server.begin();
  }


void initWiFiSoftAP() {
  NetCfg cfg {};
  storage::loadNetCfg(cfg);
  Serial.println("\nWIFI ACCESS POINT (V1)");
  Serial.printf("Connect to: %s\nOpen: http://", cfg.ssid.c_str());
  Serial.printf("NETCFG: ssid: %s  pass: %s", cfg.ssid.c_str(), cfg.pass.c_str());
  WiFi.softAP(cfg.ssid, cfg.pass);
  IPAddress myIP = WiFi.softAPIP();
}

void setup() {
  M5.begin(true, false, true);
  M5.dis.clear();
  delay(50);
  led::setLedHex("#00FF00");

  if (!LittleFS.begin(true)) {
    Serial.println("[ERROR]: Error has occurred with serial filesystem");
    return;
  }
  
  initWiFiSoftAP();
  initServer();

  Serial.println("[INFO]: M5 App Setup Done");
}

void loop() {
  server.handleClient();
  delay(2);
}
