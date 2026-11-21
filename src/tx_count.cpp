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
