/*
 * ESP32 Overload and Stability Test for M5Stack Atom Lite (Concurrent Version)
 * 
 * Browser-based HTTP stress testing with concurrent requests
 * Tests LED color change API under increasing load to find:
 * - Maximum stable command frequency
 * - System behavior under concurrent load
 * - Critical failure points
 * - Soft recovery
 * 
 * Hardware: M5Stack Atom Lite (WS2812B RGB LED on GPIO 27)
 * 
 * Required Libraries: M5Atom, WiFi
 * 
 * Access Point: M5Stack_Test / 12345678
 * Default IP: 192.168.4.1
 * Test Interface: http://192.168.4.1/test
 * 
 * How to use:
 * 1. Upload this code to M5 Atom Lite
 * 2. Connect to WiFi: M5Stack_Test / 12345678
 * 3. Open browser: http://192.168.4.1/test
 * 4. Click "Start Test" and wait ~6 minutes
 * 5. View color-coded results and throttling recommendations
 */

#include <M5Atom.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>

// WiFi AP Configuration
const char* ap_ssid     = "M5Stack_Test";
const char* ap_password = "12345678";

WiFiServer server(80);

// Current LED color (0xRRGGBB)
uint32_t currentColor = 0x00FF00; // Start with green

// Server state
bool serverReady = false;


// --- Helper Functions ---

