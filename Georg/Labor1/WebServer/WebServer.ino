/*
*******************************************************************************
* M5Stack Atom Lite — WiFi AP + LED Color Control (SIMPLE VERSION)
* 
* FEATURES:
* - Single page interface - everything on one page
* - LED color picker with live preview
* - Basic WiFi credentials form (no validation)
* - Saves settings to NVS flash memory
* 
* ENDPOINTS:
* - /      -> Main page (color picker + WiFi settings form)
* - /set   -> Applies LED color from ?value=%23RRGGBB
* - /wifi  -> Saves SSID/password/token to flash (no password verification)
* 
* LIMITATIONS:
* - No password validation or confirmation
* - Shows saved password in plain text on form
* - No separate settings page
* - Minimal error handling
* 
* USE CASE: Quick setup, testing, or when security is not a concern
* 
* Access Point: M5Stack_Ap / 66666666
* Default IP: 192.168.4.1
*******************************************************************************
*/

#include <M5Atom.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>
#include <Preferences.h>

const char* ap_ssid     = "M5Stack_Ap";
const char* ap_password = "66666666";   // >= 8 chars for WPA2 AP

WiFiServer server(80);
Preferences prefs;

// Persisted config keys (NVS namespace "netcfg")
struct NetCfg {
  String ssid;
  String pass;
  String token; // optional auth token for future use
} netcfg;

// Store current LED color as 0xRRGGBB
uint32_t currentColor = 0x00ff00; // default: green

// --- Helpers ---

// URL-decode minimal subset: %XX and '+' -> space
String urlDecode(const String& s) {
  String out; out.reserve(s.length());
  for (size_t i = 0; i < s.length(); ++i) {
    char c = s[i];
    if (c == '+') { out += ' '; }
    else if (c == '%' && i + 2 < s.length()) {
      auto hexVal = [](char h)->int{
        if (h>='0'&&h<='9') return h-'0';
        if (h>='A'&&h<='F') return 10+(h-'A');
        if (h>='a'&&h<='f') return 10+(h-'a');
        return -1;
      };
      int v1 = hexVal(s[i+1]), v2 = hexVal(s[i+2]);
      if (v1>=0 && v2>=0) { out += char((v1<<4)|v2); i+=2; }
      else out += c;
    } else out += c;
  }
  return out;
}

// Parse "#RRGGBB" or "RRGGBB" -> 0xRRGGBB
bool parseHexColor(String hex, uint32_t& outColor) {
  if (hex.length()==7 && hex[0]=='#') hex.remove(0,1);
  if (hex.length()!=6) return false;
  char buf[7]; hex.toCharArray(buf,7);
  char* endptr=nullptr;
  uint32_t val = strtoul(buf, &endptr, 16);
  if (*endptr!='\0') return false;
  outColor = val & 0xFFFFFF;
  return true;
}

void applyColor(uint32_t color) {
  uint8_t r=(color>>16)&0xFF, g=(color>>8)&0xFF, b=color&0xFF;
  M5.dis.drawpix(0, CRGB(r,g,b));
}

// load/save settings
void loadNetCfg() {
  prefs.begin("netcfg", true);
  netcfg.ssid  = prefs.getString("ssid",  "");
  netcfg.pass  = prefs.getString("pass",  "");
  netcfg.token = prefs.getString("token", "");
  prefs.end();
}

void saveNetCfg(const String& ssid, const String& pass, const String& token) {
  prefs.begin("netcfg", false);
  prefs.putString("ssid",  ssid);
  prefs.putString("pass",  pass);
  prefs.putString("token", token);
  prefs.end();
  netcfg.ssid  = ssid;
  netcfg.pass  = pass;
  netcfg.token = token;
}

// --- HTTP ---

