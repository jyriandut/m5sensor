// http_server.cpp
#include "http_server.h"
#include "Arduino.h"
#include "wifi_manager.h"
#include "storage.h"
#include "pressure.h"
#include <LittleFS.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

#define API_TOKEN_NAME "M5-Api-Key"
// --- Helpers ---
inline void send_json(AsyncWebServerRequest* req, const JsonDocument &doc, int code = 200) {
  String out; serializeJson(doc, out);
  req->send(code, "application/json", out);
}
inline void send_plain(AsyncWebServerRequest* req, String message, int code = 200) {
  req->send(code, "text/plain", message);
}

namespace http_server {

  storage::WifiCredentials wifiCreds;
  static PressureReader pressure_reader = nullptr;

  bool validate_request(AsyncWebServerRequest* req) {
    auto header = req->getHeader(API_TOKEN_NAME);
    const String keyValue = header->value();
    Serial.println("Key from header " + keyValue);
    Serial.println("Creds from header " + wifiCreds.token);
    if (!keyValue.equals(wifiCreds.token)) {
      Serial.println("Tokens don't match");
      send_plain(req, "Unauthorized", 403);
      return false;
    }
    return true;
  }
  
  void init_ap_http_server(AsyncWebServer &server, LedBlinker &led_blinker) {
    server.on("/api/led", HTTP_GET, [&led_blinker](AsyncWebServerRequest *req) {
      JsonDocument doc;
      doc["color"] = led::rgbToHex(led_blinker.colorSolid);
      send_json(req, doc, 200);
    });

    server.on(
      "/api/led",
      HTTP_POST,
      [](AsyncWebServerRequest*) {
      },
      nullptr,
      [&led_blinker](AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t index, size_t total) {
        if (index + len != total) return;

        if (!req->hasHeader(API_TOKEN_NAME)) {
          send_plain(req, "Unauthorized", 403); return;
        }
        
        if (!req->contentType().equalsIgnoreCase("application/json")) {
          send_plain(req, "Expected Content-Type: application/json", 415);
          return;
        }

        JsonDocument payload;  // ArduinoJson v7 dynamic doc
        auto err = deserializeJson(payload, data, len);
        if (err) { send_plain(req, "Invalid JSON", 400); return; }

        const char *hex = payload["color"];
        if (!hex) { send_plain(req, "Missing 'color'", 400); return; }

        String h = String(hex);
        if (h.length() != 7 || h[0] != '#') {
          send_plain(req, "Expect color like \"#RRGGBB\"", 400);
          return;
        }

        LedRGB rgb = led::hexToRgb(hex);
        led_blinker.set_solid(rgb);

        JsonDocument out;
        out["color"] = led::rgbToHex(led_blinker.colorSolid);
        send_json(req, out, 200);
      }
    );

    // ---------- GET /api/wifi ----------
    server.on("/api/wifi", HTTP_GET, [](AsyncWebServerRequest *req) {
      JsonDocument doc;
      
      JsonArray arr = doc["networks"].to<JsonArray>();
      std::vector<wifi_manager::NetworkData> networks;
      wifi_manager::scan_wifi_networks(networks);
      
      Serial.printf("Found %u networks.\n", (unsigned)networks.size());
      for (const auto& n : networks) {
        JsonObject x = arr.add<JsonObject>();
        x["ssid"] = n.ssid;
        x["rssi"] = n.rssi;
      }

      storage::WifiCredentials wifi_creds;
      storage::load_wifi_credentials(wifi_creds);
      doc["ssid"]  = wifi_creds.ssid;
      doc["pass"]  = wifi_creds.pass;
      doc["token"] = wifi_creds.token;

      send_json(req, doc, 200);
    });

    server.on(
      "/api/wifi",
      HTTP_POST,
      [](AsyncWebServerRequest*) { },
      nullptr,
      [](AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t index, size_t total) {
        if (index + len != total) return;

        if (!req->contentType().equalsIgnoreCase("application/json")) {
          send_plain(req, "Expected Content-Type: application/json", 415);
          return;
        }

        JsonDocument payload;
        auto err = deserializeJson(payload, data, len);
        if (err) { send_plain(req, "Invalid JSON", 400); return; }

        const char *ssid = payload["ssid"];
        const char *pass = payload["pass"] | "";
        const char *token = payload["token"];

        if (!ssid || !ssid[0]) {
          send_plain(req, "Missing 'ssid'", 400);
          return;
        }
        if (!token || !token[0]) { send_plain(req, "Missing 'token'", 400); return; }

        storage::WifiCredentials wifi_creds;
        wifi_creds.ssid = ssid;
        wifi_creds.pass = pass;
        wifi_creds.token = token;

        storage::clear_wifi_credentials();
        if (!storage::save_wifi_credentials(wifi_creds)) {
          send_plain(req, "Failed to save preferences, try again.", 400);
          return;
        }

        JsonDocument out;
        out["status"] = "success";
        send_json(req, out, 200);
      }
    );

    // Not Found
    server.onNotFound([](AsyncWebServerRequest *req) {
      send_plain(req, "Not Found", 404);
    });

    // Serve LittleFS:/ with index.html by default
    server.serveStatic("/", LittleFS, "/")
      .setDefaultFile("index.html")
      .setCacheControl("public, max-age=86400");
    
    server.begin();
  }

  bool init_client_http_server(AsyncWebServer &server, AsyncWebSocket &ws, PressureReader reader) {
    storage::load_wifi_credentials(wifiCreds);
    pressure_reader = reader;

    if (wifiCreds.token == "" || wifiCreds.token == nullptr) {
      Serial.println("API token not available. Resetting back to Provisioning mode");
      return false;
    }
    
    server.onNotFound([](AsyncWebServerRequest *req) {
      send_plain(req, "Not Found", 404);
    });

    server.on("/api/pressure", HTTP_GET, [](AsyncWebServerRequest *req) {
      if (!http_server::validate_request(req)) {
        Serial.println("Request is not valid");
        return;
      }
      if (!pressure_reader) {
        send_plain(req, "Pressure module disabled", 503);
        return;
      }
      JsonDocument doc;

      float pressure = pressure_reader();
      doc["pressure"] = pressure;
      send_json(req, doc, 200);
    });
    
    server.serveStatic("/", LittleFS, "/")
        .setDefaultFile("main.html")
        .setCacheControl("public, max-age=86400");

    
    server.addHandler(&ws);
    server.begin();
    return true;
  }
  
}
