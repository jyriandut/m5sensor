#include "serial.h"
#include "storage.h"
#include "utils.h"
#include "wifi_manager.h"
#include <vector>

namespace serial {
  bool init_ap(LedBlinker &led_blinker) {
    std::vector<wifi_manager::NetworkData> networks;
    Serial.println("Please wait. Scanning for WiFi networks...");
    wifi_manager::scan_wifi_networks(networks);
    delay(1000);
    if (networks.empty()) {
      Serial.println("Waiting for wifi networks...");
      return false;
    }
    
    Serial.printf("Found %u networks.\n", (unsigned)networks.size());
    Serial.println("Found networks:");
    for (auto n : networks) {
      Serial.printf("SSID: %s \n", n.ssid.c_str());
    }

    Serial.println("Please select SSID: ");
    auto ssidEntered = false;
    String ssid;
    while(!ssidEntered) {
      ssid = Serial.readStringUntil('\n');
      if (!ssid.isEmpty()) {
        ssidEntered = true;
      }
    }
    
    Serial.printf("SSID: %s", ssid.c_str());
    Serial.println("Please insert password: ");
    auto passEntered = false;
    String pass;
    while(!passEntered) {
      pass = Serial.readStringUntil('\n');
      if (!pass.isEmpty()) {
        passEntered = true;
      }
    }
    Serial.printf("Pass: %s", pass.c_str());
    storage::WifiCredentials wifiCreds;
    storage::save_wifi_credentials(wifiCreds);
    return true;
  }
} // namespace serial
