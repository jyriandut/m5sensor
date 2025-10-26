#include "storage.h"

namespace storage {
  bool save_wifi_credentials(const WifiCredentials &cfg) {
    Preferences prefs;
    if (!prefs.begin(NET_CFG, false)) {
      Serial.printf("ERROR: Failed to open Preferences " NET_CFG);
      return false;
    }

    prefs.putString("ssid",  cfg.ssid);
    prefs.putString("pass",  cfg.pass);
    prefs.putString("token", cfg.token);
    prefs.end();
    return true; 
  }

  bool clear_wifi_credentials() {
    Serial.printf("Deleting preferences: " NET_CFG);
    Preferences prefs;
    if (!prefs.begin(NET_CFG, false)) {
      Serial.printf("ERROR: Couldn't load Preferences: " NET_CFG);
      return false;
    }
    bool cleared = prefs.clear();
    prefs.end();
    return cleared;
  }

  bool has_wifi_credentials() {
    Preferences prefs;
    bool result = true;
    if (!prefs.begin(NET_CFG, true)) {
      Serial.printf("ERROR: Couldn't load Preferences: " NET_CFG);
      result = false;
    }
    if (!prefs.isKey("ssid")) {
      Serial.println("ssid key not found in preferences");
      result = false;
    }
    else if (prefs.getString("ssid").equals(String())) {
      Serial.println("ssid key value empty");
      result = false;
    }
    prefs.end();
    return result;
  }

  bool load_wifi_credentials(WifiCredentials &cfg) {
    Preferences prefs;

    if (!prefs.begin(NET_CFG, false)) {
      Serial.printf("ERROR: Couldn't load Preferences: " NET_CFG);
      return false;
    }

    cfg.ssid = prefs.getString("ssid", "");
    Serial.printf("prefs.getString ssid %s \n", cfg.ssid.c_str());
    
    cfg.pass  = prefs.getString("pass", "");
    cfg.token = prefs.getString("token", "");
    prefs.end();
    return true;
  }
  
}
