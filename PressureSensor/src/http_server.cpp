// http_server.cpp
#include "http_server.h"
#include "wifi_manager.h"
#include "storage.h"

#include <LittleFS.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

// --- Helpers ---
inline void send_json(AsyncWebServerRequest* req, const JsonDocument &doc, int code = 200) {
  String out; serializeJson(doc, out);
  req->send(code, "application/json", out);
}
inline void send_plain(AsyncWebServerRequest* req, String message, int code = 200) {
  req->send(code, "text/plain", message);
}

namespace http_server {

  void init_ap_http_server(AsyncWebServer &server, LedBlinker& led_blinker) {
    // ---------- GET /api/led ----------
    server.on("/api/led", HTTP_GET, [&led_blinker](AsyncWebServerRequest *req) {
      JsonDocument doc;
      doc["color"] = led::rgbToHex(led_blinker.colorSolid);
      send_json(req, doc, 200);
    });

    // ---------- POST /api/led  (expects {"color":"#RRGGBB"}) ----------
    server.on(
      "/api/led",
      HTTP_POST,
      [](AsyncWebServerRequest*) {
      },
      nullptr,
      [&led_blinker](AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t index, size_t total) {
        if (index + len != total) return;

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

        if (!ssid || !ssid[0]) { send_plain(req, "Missing 'ssid'", 400); return; }

        storage::WifiCredentials wifi_creds;
        wifi_creds.ssid = ssid;
        wifi_creds.pass = pass;

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

  void init_client_http_server(AsyncWebServer &server, AsyncWebSocket& ws) {
    server.onNotFound([](AsyncWebServerRequest *req) {
      send_plain(req, "Not Found", 404);
    });

    server.serveStatic("/", LittleFS, "/")
        .setDefaultFile("main.html")
        .setCacheControl("public, max-age=86400");

    
    server.addHandler(&ws);
    server.begin();
  }
  
}
