#include "client_monitor.h"
#include "ui_helpers.h"
#include "wifi_state.h"
#include "attack_engine.h"
#include "state_machine.h"
#include "web_remote.h"
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
    if (WiFi.getMode() != WIFI_AP_STA) radioSetMode(RADIO_AP_STA);
  } else {
    if (WiFi.getMode() != WIFI_AP_STA) radioSetMode(RADIO_AP_STA);
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
  setAttackMode(ATK_CLIENT_MONITOR);
  Serial.printf("[CLIENT] Monitor %02X:%02X:%02X:%02X:%02X:%02X ch=%d SSID=%s\n",
    g_targetBSSID[0], g_targetBSSID[1], g_targetBSSID[2],
    g_targetBSSID[3], g_targetBSSID[4], g_targetBSSID[5],
    g_targetChannel, targetSSID.c_str());
}

void stopClientMonitor() {
  isClientMonitorRunning = false;
  setAttackMode(ATK_IDLE);
  resetWifiStateAfterAttack();
  Serial.println("[CLIENT] Monitor berhenti");
}

