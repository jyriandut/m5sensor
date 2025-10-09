#include <M5Atom.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <WebServer.h>
#include <LittleFS.h>
#include <cstddef>
#include <cstdint>
#include <vector>
//#include "LedBlinker.h"
#include "crgb.h"
#include "esp_wifi_types.h"
#include "led_blinker.h"

#define NET_CFG "netcfg"
#define DEFAULT_WIFI_SSID "M5Stack_Atom"
#define DEFAULT_WIFI_PASS "66666666"
#define DEFAULT_LED_COLOR "#00FF00"

WebServer server(80);
Preferences prefs;

typedef struct {
  String ssid;
  String pass;
  String token; // optional auth token for future use
} NetCfg;

LedBlinker ledBlinker;

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
  // bool connectWiFiSTA(const char* ssid, const char* pass, uint32_t timeoutMs = 10000) {
  //   WiFi.mode(WIFI_STA);
  //   WiFi.begin(ssid, pass);
  //   Serial.printf("Connecting to WiFi SSID: %s\n", ssid);

  //   uint32_t start = millis();
  //   while (WiFi.status() != WL_CONNECTED && millis() - start < timeoutMs) {
  //     delay(200);
  //     Serial.print(".");
  //   }
  //   Serial.println();

  //   if (WiFi.status() == WL_CONNECTED) {
  //     Serial.printf("STA connected. IP: %s\n", WiFi.localIP().toString().c_str());
  //     return true;
  //   }
  //   Serial.println("STA connect failed, will start SoftAP.");
  //   return false;
  // }

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

  void initWiFi() {
    NetCfg cfg{};
    storage::loadNetCfg(cfg);

    WiFi.mode(WIFI_MODE_APSTA);
    bool ok = WiFi.softAP(cfg.ssid.c_str(), cfg.pass.c_str());
    Serial.println("\nWIFI ACCESS POINT (fallback)");
    Serial.printf("SSID: %s  PASS: %s\n", cfg.ssid.c_str(), cfg.pass.c_str());
    Serial.printf("AP IP: %s\n", WiFi.softAPIP().toString().c_str());
    // if (!connectWiFiSTA(cfg.ssid.c_str(), cfg.pass.c_str())) {
    //   // Fallback to AP using the same (or default) creds
      
    // } else {
    //   // mDNS so you can use http://m5.local
    //   if (MDNS.begin("m5")) {
    //     MDNS.addService("http", "tcp", 80);
    //     Serial.println("mDNS responder started as m5.local");
    //   } else {
    //     Serial.println("mDNS start failed");
    //   }
    // }
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
    
    NetCfg cfg {};
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
    
    storage::loadNetCfg(cfg);
    doc["ssid"] = cfg.ssid;
    doc["pass"] = cfg.pass;
    doc["token"] = cfg.token;
    
    json::sendJson(doc);
  }
} 


void initServer() {
    server.serveStatic("/", LittleFS, "/index.html");
    server.serveStatic("/app.js", LittleFS, "/app.js");
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

void setPixel(LedRGB rgb) {
  M5.dis.drawpix(0, CRGB(rgb.r, rgb.g, rgb.b));
}


void setup() {
  M5.begin(true, false, true);
  M5.dis.clear();
  ledBlinker.init(setPixel);

  // ledBlinker.set_solid({0, 100, 0});
  ledBlinker.set_blink({0, 100, 0}, {100, 0, 0});
  
  if (!LittleFS.begin(true)) {
    Serial.println("[ERROR]: Error has occurred with serial filesystem");
    return;
  }
  
  network::initWiFiSoftAP();
  initServer();

  Serial.println("[INFO]: M5 App Setup Done");
}

void loop() {
  M5.update();
  server.handleClient();
  ledBlinker.tick();
}
