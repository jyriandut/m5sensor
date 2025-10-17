/*
 * ESP32 Overload and Stability Test for M5Stack Atom Lite
 * 
 * Self-contained test that runs both web server and test client
 * Tests LED color change API under increasing load to find:
 * - Maximum stable command frequency
 * - Graceful degradation limits
 * - Critical failure points
 * - Recovery time after overload
 * 
 * Hardware: M5Stack Atom Lite (WS2812B RGB LED on GPIO 27)
 * Serial Monitor: 115200 baud
 * 
 * Required Libraries: M5Atom, WiFi, HTTPClient
 * 
 * Access Point: M5Stack_Test / 12345678
 * Default IP: 192.168.4.1
 * 
 * Press the button to start the test sequence
 */

#include <M5Atom.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>
#include <HTTPClient.h>

// WiFi AP Configuration
const char* ap_ssid     = "M5Stack_Test";
const char* ap_password = "12345678";
const char* serverIP    = "127.0.0.1"; // Use localhost for self-testing

WiFiServer server(80);

// Current LED color (0xRRGGBB)
uint32_t currentColor = 0x00FF00; // Start with green
uint32_t lastSetColor = 0x00FF00;

// Test configuration
const int testFrequencies[] = {10, 50, 100, 200, 300, 400, 500, 600, 700, 800, 900, 1000};
const int numFrequencies = 12;
const int testDuration = 30000; // 30 seconds per frequency level

// Test state
bool testRunning = false;
bool serverReady = false;

// Test results structure
struct TestResults {
  int frequency;
  int totalRequests;
  int successfulRequests;
  int failedRequests;
  int droppedCommands;
  float avgResponseTime;
  float maxResponseTime;
  float minResponseTime;
};

TestResults results[12]; // Store results for each frequency
int currentTestIndex = 0;

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
  client.println("<button id='startBtn' onclick='startTest()'>Start Test</button>");
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
  
  client.println("async function runFrequencyTest(freq){");
  client.println("document.getElementById('status').className='running';");
  client.println("document.getElementById('status').textContent=`Testing ${freq} cmd/s...`;");
  client.println("const delay=1000/freq;");
  client.println("let total=0,success=0,failed=0,totalTime=0,maxTime=0,minTime=999999;");
  client.println("const startTime=Date.now();");
  client.println("while(Date.now()-startTime<testDuration){");
  client.println("const color=randomColor();");
  client.println("const result=await sendSetRequest(color);");
  client.println("total++;");
  client.println("if(result.success){success++;totalTime+=result.time;");
  client.println("maxTime=Math.max(maxTime,result.time);");
  client.println("minTime=Math.min(minTime,result.time);}else{failed++;}");
  client.println("if(total%10===0){");
  client.println("document.getElementById('progress').innerHTML=");
  client.println("`<div class='progress'>Progress: ${total} requests sent...</div>`;}");
  client.println("await new Promise(r=>setTimeout(r,delay));}");
  client.println("return{freq,total,success,failed,");
  client.println("avgTime:success>0?totalTime/success:0,");
  client.println("maxTime:maxTime<999999?maxTime:0,");
  client.println("minTime:minTime<999999?minTime:0};}");
  
  client.println("async function startTest(){");
  client.println("if(isRunning)return;");
  client.println("isRunning=true;");
  client.println("document.getElementById('startBtn').disabled=true;");
  client.println("results=[];");
  client.println("for(let i=0;i<frequencies.length;i++){");
  client.println("const result=await runFrequencyTest(frequencies[i]);");
  client.println("results.push(result);");
  client.println("displayResults();}");
  client.println("document.getElementById('status').className='complete';");
  client.println("document.getElementById('status').textContent='✓ Test Complete!';");
  client.println("document.getElementById('progress').innerHTML='';");
  client.println("isRunning=false;}");
  
  client.println("function displayResults(){");
  client.println("let html='<h2>Results</h2><table><tr>';");
  client.println("html+='<th>Freq (cmd/s)</th><th>Total</th><th>Success</th><th>Failed</th>';");
  client.println("html+='<th>Success %</th><th>Avg (ms)</th><th>Max (ms)</th></tr>';");
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
  client.println("html+=`<td>${r.maxTime.toFixed(1)}</td></tr>`;});");
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

// --- HTTP Client Test Functions ---

// Send SET request and measure response time
bool sendSetRequest(uint32_t color, float& responseTime) {
  HTTPClient http;
  String url = "http://" + String(serverIP) + "/set?color=%23" + colorToHex(color);
  
  unsigned long startTime = millis();
  http.begin(url);
  http.setTimeout(5000); // 5 second timeout
  
  int httpCode = http.GET();
  unsigned long endTime = millis();
  responseTime = endTime - startTime;
  
  http.end();
  
  return (httpCode == 200);
}

