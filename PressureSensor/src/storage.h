#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include "config.h"


namespace storage {

  struct WifiCredentials {
    String ssid;
    String pass;
    String token; 
  };
  
  bool save_wifi_credentials(const WifiCredentials& cfg);
  bool clear_wifi_credentials();
  bool has_wifi_credentials();
  bool load_wifi_credentials(WifiCredentials& cfg);
}
