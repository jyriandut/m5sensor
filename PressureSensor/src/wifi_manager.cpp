#include "wifi_manager.h"
#include "led_blinker.h"

namespace wifi_manager {
  void init_ap_wifi() {
    WiFi.mode(WIFI_MODE_APSTA);
    bool ok = WiFi.softAP(AP_WIFI_SSID, AP_WIFI_PASS);
    
    Serial.println("\nWIFI ACCESS POINT (fallback)");
    Serial.printf("SSID: %s  PASS: %s\n", AP_WIFI_SSID, AP_WIFI_PASS);
    Serial.printf("AP IP: %s\n", WiFi.softAPIP().toString().c_str());
  }
  
  void scan_wifi_networks(std::vector<NetworkData> &networks) {
    Serial.println("Scanning WiFi networks");

    int n = WiFi.scanComplete();
    if (n == -2) {
      WiFi.scanNetworks(true);
    } else if (n) {
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
        WiFi.scanDelete();
        if(WiFi.scanComplete() == -2){
          WiFi.scanNetworks(true);
        }
      }
    }
  }

  bool init_sta_wifi(const String ssid, const String password, LedBlinker& ledBlinker) {
    WiFi.mode(WIFI_MODE_STA);
    WiFi.setAutoReconnect(false);

    ledBlinker.set_blink(COLOR_BLUE, COLOR_BLACK, 1000);
    ledBlinker.tick();

    WiFi.begin(ssid, password);
    Serial.printf("Connecting to WiFi network %s\n", ssid.c_str());

    int totalAttempts = 20;
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts <= totalAttempts ) {
      Serial.printf("Connecting... %u; attempt nr %d of %d \n", WiFi.status(), attempts, totalAttempts);
      delay(1000);
      ledBlinker.tick();
      attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("Connected to WiFi network");
      ledBlinker.set_solid(COLOR_GREEN);
      return true;
    }
    Serial.println("Failed to connect, falling back to AP mode");
    return false;
  }
  
}