// Send GET request to verify current color
bool sendGetRequest(uint32_t& retrievedColor) {
  HTTPClient http;
  String url = "http://" + String(serverIP) + "/get";
  
  http.begin(url);
  http.setTimeout(5000);
  
  int httpCode = http.GET();
  
  if (httpCode == 200) {
    String payload = http.getString();
    // Parse JSON: {"color":"#RRGGBB"}
    int colorStart = payload.indexOf("#") + 1;
    if (colorStart > 0) {
      String colorStr = payload.substring(colorStart, colorStart + 6);
      http.end();
      return parseHexColor(colorStr, retrievedColor);
    }
  }
  
  http.end();
  return false;
}

// Run test at specific frequency
void runFrequencyTest(int frequency, int duration) {
  Serial.println("\n=================================");
  Serial.print("Testing at ");
  Serial.print(frequency);
  Serial.println(" commands/second");
  Serial.print("Duration: ");
  Serial.print(duration / 1000);
  Serial.println(" seconds");
  Serial.println("=================================");
  
  int totalRequests = 0;
  int successfulRequests = 0;
  int failedRequests = 0;
  float totalResponseTime = 0;
  float maxResponse = 0;
  float minResponse = 999999;
  
  unsigned long delayBetweenRequests = 1000 / frequency; // milliseconds
  unsigned long startTime = millis();
  unsigned long lastRequestTime = 0;
  
  while (millis() - startTime < duration) {
    // Handle server requests
    handleClient();
    
    // Check if it's time to send next request
    if (millis() - lastRequestTime >= delayBetweenRequests) {
      uint32_t testColor = randomColor();
      float responseTime = 0;
      
      bool success = sendSetRequest(testColor, responseTime);
      
      totalRequests++;
      if (success) {
        successfulRequests++;
        totalResponseTime += responseTime;
        if (responseTime > maxResponse) maxResponse = responseTime;
        if (responseTime < minResponse) minResponse = responseTime;
        lastSetColor = testColor;
        
        // Print progress every 10 requests
        if (totalRequests % 10 == 0) {
          Serial.print(".");
        }
      } else {
        failedRequests++;
        Serial.print("X");
      }
      
      lastRequestTime = millis();
    }
    
    delay(1); // Small delay to prevent watchdog issues
  }
  
  Serial.println("\n");
  
  // Verify with GET request
  delay(500); // Wait a bit before verification
  uint32_t retrievedColor = 0;
  bool getSuccess = sendGetRequest(retrievedColor);
  int droppedCommands = 0;
  
  if (getSuccess) {
    if (retrievedColor != lastSetColor) {
      droppedCommands = 1;
      Serial.println("⚠ WARNING: Last SET color doesn't match GET color!");
      Serial.print("  Expected: #");
      Serial.println(colorToHex(lastSetColor));
      Serial.print("  Got:      #");
      Serial.println(colorToHex(retrievedColor));
    } else {
      Serial.println("✓ GET verification successful - no commands dropped");
    }
  } else {
    Serial.println("✗ GET request failed!");
  }
  
  // Store results
  results[currentTestIndex].frequency = frequency;
  results[currentTestIndex].totalRequests = totalRequests;
  results[currentTestIndex].successfulRequests = successfulRequests;
  results[currentTestIndex].failedRequests = failedRequests;
  results[currentTestIndex].droppedCommands = droppedCommands;
  results[currentTestIndex].avgResponseTime = (successfulRequests > 0) ? (totalResponseTime / successfulRequests) : 0;
  results[currentTestIndex].maxResponseTime = maxResponse;
  results[currentTestIndex].minResponseTime = (minResponse < 999999) ? minResponse : 0;
  
  // Print summary
  Serial.println("\n--- Test Summary ---");
  Serial.print("Total requests: ");
  Serial.println(totalRequests);
  Serial.print("Successful: ");
  Serial.print(successfulRequests);
  Serial.print(" (");
  Serial.print((successfulRequests * 100.0) / totalRequests);
  Serial.println("%)");
  Serial.print("Failed: ");
  Serial.println(failedRequests);
  Serial.print("Avg response time: ");
  Serial.print(results[currentTestIndex].avgResponseTime);
  Serial.println(" ms");
  Serial.print("Min response time: ");
  Serial.print(results[currentTestIndex].minResponseTime);
  Serial.println(" ms");
  Serial.print("Max response time: ");
  Serial.print(results[currentTestIndex].maxResponseTime);
  Serial.println(" ms");
  Serial.println("=================================\n");
  
  currentTestIndex++;
}

