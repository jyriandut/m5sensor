/*
*******************************************************************************
* M5Stack Atom Lite — WiFi AP + HTML <input type="color"> + Settings (no JS)
* - AP mode only (V1)
* - /      -> HTML (color picker + settings form)
* - /set   -> applies LED color from ?value=%23RRGGBB
* - /wifi  -> saves ssid/pass (and optional token) into NVS
*******************************************************************************
*/

#include "ArduinoJson.hpp"
#include "ArduinoJson/Json/JsonSerializer.hpp"
#include <M5Atom.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <WebServer.h>
#include <LittleFS.h>

#define NET_CFG "netcfg"

const char* ap_ssid     = "M5Stack_Ap";
const char* ap_password = "66666666";   // >= 8 chars for WPA2 AP

WebServer server(80);
Preferences prefs;

// Persisted config keys (NVS namespace "netcfg")
struct NetCfg {
  String ssid;
  String pass;
  String token; // optional auth token for future use
} netcfg;

// Store current LED color as 0xRRGGBB
String currentHex = "00ff00"; // default: green

void loadNetCfg() {
  prefs.begin(NET_CFG, true);
  netcfg.ssid  = prefs.getString("ssid",  "");
  netcfg.pass  = prefs.getString("pass",  "");
  netcfg.token = prefs.getString("token", "");
  prefs.end();
}

void saveNetCfg(const String& ssid, const String& pass, const String& token) {
  prefs.begin(NET_CFG, false);
  prefs.putString("ssid",  ssid);
  prefs.putString("pass",  pass);
  prefs.putString("token", token);
  prefs.end();
  netcfg.ssid  = ssid;
  netcfg.pass  = pass;
  netcfg.token = token;
}

// --- Helpers ---
uint32_t hexToRgb(String hex) {
  // Expect "#RRGGBB"
  if (hex.length() != 7 || hex.charAt(0) != '#') return 0;
  uint8_t r = strtoul(hex.substring(1,3).c_str(), nullptr, 16);
  uint8_t g = strtoul(hex.substring(3,5).c_str(), nullptr, 16);
  uint8_t b = strtoul(hex.substring(5,7).c_str(), nullptr, 16);
  return (r << 16) | (g << 8) | b;
}

void setLedHex(const String& hex) {
  uint32_t rgb = hexToRgb(hex);
  uint8_t r = (rgb >> 16) & 0xFF;
  uint8_t g = (rgb >> 8) & 0xFF;
  uint8_t b = rgb & 0xFF;
  M5.dis.drawpix(0, CRGB(r, g, b));
  currentHex = hex;
}

void sendJson(const JsonDocument &doc, int code = 200) {
  String out;
  serializeJson(doc, out);
  
  server.send(code, "application/json", out);
}

void handlePostLed() {
  if (server.method() != HTTP_POST) { server.send(405, "text/plain", "Method Not Allowed"); return; }
  if (!server.hasArg("plain")) { server.send(400, "text/plain", "Missing body"); return; }

  JsonDocument in;

  auto err = deserializeJson(in, server.arg("plain"));
  if (err) { server.send(400, "text/plain", "Invalid JSON"); return; }

  const char *hex = in["color"];

  if (!hex) {
    server.send(400, "text/plain", "Missing 'color'");
    return;
  }
  
  String h = String(hex);
  if (h.length() != 7 || h[0] != '#') { server.send(400, "text/plain", "Expect color like \"#RRGGBB\""); return; }

  setLedHex(h);

  JsonDocument out;

  out["color"] = currentHex;
  
  String s;
  serializeJson(out, s);
  server.send(200, "application/json", s);
}

void handleGetLed() {
  JsonDocument doc;

  doc["color"] = currentHex;

  sendJson(doc);
}

void initServer() {
  server.serveStatic("/", LittleFS, "/index.html");
  server.serveStatic("/index.js", LittleFS, "/index.js");
  server.serveStatic("/pico.lime.min.css", LittleFS, "/pico.lime.min.css");
  server.serveStatic("/alpine.min.js", LittleFS, "/alpine.min.js");

  server.on("/api/led", HTTP_GET, handleGetLed);
  server.on("/api/led", HTTP_POST, handlePostLed);

  
  // Fallback
  server.onNotFound([]() {
    server.send(404, "text/plain", "Not Found");
  });
  
  server.begin();
}

void initWiFiSoftAP() {
  loadNetCfg();
  Serial.println("\nWIFI ACCESS POINT (V1)");
  Serial.printf("Connect to: %s\nOpen: http://", ap_ssid);
  
  WiFi.softAP(ap_ssid, ap_password);
  IPAddress myIP = WiFi.softAPIP();
  Serial.printf("Server is running on %s \n", myIP.toString());
}

void setup() {
  M5.begin(true, false, true);
  M5.dis.clear();
  setLedHex("#00FF00");

  if (!LittleFS.begin(true)) {
    Serial.println("[ERROR]: Error has occurred with serial filesystem");
    return;
  }
  delay(50);
  initWiFiSoftAP();
  initServer();

  Serial.println("[INFO]: M5 App Setup Done");
}

void loop() {
  server.handleClient();
  delay(2);
}
