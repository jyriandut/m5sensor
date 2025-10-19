/*
*******************************************************************************
* M5Stack Atom Lite — WiFi AP + LED Color Control + Pressure Sensor Display
* 
* FEATURES:
* - Separate pages for main interface, WiFi settings, and pressure monitoring
* - LED color picker with live preview
* - Real-time pressure sensor readings from MPX5700AP (15-700 kPa)
* - Secure WiFi configuration with validation
* - Professional UI with better UX
* 
* ENDPOINTS:
* - /              -> Main page (color picker + links to settings and pressure)
* - /set           -> Applies LED color from ?value=%23RRGGBB
* - /wifisettings  -> Dedicated WiFi configuration page
* - /savewifi      -> Saves WiFi credentials with validation
* - /pressure      -> Pressure sensor monitoring page
* - /getpressure   -> JSON API endpoint for current pressure reading
* 
* Hardware:
* - M5Stack Atom Lite
* - MPX5700AP Pressure Sensor on GPIO 32
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

// Pressure sensor configuration
const int analogPin = 32;  // MPX5700AP connected to GPIO 32

// Persisted config keys (NVS namespace "netcfg")
struct NetCfg {
  String ssid;
  String pass;
  String token; // optional auth token for future use
} netcfg;

// Store current LED color as 0xRRGGBB
uint32_t currentColor = 0x00ff00; // default: green

// --- Pressure Sensor Functions ---

// Convert MPX5700AP analog reading to kPa
// MPX5700AP: 15-700 kPa range, 0.2V-4.7V output
// ESP32 ADC: 12-bit (0-4095), reference voltage ~3.3V
float readPressureKPa() {
  int rawValue = analogRead(analogPin);  // 0-4095
  
  // Convert ADC reading to voltage (0-3.3V)
  float voltage = (rawValue / 4095.0) * 3.3;
  
  // MPX5700AP transfer function: Vout = Vs * (0.0012858 * P + 0.04)
  // Where Vs = 5V (supply voltage), P = pressure in kPa
  // Rearranging: P = (Vout/Vs - 0.04) / 0.0012858
  // For 3.3V supply: P = (Vout/3.3 - 0.04) / 0.0012858
  
  // Simplified linear conversion for 15-700 kPa range:
  // At 15 kPa: ~0.2V, At 700 kPa: ~4.7V (assuming 5V supply)
  // Adjusted for 3.3V ADC reference:
  float pressureKPa = ((voltage - 0.2) / (4.7 - 0.2)) * (700.0 - 15.0) + 15.0;
  
  // Clamp to valid range
  if (pressureKPa < 0) pressureKPa = 0;
  if (pressureKPa > 700) pressureKPa = 700;
  
  return pressureKPa;
}

// --- Helper Functions ---

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

// --- HTTP Response Functions ---

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
    "<title>M5 Atom Control</title>"
    "<style>body{font-family:system-ui,sans-serif;margin:1.2rem}"
    "label{display:inline-block;min-width:9rem}"
    "input,button{padding:.5rem;border:1px solid #ccc;border-radius:.5rem}"
    "form{margin:.8rem 0} .box{margin-top:.6rem;width:140px;height:40px;border:1px solid #ccc}"
    ".muted{color:#666;font-size:.9em}"
    "a{color:#0066cc;text-decoration:none;font-weight:500}"
    "a:hover{text-decoration:underline}"
    ".nav-links{margin:1.5rem 0;padding:1rem;background:#f5f5f5;border-radius:.5rem}"
    ".nav-links a{display:block;margin:.5rem 0}</style></head><body>"
    "<h1>M5 Atom Lite — Control Panel</h1>"
  );

  if (statusMsg) {
    client.print("<p class='muted'>"); client.print(statusMsg); client.println("</p>");
  }

  client.print("<h2>LED Color Control</h2>");
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

  // Navigation links
  client.println("<div class='nav-links'>");
  client.println("<h2>Navigation</h2>");
  client.println("<a href='/pressure'>📊 View Pressure Sensor Data</a>");
  client.println("<a href='/wifisettings'>⚙️ Configure WiFi Settings</a>");
  client.println("</div>");

  client.println("<p class='muted'>Device runs in AP mode. Saved WiFi credentials will be used in future STA mode.</p>");
  client.println("</body></html>");
}

void sendPressurePage(WiFiClient& client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html; charset=utf-8");
  client.println("Connection: close");
  client.println();

  client.print(
    "<!DOCTYPE html><html><head><meta charset='utf-8'>"
    "<meta name='viewport' content='width=device-width, initial-scale=1'>"
    "<title>Pressure Monitor</title>"
    "<style>body{font-family:system-ui,sans-serif;margin:1.2rem;background:#f5f5f5}"
    ".container{max-width:800px;margin:0 auto;background:white;padding:2rem;border-radius:.5rem;box-shadow:0 2px 8px rgba(0,0,0,0.1)}"
    "h1{color:#333;margin-top:0}"
    ".reading{font-size:3rem;font-weight:bold;color:#0066cc;text-align:center;margin:2rem 0;padding:1.5rem;background:#e3f2fd;border-radius:.5rem}"
    ".unit{font-size:1.5rem;color:#666;margin-left:.5rem}"
    ".info{background:#fff3cd;padding:1rem;border-radius:.5rem;margin:1rem 0;border-left:4px solid #ffc107}"
    ".raw{color:#666;font-size:.9em;text-align:center;margin-top:.5rem}"
    "a{color:#0066cc;text-decoration:none;font-weight:500;display:inline-block;margin-top:1.5rem}"
    "a:hover{text-decoration:underline}"
    ".status{text-align:center;color:#666;font-size:.9em;margin-top:1rem}</style>"
    "<script>"
    "function updatePressure(){"
    "fetch('/getpressure').then(r=>r.json()).then(data=>{"
    "document.getElementById('pressure').textContent=data.pressure_kpa.toFixed(2);"
    "document.getElementById('raw').textContent='Raw ADC: '+data.raw_value;"
    "document.getElementById('voltage').textContent='Voltage: '+data.voltage.toFixed(3)+'V';"
    "}).catch(e=>console.error('Error:',e));}"
    "setInterval(updatePressure,500);"
    "updatePressure();"
    "</script></head><body>"
    "<div class='container'>"
    "<h1>📊 Pressure Sensor Monitor</h1>"
    "<div class='info'>"
    "<strong>Sensor:</strong> MPX5700AP<br>"
    "<strong>Range:</strong> 15-700 kPa<br>"
    "<strong>Update Rate:</strong> 2 Hz (500ms)"
    "</div>"
    "<div class='reading'>"
    "<span id='pressure'>--</span><span class='unit'>kPa</span>"
    "</div>"
    "<div class='raw' id='raw'>Raw ADC: --</div>"
    "<div class='raw' id='voltage'>Voltage: --</div>"
    "<div class='status'>Live updating...</div>"
    "<a href='/'>← Back to Main Page</a>"
    "</div></body></html>"
  );
}

void sendPressureJson(WiFiClient& client) {
  int rawValue = analogRead(analogPin);
  float voltage = (rawValue / 4095.0) * 3.3;
  float pressureKPa = readPressureKPa();

  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: application/json");
  client.println("Connection: close");
  client.println();
  
  client.print("{");
  client.print("\"pressure_kpa\":");
  client.print(pressureKPa, 2);
  client.print(",\"raw_value\":");
  client.print(rawValue);
  client.print(",\"voltage\":");
  client.print(voltage, 3);
  client.println("}");
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
  
  Serial.begin(115200);
  delay(1000);
  
  loadNetCfg();
  applyColor(currentColor);

  Serial.println("\n═══════════════════════════════════════════════");
  Serial.println("  M5 ATOM LITE - WiFi AP + Pressure Monitor");
  Serial.println("═══════════════════════════════════════════════");
  Serial.printf("Connect to: %s\n", ap_ssid);
  Serial.printf("Password: %s\n", ap_password);
  
  WiFi.softAP(ap_ssid, ap_password);
  IPAddress myIP = WiFi.softAPIP();
  
  Serial.printf("\nIP Address: http://");
  Serial.println(myIP);
  Serial.println("\nEndpoints:");
  Serial.println("  /           - Main control panel");
  Serial.println("  /pressure   - Pressure sensor monitor");
  Serial.println("  /wifisettings - WiFi configuration");
  Serial.println("═══════════════════════════════════════════════\n");

  server.begin();
  
  // Blink LED to indicate ready
  for (int i = 0; i < 3; i++) {
    M5.dis.drawpix(0, CRGB::Green);
    delay(200);
    M5.dis.drawpix(0, CRGB::Black);
    delay(200);
  }
  applyColor(currentColor);
}

void loop() {
  WiFiClient client = server.available();
  if (!client) {
    // Log pressure to serial monitor every 500ms when no client connected
    static unsigned long lastPrint = 0;
    if (millis() - lastPrint > 500) {
      float pressure = readPressureKPa();
      Serial.print("Pressure: ");
      Serial.print(pressure, 2);
      Serial.println(" kPa");
      lastPrint = millis();
    }
    return;
  }

  String currentLine = "", requestLine = "";
  const char* statusMsgToShow = nullptr;

  while (client.connected()) {
    if (!client.available()) { delay(1); continue; }
    char c = client.read();

    if (c == '\n') {
      if (currentLine.length() == 0) {
        // headers ended
        if (requestLine.startsWith("GET ")) {
          // Check for pressure page
          if (requestLine.indexOf("GET /pressure") >= 0 && requestLine.indexOf("GET /getpressure") < 0) {
            sendPressurePage(client);
            break;
          }
          // Check for pressure JSON API
          else if (requestLine.indexOf("GET /getpressure") >= 0) {
            sendPressureJson(client);
            break;
          }
          // Check if this is the WiFi settings page
          else if (requestLine.indexOf("GET /wifisettings") >= 0) {
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
