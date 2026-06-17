#pragma once
#include <Arduino.h>
#include <Preferences.h>

class Settings {
public:
    static Settings& getInstance() {
        static Settings instance;
        return instance;
    }

    void begin();
    
    // Brightness (0-255)
    uint8_t getBrightness();
    void setBrightness(uint8_t val);

    // Obfuscation (noise packets)
    bool getObfuscation();
    void setObfuscation(bool enabled);

    // Last used page index
    uint8_t getLastPage();
    void setLastPage(uint8_t page);

    // WiFi Credentials
    String getWifiSSID();
    String getWifiPass();
    void setWifiCreds(String ssid, String pass);

private:
    Settings() {}
    Preferences _prefs;
    const char* _ns = "cyberdeck";
};
