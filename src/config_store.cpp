#include "config_store.h"

static const int MAX_SAVED_PWDS = 50;

void savePasswordToNVS(String email, String pass) {
  int count = prefs.getInt("pwd_count", 0);
  if (count < MAX_SAVED_PWDS) {
    prefs.putString(("pwd_" + String(count)).c_str(), email + "|" + pass);
    prefs.putULong(("pwd_ts_" + String(count)).c_str(), millis());
    prefs.putInt("pwd_count", count + 1);
  }
}

void deletePasswordEntry(int idx) {
  int total = prefs.getInt("pwd_count", 0);
  if (idx < 0 || idx >= total) return;
  for (int i = idx; i < total - 1; i++) {
    prefs.putString(("pwd_" + String(i)).c_str(), prefs.getString(("pwd_" + String(i + 1)).c_str(), ""));
    prefs.putULong(("pwd_ts_" + String(i)).c_str(), prefs.getULong(("pwd_ts_" + String(i + 1)).c_str(), 0));
  }
  prefs.remove(("pwd_" + String(total - 1)).c_str());
  prefs.remove(("pwd_ts_" + String(total - 1)).c_str());
  prefs.putInt("pwd_count", total - 1);
}

void updatePasswordVerification(bool ok) {
  int count = prefs.getInt("pwd_count", 0);
  if (count <= 0) return;
  String key = "pwd_" + String(count - 1);
  String entry = prefs.getString(key.c_str(), "");
  int p1 = entry.indexOf('|');
  if (p1 < 0) return;
  String email = entry.substring(0, p1);
  String rest = entry.substring(p1 + 1);
  int p2 = rest.indexOf('|');
  String pass = (p2 > 0) ? rest.substring(0, p2) : rest;
  prefs.putString(key.c_str(), email + "|" + pass + "|" + (ok ? "1" : "0"));
}

void saveConfig() {
  prefs.putInt("cfg_deauth", cfg_deauth_burst);
  prefs.putInt("cfg_auth", cfg_auth_burst);
  prefs.putInt("cfg_hop", cfg_hop_interval_ms);
  prefs.putInt("cfg_beacon", cfg_beacon_hop_ms);
  prefs.putInt("cfg_reason", cfg_deauth_reason);
  prefs.putString("wifi_ssid", cfg_wifi_ssid);
  prefs.putString("wifi_pass", cfg_wifi_pass);
  prefs.putString("srv_url", cfg_server_url);
  prefs.putString("tpl_url", cfg_template_url);
  prefs.putInt("autosync", cfg_autosync_min);
}
bool applyAttackPreset(const String& preset) {
  String p = preset;
  p.toLowerCase();
  if (p == "aggressive") {
    cfg_deauth_burst = 20;
    cfg_auth_burst = 80;
    cfg_hop_interval_ms = 60;
    cfg_beacon_hop_ms = 50;
    cfg_deauth_reason = 0;
  } else if (p == "normal") {
    cfg_deauth_burst = 5;
    cfg_auth_burst = 20;
    cfg_hop_interval_ms = 120;
    cfg_beacon_hop_ms = 100;
    cfg_deauth_reason = 0;
  } else if (p == "stealth") {
    cfg_deauth_burst = 1;
    cfg_auth_burst = 5;
    cfg_hop_interval_ms = 300;
    cfg_beacon_hop_ms = 300;
    cfg_deauth_reason = 7;
  } else {
    return false;
  }
  saveConfig();
  return true;
}