void sendHtml(WiFiClient& client, const char* statusMsg = nullptr) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html; charset=utf-8");
  client.println("Connection: close");
  client.println();

  char hexbuf[8];
  snprintf(hexbuf, sizeof(hexbuf), "#%06X", currentColor);

  client.print(
    "<!DOCTYPE html><html><head><meta charset='utf-8'>"
    "<meta name='viewport' content='width=device-width, initial-scale=1'>"
    "<title>M5 Atom Color</title>"
    "<style>body{font-family:system-ui,sans-serif;margin:1.2rem}"
    "label{display:inline-block;min-width:9rem}"
    "input,button{padding:.5rem;border:1px solid #ccc;border-radius:.5rem}"
    "form{margin:.8rem 0} .box{margin-top:.6rem;width:140px;height:40px;border:1px solid #ccc}"
    ".muted{color:#666;font-size:.9em}</style></head><body>"
    "<h1>M5 Atom Lite — LED Color</h1>"
  );

  if (statusMsg) {
    client.print("<p class='muted'>"); client.print(statusMsg); client.println("</p>");
  }

  client.print("<p>Current color: <b>");
  client.print(hexbuf);
  client.println("</b></p>");

  // Color form
  client.print("<form action='/set' method='get'>"
               "<label for='color'>Choose color:</label>"
               "<input type='color' id='color' name='value' value='");
  client.print(hexbuf);
  client.println("'> <button type='submit'>Apply</button></form>");

  client.print("<div class='box' style='background:"); client.print(hexbuf); client.println("'></div>");

  // Settings form (no JS)
  client.println("<h2>Settings (stored in flash)</h2>");
  client.println("<form action='/wifi' method='get'>");
  client.print("<label>WiFi SSID:</label><input name='ssid' value='");
  client.print(netcfg.ssid); client.println("'><br>");
  client.print("<label>WiFi Password:</label><input name='pass' value='");
  client.print(netcfg.pass); client.println("'><br>");
  client.print("<label>Auth token (opt):</label><input name='token' value='");
  client.print(netcfg.token); client.println("'><br>");
  client.println("<button type='submit'>Save</button></form>");

  client.println("<p class='muted'>V1: device runs AP only. Saved SSID/PASS/TOKEN will be used in future STA mode.</p>");
  client.println("</body></html>");
}

// Extract query value by key= from "k1=v1&key=VALUE&k3=v3"
String queryGet(const String& query, const String& key) {
  String k = key + "=";
  int p = query.indexOf(k);
  if (p < 0) return "";
  int s = p + k.length();
  int e = query.indexOf('&', s);
  if (e < 0) e = query.length();
  return query.substring(s, e);
}

bool handleSetPath(const String& requestLine) {
  int sp1 = requestLine.indexOf(' ');
  int sp2 = requestLine.indexOf(' ', sp1 + 1);
  if (sp1 < 0 || sp2 < 0) return false;
  String path = requestLine.substring(sp1 + 1, sp2);
  if (!path.startsWith("/set")) return false;

  int qpos = path.indexOf('?');
  if (qpos < 0) return false;
  String query = path.substring(qpos + 1);

  String rawVal = queryGet(query, "value");
  if (rawVal == "") return false;

  String val = urlDecode(rawVal); // "#rrggbb"
  uint32_t parsed;
  if (parseHexColor(val, parsed)) {
    currentColor = parsed;
    applyColor(currentColor);
    return true;
  }
  return false;
}

// returns status string to show on page
const char* handleWifiPath(const String& requestLine) {
  static String status; status = "";

  int sp1 = requestLine.indexOf(' ');
  int sp2 = requestLine.indexOf(' ', sp1 + 1);
  if (sp1 < 0 || sp2 < 0) return nullptr;
  String path = requestLine.substring(sp1 + 1, sp2);
  if (!path.startsWith("/wifi")) return nullptr;

  int qpos = path.indexOf('?');
  if (qpos < 0) { status = "No query"; return status.c_str(); }
  String query = path.substring(qpos + 1);

  String ssid  = urlDecode(queryGet(query, "ssid"));
  String pass  = urlDecode(queryGet(query, "pass"));
  String token = urlDecode(queryGet(query, "token"));

  // minimal validation
  if (ssid.length()==0) { status = "SSID empty"; return status.c_str(); }
  // pass may be empty for open networks in future, but for WPA it should be >=8
  if (pass.length()>0 && pass.length()<8) { status = "Password must be >= 8 chars"; return status.c_str(); }

  saveNetCfg(ssid, pass, token);
  status = "Saved";
  return status.c_str();
}

void setup() {
  M5.begin(true, false, true);
  delay(50);
  loadNetCfg();
  applyColor(currentColor);

  Serial.println("\nWIFI ACCESS POINT (V1)");
  Serial.printf("Connect to: %s\nOpen: http://", ap_ssid);
  WiFi.softAP(ap_ssid, ap_password);
  IPAddress myIP = WiFi.softAPIP();
  Serial.println(myIP);

  server.begin();
}

void loop() {
  WiFiClient client = server.available();
  if (!client) return;

  String currentLine = "", requestLine = "";
  const char* statusMsgToShow = nullptr;

  while (client.connected()) {
    if (!client.available()) { delay(1); continue; }
    char c = client.read();

    if (c == '\n') {
      if (currentLine.length() == 0) {
        // headers ended
        if (requestLine.startsWith("GET ")) {
          // apply /set (if any)
          handleSetPath(requestLine);
          // process /wifi (if any) and capture status message
          statusMsgToShow = handleWifiPath(requestLine);
        }
        // render UI
        sendHtml(client, statusMsgToShow);
        break;
      } else {
        if (requestLine.length() == 0) requestLine = currentLine;
        currentLine = "";
      }
    } else if (c != '\r') {
      currentLine += c;
    }
  }
  client.stop();
}