// Display final results
void displayFinalResults() {
  Serial.println("\n\n");
  Serial.println("╔════════════════════════════════════════════════════════════╗");
  Serial.println("║           OVERLOAD TEST - FINAL RESULTS                    ║");
  Serial.println("╚════════════════════════════════════════════════════════════╝");
  Serial.println();
  
  Serial.println("Freq  | Total | Success | Failed | Dropped | Avg(ms) | Max(ms)");
  Serial.println("------|-------|---------|--------|---------|---------|--------");
  
  int maxStableFreq = 0;
  int gracefulDegradationFreq = 0;
  int criticalFailureFreq = 0;
  
  for (int i = 0; i < currentTestIndex; i++) {
    Serial.printf("%4d  | %5d | %7d | %6d | %7d | %7.1f | %7.1f\n",
                  results[i].frequency,
                  results[i].totalRequests,
                  results[i].successfulRequests,
                  results[i].failedRequests,
                  results[i].droppedCommands,
                  results[i].avgResponseTime,
                  results[i].maxResponseTime);
    
    // Determine thresholds
    float successRate = (results[i].successfulRequests * 100.0) / results[i].totalRequests;
    
    if (successRate >= 95 && results[i].avgResponseTime < 200 && results[i].droppedCommands == 0) {
      maxStableFreq = results[i].frequency;
    }
    
    if (successRate >= 80 && results[i].avgResponseTime < 500 && gracefulDegradationFreq == 0) {
      gracefulDegradationFreq = results[i].frequency;
    }
    
    if ((successRate < 50 || results[i].droppedCommands > 0) && criticalFailureFreq == 0) {
      criticalFailureFreq = results[i].frequency;
    }
  }
  
  Serial.println();
  Serial.println("═══════════════════════════════════════════════════════════");
  Serial.println("                    RECOMMENDATIONS                        ");
  Serial.println("═══════════════════════════════════════════════════════════");
  Serial.print("✓ Maximum Stable Frequency: ");
  Serial.print(maxStableFreq);
  Serial.println(" cmd/s");
  Serial.println("  (>95% success, <200ms avg, no drops)");
  Serial.println();
  
  if (gracefulDegradationFreq > maxStableFreq) {
    Serial.print("⚠ Graceful Degradation Starts: ");
    Serial.print(gracefulDegradationFreq);
    Serial.println(" cmd/s");
    Serial.println("  (>80% success, <500ms avg)");
    Serial.println();
  }
  
  if (criticalFailureFreq > 0) {
    Serial.print("✗ Critical Failure Point: ");
    Serial.print(criticalFailureFreq);
    Serial.println(" cmd/s");
    Serial.println("  (<50% success or commands dropped)");
    Serial.println();
  }
  
  Serial.println("═══════════════════════════════════════════════════════════");
  Serial.println("THROTTLING RECOMMENDATION FOR WEB INTERFACE:");
  Serial.print("Set maximum rate to ");
  Serial.print(maxStableFreq);
  Serial.println(" commands/second");
  Serial.println("═══════════════════════════════════════════════════════════");
  Serial.println();
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
  Serial.println("Press the button to start the overload test");
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
  
  // Handle server requests when not testing
  if (!testRunning) {
    handleClient();
    
    // Check for button press to start test
    if (M5.Btn.wasPressed()) {
      testRunning = true;
      currentTestIndex = 0;
      
      Serial.println("\n\n");
      Serial.println("╔════════════════════════════════════════════════════════════╗");
      Serial.println("║              STARTING OVERLOAD TEST                        ║");
      Serial.println("╚════════════════════════════════════════════════════════════╝");
      Serial.println();
      Serial.println("This will test the following frequencies:");
      for (int i = 0; i < numFrequencies; i++) {
        Serial.print("  ");
        Serial.print(testFrequencies[i]);
        Serial.println(" cmd/s");
      }
      Serial.println();
      delay(2000);
      
      // Run all frequency tests
      for (int i = 0; i < numFrequencies; i++) {
        runFrequencyTest(testFrequencies[i], testDuration);
        delay(2000); // Pause between tests
      }
      
      // Display final results
      displayFinalResults();
      
      testRunning = false;
      Serial.println("\n✓ Test complete! Press button to run again.");
      
      // Blink LED to indicate completion
      for (int i = 0; i < 5; i++) {
        M5.dis.drawpix(0, CRGB::Blue);
        delay(200);
        M5.dis.drawpix(0, CRGB::Black);
        delay(200);
      }
      applyColor(currentColor);
    }
  }
  
  delay(10);
}