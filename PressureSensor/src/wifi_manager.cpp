#include "wifi_manager.h"

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
    int n = WiFi.scanNetworks();
    Serial.println("Scan done");

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
    }
  }
}

