#ifndef NISKALA_WIFI_STATE_H
#define NISKALA_WIFI_STATE_H

#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include "radio_manager.h"

extern uint8_t g_originalApMac[6];

inline void restoreOriginalApMac() {
  if (g_originalApMac[0] != 0 || g_originalApMac[1] != 0 || g_originalApMac[2] != 0 ||
      g_originalApMac[3] != 0 || g_originalApMac[4] != 0 || g_originalApMac[5] != 0) {
    esp_wifi_set_mac(WIFI_IF_AP, g_originalApMac);
  }
}

inline void resetWifiStateAfterAttack() {
  esp_wifi_set_promiscuous(false);
  WiFi.softAPdisconnect(true);
  restoreOriginalApMac();
  radioSetMode(RADIO_STA);
}

#endif