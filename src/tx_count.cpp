#include "tx_count.h"

void countTxResult(esp_err_t err) {
  portENTER_CRITICAL(&g_txMux);
  g_packetsSent++;
  if (err != ESP_OK) g_txErrors++;
  portEXIT_CRITICAL(&g_txMux);
}

esp_err_t sendWifiFrameAndCount(const uint8_t* frame, int len, bool enSys) {
  esp_err_t err = esp_wifi_80211_tx(WIFI_IF_AP, frame, len, enSys);
  countTxResult(err);
  return err;
}

uint32_t getPacketsSent() {
  uint32_t v;
  portENTER_CRITICAL(&g_txMux);
  v = g_packetsSent;
  portEXIT_CRITICAL(&g_txMux);
  return v;
}

uint32_t getTxErrors() {
  uint32_t v;
  portENTER_CRITICAL(&g_txMux);
  v = g_txErrors;
  portEXIT_CRITICAL(&g_txMux);
  return v;
}

void resetPacketCounters() {
  portENTER_CRITICAL(&g_txMux);
  g_packetsSent = 0;
  g_txErrors = 0;
  portEXIT_CRITICAL(&g_txMux);
}
