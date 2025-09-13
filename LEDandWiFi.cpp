/*
*******************************************************************************
* M5Stack Atom Lite — WiFi AP + HTML <input type="color"> (no JS)
* - Starts a WiFi Access Point
* - Serves an HTML page with a color picker form
* - Handles GET /set?value=%23RRGGBB, decodes and applies color to LED
*******************************************************************************
*/

#include <M5Atom.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>

const char* ssid     = "M5Stack_Ap";
const char* password = "66666666";

WiFiServer server(80);

// Store current LED color as 0xRRGGBB
uint32_t currentColor = 0x00ff00; // default: green

// --------- Helpers ---------

// URL-decode minimal subset: handles %XX and '+' -> space
// Enough for "%23" coming from <input type="color">
String urlDecode(const String& s) {
  String out;
  out.reserve(s.length());
  for (size_t i = 0; i < s.length(); ++i) {
    char c = s[i];
    if (c == '+') {
      out += ' ';
    } else if (c == '%' && i + 2 < s.length()) {
      char h1 = s[i + 1];
      char h2 = s[i + 2];
      auto hexVal = [](char h) -> int {
        if (h >= '0' && h <= '9') return h - '0';
        if (h >= 'A' && h <= 'F') return 10 + (h - 'A');
        if (h >= 'a' && h <= 'f') return 10 + (h - 'a');
        return -1;
      };
      int v1 = hexVal(h1), v2 = hexVal(h2);
      if (v1 >= 0 && v2 >= 0) {
        out += char((v1 << 4) | v2);
        i += 2;
      } else {
        // Invalid % sequence: keep as-is
        out += c;
      }
    } else {
      out += c;
    }
  }
  return out;
}

// Parse "#RRGGBB" or "RRGGBB" into 0xRRGGBB (uint32)
bool parseHexColor(String hex, uint32_t& outColor) {
  // Remove leading '#'
  if (hex.length() == 7 && hex[0] == '#') hex.remove(0, 1);
  if (hex.length() != 6) return false;

  // Convert 6 hex digits to number
  char buf[7];
  hex.toCharArray(buf, 7);
  char* endptr = nullptr;
  uint32_t val = strtoul(buf, &endptr, 16);
  if (*endptr != '\0') return false;

  outColor = val & 0xFFFFFF; // ensure 24-bit
  return true;
}

// Apply color to Atom's single RGB LED
void applyColor(uint32_t color) {
  // color is 0xRRGGBB; CRGB expects (r,g,b)
  uint8_t r = (color >> 16) & 0xFF;
  uint8_t g = (color >> 8)  & 0xFF;
  uint8_t b = (color)       & 0xFF;
  M5.dis.drawpix(0, CRGB(r, g, b));
}

// Build and send the HTML page (shows current color and a form)
void sendHtml(WiFiClient& client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html; charset=utf-8");
  client.println("Connection: close");
  client.println();

  // Build current color as "#RRGGBB"
  char hexbuf[8];
  snprintf(hexbuf, sizeof(hexbuf), "#%06X", currentColor);

  client.print(F(
    "<!DOCTYPE html><html><head><meta charset='utf-8'>"
    "<meta name='viewport' content='width=device-width, initial-scale=1'>"
    "<title>M5 Atom Color</title></head><body>"
    "<h1>M5 Atom Lite — LED Color</h1>"));

  client.print("<p>Current color: <b>");
  client.print(hexbuf);
  client.println("</b></p>");

  // Simple form, no JS. On submit it calls /set?value=%23RRGGBB
  client.print("<form action='/set' method='get'>"
               "<label for='color'>Choose color:</label> "
               "<input type='color' id='color' name='value' value='");
  client.print(hexbuf);
  client.println("'> "
                 "<button type='submit'>Apply</button>"
                 "</form>");

  // Small preview box
  client.print("<div style='margin-top:1rem;width:120px;height:40px;"
               "border:1px solid #ccc;background:");
  client.print(hexbuf);
  client.println("'></div>");

  client.println("</body></html>");
}

// Try to handle GET /set?value=... ; returns true if handled (and LED set)
bool handleSetPath(const String& requestLine) {
  // Example first line: "GET /set?value=%23ff00aa HTTP/1.1"
  // We only need the path+query part
  int sp1 = requestLine.indexOf(' ');
  if (sp1 < 0) return false;
  int sp2 = requestLine.indexOf(' ', sp1 + 1);
  if (sp2 < 0) return false;
  String path = requestLine.substring(sp1 + 1, sp2); // "/set?value=%23ff00aa"

  // Must start with "/set?"
  if (!path.startsWith("/set")) return false;

  int qpos = path.indexOf('?');
  if (qpos < 0) return false;

  String query = path.substring(qpos + 1); // e.g. "value=%23ff00aa"
  // Very small parser for "value=..."
  const String key = "value=";
  int kv = query.indexOf(key);
  if (kv < 0) return false;

  String rawVal = query.substring(kv + key.length()); // "%23ff00aa" or "#ff00aa"
  String val = urlDecode(rawVal);                      // "#ff00aa"

  uint32_t parsed;
  if (parseHexColor(val, parsed)) {
    currentColor = parsed;
    applyColor(currentColor);
    return true;
  }
  return false;
}

// --------- Arduino setup/loop ---------

void setup() {
  M5.begin(true, false, true);     // Initialize serial, button, LED
  delay(50);
  applyColor(currentColor);        // Show default LED color

  Serial.println("\nWIFI ACCESS POINT");
  Serial.printf("Please connect: %s\nThen open: http://", ssid);

  WiFi.softAP(ssid, password);     // Create AP (use without password for open AP)
  IPAddress myIP = WiFi.softAPIP();
  Serial.println(myIP);

  server.begin();
}

void loop() {
  WiFiClient client = server.available();
  if (!client) return;

  Serial.println("New Client:");
  String currentLine = "";
  String requestLine = ""; // store the very first line: "GET /... HTTP/1.1"

  // Read HTTP request
  while (client.connected()) {
    if (!client.available()) {
      delay(1);
      continue;
    }
    char c = client.read();
    Serial.write(c);

    if (c == '\n') {
      // Empty line -> end of headers
      if (currentLine.length() == 0) {
        // Handle any /set?value=... that may have come in the request line
        if (requestLine.startsWith("GET ")) {
          handleSetPath(requestLine);
        }
        // Always serve the HTML page after processing
        sendHtml(client);
        break;
      } else {
        // Capture the request line (first line only)
        if (requestLine.length() == 0) {
          requestLine = currentLine; // e.g., "GET /set?value=%23ff00aa HTTP/1.1"
        }
        currentLine = "";
      }
    } else if (c != '\r') {
      currentLine += c;
    }
  }

  client.stop();
}