// Generate random RGB color
uint32_t randomColor() {
  uint8_t r = random(0, 256);
  uint8_t g = random(0, 256);
  uint8_t b = random(0, 256);
  return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

// Apply color to LED
void applyColor(uint32_t color) {
  uint8_t r = (color >> 16) & 0xFF;
  uint8_t g = (color >> 8) & 0xFF;
  uint8_t b = color & 0xFF;
  M5.dis.drawpix(0, CRGB(r, g, b));
}

// Convert color to hex string
String colorToHex(uint32_t color) {
  char buf[8];
  snprintf(buf, sizeof(buf), "%06X", color);
  return String(buf);
}

// Parse hex color string to uint32_t
bool parseHexColor(String hex, uint32_t& outColor) {
  if (hex.length() == 7 && hex[0] == '#') hex.remove(0, 1);
  if (hex.length() != 6) return false;
  char buf[7];
  hex.toCharArray(buf, 7);
  char* endptr = nullptr;
  uint32_t val = strtoul(buf, &endptr, 16);
  if (*endptr != '\0') return false;
  outColor = val & 0xFFFFFF;
  return true;
}

// --- HTTP Server Functions ---

void sendTestPage(WiFiClient& client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html; charset=utf-8");
  client.println("Connection: close");
  client.println();
  
  client.println("<!DOCTYPE html><html><head><meta charset='utf-8'>");
  client.println("<meta name='viewport' content='width=device-width, initial-scale=1'>");
  client.println("<title>M5 Overload Test</title>");
  client.println("<style>");
  client.println("body{font-family:system-ui,sans-serif;margin:2rem;background:#f5f5f5}");
  client.println(".container{max-width:900px;margin:0 auto;background:white;padding:2rem;border-radius:8px;box-shadow:0 2px 8px rgba(0,0,0,0.1)}");
  client.println("h1{color:#333;margin-top:0}");
  client.println("button{padding:12px 24px;font-size:16px;background:#0066cc;color:white;border:none;border-radius:6px;cursor:pointer;margin:10px 5px}");
  client.println("button:hover{background:#0052a3}");
  client.println("button:disabled{background:#ccc;cursor:not-allowed}");
  client.println("#status{padding:15px;margin:20px 0;border-radius:6px;font-weight:500}");
  client.println(".running{background:#fff3cd;color:#856404;border:1px solid #ffeaa7}");
  client.println(".complete{background:#d4edda;color:#155724;border:1px solid #c3e6cb}");
  client.println("table{width:100%;border-collapse:collapse;margin:20px 0}");
  client.println("th,td{padding:12px;text-align:left;border-bottom:1px solid #ddd}");
  client.println("th{background:#f8f9fa;font-weight:600}");
  client.println(".good{color:#28a745}");
  client.println(".warning{color:#ffc107}");
  client.println(".bad{color:#dc3545}");
  client.println(".progress{margin:10px 0;font-size:14px;color:#666}");
  client.println("</style></head><body>");
  client.println("<div class='container'>");
  client.println("<h1>M5 Atom Overload Test</h1>");
  client.println("<p>This test will send HTTP requests at increasing frequencies to measure server performance.</p>");
  client.println("<h3>Test Criteria</h3>");
  client.println("<table style='font-size:14px'>");
  client.println("<tr><th>Color</th><th>Criteria</th><th>Meaning</th></tr>");
  client.println("<tr class='good'><td>🟢 Green</td><td>Success ≥95% AND Avg &lt;200ms</td><td><strong>Stable</strong> - Safe for production</td></tr>");
  client.println("<tr class='warning'><td>🟡 Yellow</td><td>Success ≥80% AND Avg &lt;500ms</td><td><strong>Warning</strong> - Degraded performance</td></tr>");
  client.println("<tr class='bad'><td>🔴 Red</td><td>Success &lt;80% OR Avg ≥500ms</td><td><strong>Failure</strong> - System overloaded</td></tr>");
  client.println("</table>");
  client.println("<button id='startBtn' onclick='startTest()'>Start Test</button>");
  client.println("<button id='stopBtn' onclick='stopTest()' disabled style='background:#dc3545'>Stop Test</button>");
  client.println("<button onclick='location.reload()'>Reset</button>");
  client.println("<div id='status'></div>");
  client.println("<div id='progress'></div>");
  client.println("<div id='results'></div>");
  client.println("<script>");
  
  // JavaScript test code
  client.println("const frequencies=[10,50,100,200,300,400,500,600,700,800,900,1000];");
  client.println("const testDuration=30000;");
  client.println("let results=[];");
  client.println("let currentTest=0;");
  client.println("let isRunning=false;");
  client.println("let shouldStop=false;");
  
  client.println("function randomColor(){");
  client.println("const r=Math.floor(Math.random()*256).toString(16).padStart(2,'0');");
  client.println("const g=Math.floor(Math.random()*256).toString(16).padStart(2,'0');");
  client.println("const b=Math.floor(Math.random()*256).toString(16).padStart(2,'0');");
  client.println("return r+g+b;}");
  
  client.println("async function sendSetRequest(color){");
  client.println("const start=performance.now();");
  client.println("try{");
  client.println("const response=await fetch(`/set?color=%23${color}`);");
  client.println("const end=performance.now();");
  client.println("return{success:response.ok,time:end-start};");
  client.println("}catch(e){");
  client.println("return{success:false,time:performance.now()-start};}}");
  
  client.println("async function testRecovery(){");
  client.println("document.getElementById('progress').innerHTML='<div class=\"progress\">Testing recovery...</div>';");
  client.println("await new Promise(r=>setTimeout(r,10000));");
  client.println("const recoveryStart=Date.now();");
  client.println("let recovered=false;");
  client.println("for(let i=0;i<10;i++){");
  client.println("const result=await sendSetRequest(randomColor());");
  client.println("if(result.success&&result.time<100){recovered=true;break;}");
  client.println("await new Promise(r=>setTimeout(r,1000));}");
  client.println("const recoveryTime=Date.now()-recoveryStart;");
  client.println("return{recovered,recoveryTime};}");
  
  client.println("async function runFrequencyTest(freq){");
  client.println("if(shouldStop)return null;");
  client.println("document.getElementById('status').className='running';");
  client.println("document.getElementById('status').textContent=`Testing ${freq} cmd/s...`;");
  client.println("let total=0,success=0,failed=0,totalTime=0,maxTime=0,minTime=999999;");
  client.println("const startTime=Date.now();");
  client.println("const promises=[];");
  client.println("const targetRequests=freq*testDuration/1000;");
  client.println("for(let i=0;i<targetRequests;i++){");
  client.println("if(shouldStop)break;");
  client.println("const color=randomColor();");
  client.println("const promise=sendSetRequest(color).then(result=>{");
  client.println("total++;");
  client.println("if(result.success){success++;totalTime+=result.time;");
  client.println("maxTime=Math.max(maxTime,result.time);");
  client.println("minTime=Math.min(minTime,result.time);}else{failed++;}");
  client.println("if(total%50===0){");
  client.println("document.getElementById('progress').innerHTML=");
  client.println("`<div class='progress'>Progress: ${total}/${targetRequests} requests...</div>`;}");
  client.println("});");
  client.println("promises.push(promise);");
  client.println("if(promises.length>=freq){await Promise.race(promises);}}");
  client.println("await Promise.all(promises);");
  client.println("return{freq,total,success,failed,");
  client.println("avgTime:success>0?totalTime/success:0,");
  client.println("maxTime:maxTime<999999?maxTime:0,");
  client.println("minTime:minTime<999999?minTime:0,recoveryTime:null,recovered:null};}");
  
  client.println("function stopTest(){");
  client.println("shouldStop=true;");
  client.println("document.getElementById('stopBtn').disabled=true;");
  client.println("document.getElementById('status').className='complete';");
  client.println("document.getElementById('status').textContent='⚠️ Test Stopped by User';");
  client.println("displayResults();}");
  
  client.println("async function startTest(){");
  client.println("if(isRunning)return;");
  client.println("isRunning=true;shouldStop=false;");
  client.println("document.getElementById('startBtn').disabled=true;");
  client.println("document.getElementById('stopBtn').disabled=false;");
  client.println("results=[];");
  client.println("for(let i=0;i<frequencies.length;i++){");
  client.println("if(shouldStop)break;");
  client.println("const result=await runFrequencyTest(frequencies[i]);");
  client.println("if(!result)break;");
  client.println("results.push(result);");
  client.println("const successRate=(result.success/result.total*100);");
  client.println("if(successRate<95||result.avgTime>200){");
  client.println("const recovery=await testRecovery();");
  client.println("result.recoveryTime=recovery.recoveryTime;");
  client.println("result.recovered=recovery.recovered;");
  client.println("if(!recovery.recovered){");
  client.println("document.getElementById('status').className='complete';");
  client.println("document.getElementById('status').textContent='⚠️ System did not recover - stopping test';");
  client.println("break;}}");
  client.println("displayResults();}");
  client.println("if(!shouldStop){");
  client.println("document.getElementById('status').className='complete';");
  client.println("document.getElementById('status').textContent='✓ Test Complete!';}");
  client.println("document.getElementById('progress').innerHTML='';");
  client.println("document.getElementById('stopBtn').disabled=true;");
  client.println("isRunning=false;}");
  
  client.println("function displayResults(){");
  client.println("let html='<h2>Results</h2><table><tr>';");
  client.println("html+='<th>Freq (cmd/s)</th><th>Total</th><th>Success</th><th>Failed</th>';");
  client.println("html+='<th>Success %</th><th>Avg (ms)</th><th>Max (ms)</th><th>Recovery</th></tr>';");
  client.println("let maxStable=0;");
  client.println("results.forEach(r=>{");
  client.println("const successRate=(r.success/r.total*100).toFixed(1);");
  client.println("let rowClass='';");
  client.println("if(successRate>=95&&r.avgTime<200)rowClass='good';");
  client.println("else if(successRate>=80&&r.avgTime<500)rowClass='warning';");
  client.println("else rowClass='bad';");
  client.println("if(successRate>=95&&r.avgTime<200)maxStable=r.freq;");
  client.println("html+=`<tr class='${rowClass}'><td>${r.freq}</td><td>${r.total}</td>`;");
  client.println("html+=`<td>${r.success}</td><td>${r.failed}</td>`;");
  client.println("html+=`<td>${successRate}%</td>`;");
  client.println("html+=`<td>${r.avgTime.toFixed(1)}</td>`;");
  client.println("html+=`<td>${r.maxTime.toFixed(1)}</td>`;");
  client.println("if(r.recoveryTime!==null){");
  client.println("const recTime=(r.recoveryTime/1000).toFixed(1);");
  client.println("html+=`<td>${r.recovered?'✅ '+recTime+'s':'❌ Failed'}</td>`;}");
  client.println("else{html+=`<td>-</td>`;}");
  client.println("html+=`</tr>`;});");
  client.println("html+='</table>';");
  client.println("html+=`<h3>Recommendation</h3>`;");
  client.println("html+=`<p><strong>Maximum Stable Frequency:</strong> ${maxStable} cmd/s</p>`;");
  client.println("html+=`<p>Set your web interface throttling to <strong>${maxStable} commands/second</strong> maximum.</p>`;");
  client.println("document.getElementById('results').innerHTML=html;}");
  
  client.println("</script></div></body></html>");
}

void handleClient() {
  WiFiClient client = server.available();
  if (!client) return;

  String currentLine = "", requestLine = "";
  
  while (client.connected()) {
    if (!client.available()) {
      delay(1);
      continue;
    }
    
    char c = client.read();
    
    if (c == '\n') {
      if (currentLine.length() == 0) {
        // Headers ended, process request
        if (requestLine.startsWith("GET ")) {
          
          // Handle /get endpoint - return current color
          if (requestLine.indexOf("GET /get") >= 0) {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: application/json");
            client.println("Connection: close");
            client.println();
            client.print("{\"color\":\"#");
            client.print(colorToHex(currentColor));
            client.println("\"}");
            break;
          }
          
          // Handle /set endpoint - set color
          else if (requestLine.indexOf("GET /set?color=") >= 0) {
            int colorStart = requestLine.indexOf("color=") + 6;
            int colorEnd = requestLine.indexOf(' ', colorStart);
            if (colorEnd < 0) colorEnd = requestLine.indexOf('&', colorStart);
            if (colorEnd < 0) colorEnd = requestLine.length();
            
            String colorStr = requestLine.substring(colorStart, colorEnd);
            // URL decode %23 to #
            colorStr.replace("%23", "#");
            
            uint32_t newColor;
            if (parseHexColor(colorStr, newColor)) {
              currentColor = newColor;
              applyColor(currentColor);
              
              client.println("HTTP/1.1 200 OK");
              client.println("Content-Type: application/json");
              client.println("Connection: close");
              client.println();
              client.println("{\"status\":\"ok\"}");
            } else {
              client.println("HTTP/1.1 400 Bad Request");
              client.println("Connection: close");
              client.println();
            }
            break;
          }
          
          // Handle /test endpoint - browser-based test page
          else if (requestLine.indexOf("GET /test") >= 0) {
            sendTestPage(client);
            break;
          }
          
          // Default response
          else {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: text/html");
            client.println("Connection: close");
            client.println();
            client.println("<!DOCTYPE html><html><head>");
            client.println("<style>body{font-family:sans-serif;margin:2rem}");
            client.println("a{display:inline-block;padding:12px 24px;background:#0066cc;color:white;");
            client.println("text-decoration:none;border-radius:6px;margin:10px 0}");
            client.println("a:hover{background:#0052a3}</style></head><body>");
            client.println("<h1>M5 Atom Overload Test</h1>");
            client.println("<h2>Start Browser Test</h2>");
            client.println("<a href='/test'>Launch Overload Test</a>");
            client.println("<h2>API Endpoints:</h2>");
            client.println("<ul>");
            client.println("<li>GET /get - Get current color</li>");
            client.println("<li>GET /set?color=%23RRGGBB - Set color</li>");
            client.println("<li>GET /test - Browser-based test interface</li>");
            client.println("</ul>");
            client.print("<p>Current color: #");
            client.print(colorToHex(currentColor));
            client.println("</p>");
            client.println("</body></html>");
            break;
          }
        }
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

// --- Setup and Loop ---

void setup() {
  M5.begin(true, false, true);
  delay(50);
  
  Serial.begin(115200);
  delay(1000);
  
  // Apply initial color
  applyColor(currentColor);
  
  // Print welcome message
  Serial.println("\n\n╔════════════════════════════════════════════════════════════╗");
  Serial.println("║      ESP32 OVERLOAD AND STABILITY TEST                     ║");
  Serial.println("║      M5Stack Atom Lite                                     ║");
  Serial.println("╚════════════════════════════════════════════════════════════╝");
  Serial.println();
  Serial.println("Setting up WiFi Access Point...");
  
  // Start AP
  WiFi.softAP(ap_ssid, ap_password);
  IPAddress myIP = WiFi.softAPIP();
  
  Serial.print("AP SSID: ");
  Serial.println(ap_ssid);
  Serial.print("AP Password: ");
  Serial.println(ap_password);
  Serial.print("IP Address: ");
  Serial.println(myIP);
  Serial.println();
  
  // Start server
  server.begin();
  serverReady = true;
  
  Serial.println("✓ Server ready!");
  Serial.println();
  Serial.println("═══════════════════════════════════════════════════════════");
  Serial.println("Open browser: http://192.168.4.1/test");
  Serial.println("═══════════════════════════════════════════════════════════");
  Serial.println();
  
  // LED blinks green to indicate ready
  for (int i = 0; i < 3; i++) {
    M5.dis.drawpix(0, CRGB::Green);
    delay(200);
    M5.dis.drawpix(0, CRGB::Black);
    delay(200);
  }
  applyColor(currentColor);
}

void loop() {
  M5.update();
  
  // Handle server requests
  handleClient();
  
  delay(10);
}