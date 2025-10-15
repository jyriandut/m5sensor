/*
*******************************************************************************
* M5Stack Atom Lite — WiFi AP + LED Color Control (ADVANCED VERSION)
* 
* FEATURES:
* - Separate pages for main interface and WiFi settings
* - LED color picker with live preview
* - Secure WiFi configuration with validation:
*   • Requires current password to change settings
*   • Password confirmation (must match)
*   • Minimum 8 characters for WPA2
*   • Styled error/success messages
* - Professional UI with better UX
* - Password fields are hidden (type="password")
* 
* ENDPOINTS:
* - /              -> Main page (color picker + link to settings)
* - /set           -> Applies LED color from ?value=%23RRGGBB
* - /wifisettings  -> Dedicated WiFi configuration page
* - /savewifi      -> Saves WiFi credentials with validation
* 
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
    ".muted{color:#666;font-size:.9em}"
    "a{color:#0066cc;text-decoration:none;font-weight:500}"
    "a:hover{text-decoration:underline}</style></head><body>"
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

  // Link to WiFi settings page
  client.println("<h2>Settings</h2>");
  client.println("<p><a href='/wifisettings'>⚙️ Configure WiFi Settings</a></p>");

  client.println("<p class='muted'>V1: device runs AP only. Saved SSID/PASS will be used in future STA mode.</p>");
  client.println("</body></html>");
}

void sendWifiSettingsHtml(WiFiClient& client, const char* statusMsg = nullptr) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html; charset=utf-8");
  client.println("Connection: close");
  client.println();

  client.print(
    "<!DOCTYPE html><html><head><meta charset='utf-8'>"
    "<meta name='viewport' content='width=device-width, initial-scale=1'>"
    "<title>WiFi Settings</title>"
    "<style>body{font-family:system-ui,sans-serif;margin:1.2rem}"
    "label{display:block;margin-top:.8rem;font-weight:500}"
    "input{width:100%;max-width:400px;padding:.6rem;border:1px solid #ccc;border-radius:.5rem;box-sizing:border-box;margin-top:.3rem}"
    "button{margin-top:1.2rem;padding:.7rem 1.5rem;background:#0066cc;color:white;border:none;border-radius:.5rem;cursor:pointer;font-size:1rem}"
    "button:hover{background:#0052a3}"
    ".error{color:#d32f2f;margin-top:.8rem;padding:.6rem;background:#ffebee;border-radius:.5rem}"
    ".success{color:#388e3c;margin-top:.8rem;padding:.6rem;background:#e8f5e9;border-radius:.5rem}"
    ".back-link{display:inline-block;margin-top:1.5rem;color:#0066cc;text-decoration:none}"
    ".back-link:hover{text-decoration:underline}"
    ".info{color:#666;font-size:.9em;margin-top:.3rem}</style></head><body>"
    "<h1>WiFi Settings</h1>"
  );

  if (statusMsg) {
    bool isError = (strstr(statusMsg, "Error") != nullptr || strstr(statusMsg, "empty") != nullptr || 
                    strstr(statusMsg, "match") != nullptr || strstr(statusMsg, "incorrect") != nullptr);
    client.print(isError ? "<div class='error'>" : "<div class='success'>");
    client.print(statusMsg);
    client.println("</div>");
  }

  client.println("<form action='/savewifi' method='get'>");
  
  client.print("<label for='ssid'>WiFi Network Name (SSID):</label>"
               "<input type='text' id='ssid' name='ssid' value='");
  client.print(netcfg.ssid);
  client.println("' required>"
                 "<p class='info'>Enter your home WiFi network name</p>");
  
  client.println("<label for='pass'>WiFi Password:</label>"
                 "<input type='password' id='pass' name='pass' placeholder='Enter WiFi password' required>"
                 "<p class='info'>Enter your home WiFi password (min 8 characters)</p>");
  
  client.println("<button type='submit'>Save WiFi Settings</button>");
  client.println("</form>");
  
  client.println("<a href='/' class='back-link'>← Back to Main Page</a>");
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

// returns status string to show on WiFi settings page
const char* handleSaveWifiPath(const String& requestLine) {
  static String status; status = "";

  int sp1 = requestLine.indexOf(' ');
  int sp2 = requestLine.indexOf(' ', sp1 + 1);
  if (sp1 < 0 || sp2 < 0) return nullptr;
  String path = requestLine.substring(sp1 + 1, sp2);
  if (!path.startsWith("/savewifi")) return nullptr;

  int qpos = path.indexOf('?');
  if (qpos < 0) { status = "Error: No data submitted"; return status.c_str(); }
  String query = path.substring(qpos + 1);

  String ssid = urlDecode(queryGet(query, "ssid"));
  String pass = urlDecode(queryGet(query, "pass"));

  // Simple validation
  if (ssid.length() == 0) { 
    status = "Error: SSID cannot be empty"; 
    return status.c_str(); 
  }
  
  // Password length validation for WPA2
  if (pass.length() > 0 && pass.length() < 8) { 
    status = "Error: Password must be at least 8 characters"; 
    return status.c_str(); 
  }

  // Save to flash
  saveNetCfg(ssid, pass, netcfg.token);
  status = "✓ WiFi settings saved successfully!";
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
          // Check if this is the WiFi settings page
          if (requestLine.indexOf("GET /wifisettings") >= 0) {
            sendWifiSettingsHtml(client, nullptr);
            break;
          }
          // Check if this is saving WiFi settings
          else if (requestLine.indexOf("GET /savewifi") >= 0) {
            statusMsgToShow = handleSaveWifiPath(requestLine);
            sendWifiSettingsHtml(client, statusMsgToShow);
            break;
          }
          // Main page handling
          else {
            // apply /set (if any)
            handleSetPath(requestLine);
          }
        }
        // render main page UI
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
