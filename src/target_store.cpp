#include "target_store.h"
#include <Preferences.h>
#include <Arduino.h>

extern Preferences prefs;

SavedTarget g_savedTargets[TARGET_MAX];
int g_savedTargetCount = 0;
static bool g_targetLoaded = false;

static void macToString(const uint8_t* m, char out[18]) {
  snprintf(out, 18, "%02X:%02X:%02X:%02X:%02X:%02X",
           m[0], m[1], m[2], m[3], m[4], m[5]);
}

static bool stringToMac(const char* s, uint8_t mac[6]) {
  int v[6];
  if (sscanf(s, "%x:%x:%x:%x:%x:%x", &v[0], &v[1], &v[2], &v[3], &v[4], &v[5]) != 6) return false;
  for (int i = 0; i < 6; i++) mac[i] = (uint8_t)v[i];
  return true;
}

void targetStoreLoad() {
  if (g_targetLoaded) return;
  g_targetLoaded = true;
  g_savedTargetCount = prefs.getInt("tgt_count", 0);
  if (g_savedTargetCount > TARGET_MAX) g_savedTargetCount = TARGET_MAX;
  for (int i = 0; i < g_savedTargetCount; i++) {
    String key = "tgt_" + String(i);
    String macS = prefs.getString((key + "_mac").c_str(), "");
    String name = prefs.getString((key + "_name").c_str(), "");
    uint8_t ch = (uint8_t)prefs.getInt((key + "_ch").c_str(), 1);
    SavedTarget& t = g_savedTargets[i];
    t.used = stringToMac(macS.c_str(), t.mac);
    strncpy(t.name, name.c_str(), 32);
    t.name[32] = 0;
    t.channel = (ch >= 1 && ch <= 14) ? ch : 1;
  }
}

void targetStoreSave() {
  prefs.putInt("tgt_count", g_savedTargetCount);
  for (int i = 0; i < g_savedTargetCount; i++) {
    String key = "tgt_" + String(i);
    char mac[18];
    macToString(g_savedTargets[i].mac, mac);
    prefs.putString((key + "_mac").c_str(), mac);
    prefs.putString((key + "_name").c_str(), g_savedTargets[i].name);
    prefs.putInt((key + "_ch").c_str(), g_savedTargets[i].channel);
  }
}

bool targetAdd(const char* name, const uint8_t mac[6], uint8_t channel) {
  targetStoreLoad();
  if (!name || name[0] == 0) return false;
  if (g_savedTargetCount >= TARGET_MAX) return false;
  SavedTarget& t = g_savedTargets[g_savedTargetCount];
  memcpy(t.mac, mac, 6);
  strncpy(t.name, name, 32);
  t.name[32] = 0;
  t.channel = (channel >= 1 && channel <= 14) ? channel : 1;
  t.used = true;
  g_savedTargetCount++;
  targetStoreSave();
  return true;
}

bool targetDelete(int idx) {
  targetStoreLoad();
  if (idx < 0 || idx >= g_savedTargetCount) return false;
  for (int i = idx; i < g_savedTargetCount - 1; i++) {
    g_savedTargets[i] = g_savedTargets[i + 1];
  }
  g_savedTargetCount--;
  targetStoreSave();
  return true;
}

bool targetFindByName(const char* name, SavedTarget* out) {
  targetStoreLoad();
  for (int i = 0; i < g_savedTargetCount; i++) {
    if (strcmp(g_savedTargets[i].name, name) == 0) {
      if (out) *out = g_savedTargets[i];
      return true;
    }
  }
  return false;
}

void targetList() {
  targetStoreLoad();
  Serial.printf("[TARGET] %d tersimpan\n", g_savedTargetCount);
  for (int i = 0; i < g_savedTargetCount; i++) {
    char mac[18];
    macToString(g_savedTargets[i].mac, mac);
    Serial.printf("  %2d  ch=%2d  %-16s  %s\n", i, g_savedTargets[i].channel, g_savedTargets[i].name, mac);
  }
}