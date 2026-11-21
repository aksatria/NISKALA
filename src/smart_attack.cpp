#include "smart_attack.h"
#include "ui_helpers.h"
#include "state_machine.h"
#include "ui_screens.h"
#include "attack_engine.h"
#include "wifi_state.h"
#include "recon.h"
#include "web_remote.h"
#include "templates.h"
#include <WiFi.h>

int g_smartRecommendation = 0;

static const char* authName(wifi_auth_mode_t m) {
  switch (m) {
    case WIFI_AUTH_OPEN: return "OPEN";
    case WIFI_AUTH_WEP: return "WEP";
    case WIFI_AUTH_WPA_PSK: return "WPA";
    case WIFI_AUTH_WPA2_PSK: return "WPA2";
    case WIFI_AUTH_WPA_WPA2_PSK: return "WPA/WPA2";
    case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2-ENT";
    case WIFI_AUTH_WPA3_PSK: return "WPA3";
    case WIFI_AUTH_WPA2_WPA3_PSK: return "WPA2/WPA3";
    default: return "?";
  }
}

void drawSmartProfile() {
  tft.fillScreen(TFT_BLACK);
  drawHeader("SMART PROFILE", TFT_PURPLE);

  int idx = selectedIdx;
  wifi_auth_mode_t auth = WiFi.encryptionType(idx);
  bool pmf = detectPMF(idx);
  const char* vendor = vendorFromMacBytes(WiFi.BSSID(idx));

  if (auth == WIFI_AUTH_OPEN) {
    g_smartRecommendation = 1; // Evil Twin (tanpa deauth perlu)
  } else if (pmf || auth == WIFI_AUTH_WPA3_PSK || auth == WIFI_AUTH_WPA2_WPA3_PSK) {
    g_smartRecommendation = 3; // CSA Attack
  } else {
    g_smartRecommendation = 1; // Evil Twin + Deauth
  }

  const char* rec = (g_smartRecommendation == 1) ? "EVIL TWIN" :
                    (g_smartRecommendation == 3) ? "CSA ATTACK" : "DEAUTH";

  tft.setTextFont(1);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setCursor(4, 28); tft.print(targetSSID.substring(0, 18));
  tft.setCursor(4, 42); tft.print("BSSID: ");
  tft.print(String(WiFi.BSSIDstr(idx)));
  tft.setCursor(4, 56); tft.print("CH:"); tft.print(WiFi.channel(idx));
  tft.print("  AUTH:"); tft.print(authName(auth));
  tft.setCursor(4, 70); tft.print("PMF:"); tft.print(pmf ? "YA" : "TIDAK");
  tft.setCursor(4, 84); tft.print("VENDOR:"); tft.print(vendor ? vendor : "-");
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setCursor(4, 100); tft.print("REKOMENDASI: "); tft.print(rec);

  drawFooter("ATAS=Batal | BAWAH=Jalankan");
}

void startSmartRecommendedAttack() {
  stopAllAttacks();
  if (g_smartRecommendation == 3) {
    currentState = CSA_RUNNING;
    startCSAAttack();
    drawCSARunning(0);
  } else {
    selectedTemplateIdx = 0;
    verifyAttempts = 0;
    verifyState = 0;
    currentState = TEMPLATE_SELECT;
    templateSelection = 0;
    drawTemplateSelect();
  }
}