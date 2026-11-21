#ifndef TX_COUNT_H
#define TX_COUNT_H

#include <Arduino.h>
#include <esp_wifi.h>
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"

extern volatile uint32_t g_packetsSent;
extern volatile uint32_t g_txErrors;
extern portMUX_TYPE g_txMux;

inline void countTxResult(esp_err_t err) {
  g_packetsSent++;
  if (err != ESP_OK) g_txErrors++;
}

inline esp_err_t sendWifiFrameAndCount(const uint8_t* frame, int len, bool enSys) {
  esp_err_t err = esp_wifi_80211_tx(WIFI_IF_AP, frame, len, enSys);
  countTxResult(err);
  return err;
}

#endif