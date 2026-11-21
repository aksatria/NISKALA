#include "allowlist.h"
#include <string.h>

extern Preferences prefs;

static void macToString(const uint8_t* mac, char* out) {
  snprintf(out, 13, "%02X%02X%02X%02X%02X%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

static bool stringToMac(const char* s, uint8_t* mac) {
  if (strlen(s) != 12) return false;
  for (int i = 0; i < 6; i++) {
    int hi = s[i * 2];
    int lo = s[i * 2 + 1];
    auto nib = [](char c) -> int {
      if (c >= '0' && c <= '9') return c - '0';
      if (c >= 'a' && c <= 'f') return c - 'a' + 10;
      if (c >= 'A' && c <= 'F') return c - 'A' + 10;
      return -1;
    };
    int h = nib(hi); int l = nib(lo);
    if (h < 0 || l < 0) return false;
    mac[i] = (uint8_t)((h << 4) | l);
  }
  return true;
}

int allowlistCount() {
  return prefs.getInt("allow_count", 0);
}

bool clientAllowed(const uint8_t* mac) {
  char target[13];
  macToString(mac, target);
  int n = allowlistCount();
  for (int i = 0; i < n; i++) {
    String s = prefs.getString(("allow_" + String(i)).c_str(), "");
    if (s.length() == 12 && strcasecmp(s.c_str(), target) == 0) return true;
  }
  return false;
}

bool allowlistAdd(const uint8_t* mac) {
  int n = allowlistCount();
  if (n >= 32) return false;
  char s[13];
  macToString(mac, s);
  prefs.putString(("allow_" + String(n)).c_str(), String(s));
  prefs.putInt("allow_count", n + 1);
  return true;
}

bool allowlistDelete(int idx) {
  int n = allowlistCount();
  if (idx < 0 || idx >= n) return false;
  for (int i = idx; i < n - 1; i++) {
    String next = prefs.getString(("allow_" + String(i + 1)).c_str(), "");
    prefs.putString(("allow_" + String(i)).c_str(), next);
  }
  prefs.remove(("allow_" + String(n - 1)).c_str());
  prefs.putInt("allow_count", n - 1);
  return true;
}

void allowlistPrint() {
  int n = allowlistCount();
  Serial.printf("[ALLOW] %d MAC\n", n);
  for (int i = 0; i < n; i++) {
    String s = prefs.getString(("allow_" + String(i)).c_str(), "");
    Serial.printf("  %d: %s\n", i, s.c_str());
  }
}