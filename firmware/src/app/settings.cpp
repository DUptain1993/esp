#include "settings.h"

void Settings::begin() {
    _prefs.begin(_ns, false);
}

uint8_t Settings::getBrightness() {
    return _prefs.getUChar("br", 200);
}

void Settings::setBrightness(uint8_t val) {
    _prefs.putUChar("br", val);
}

bool Settings::getObfuscation() {
    return _prefs.getBool("ob", true);
}

void Settings::setObfuscation(bool enabled) {
    _prefs.putBool("ob", enabled);
}

uint8_t Settings::getLastPage() {
    return _prefs.getUChar("pg", 0);
}

void Settings::setLastPage(uint8_t page) {
    _prefs.putUChar("pg", page);
}

String Settings::getWifiSSID() {
    return _prefs.getString("w_ssid", "");
}

String Settings::getWifiPass() {
    return _prefs.getString("w_pass", "");
}

void Settings::setWifiCreds(String ssid, String pass) {
    _prefs.putString("w_ssid", ssid);
    _prefs.putString("w_pass", pass);
}
