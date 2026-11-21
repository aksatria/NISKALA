#include "radio_manager.h"

RadioMode g_radioMode = RADIO_OFF;

void radioSetMode(RadioMode mode, uint8_t channel) {
  // Matikan promiscuous dulu agar transisi selalu bersih.
  esp_wifi_set_promiscuous(false);

  switch (mode) {
    case RADIO_OFF:
      WiFi.mode(WIFI_OFF);
      break;
    case RADIO_STA:
      WiFi.mode(WIFI_STA);
      break;
    case RADIO_AP:
      WiFi.mode(WIFI_AP);
      break;
    case RADIO_AP_STA:
      WiFi.mode(WIFI_AP_STA);
      break;
    case RADIO_PROMISC_STA:
      WiFi.mode(WIFI_STA);
      if (channel >= 1 && channel <= 14) {
        esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
      }
      esp_wifi_set_promiscuous(true);
      break;
  }

  g_radioMode = mode;
}

void radioSetPromiscuous(bool enable, uint8_t channel) {
  if (enable) {
    if (WiFi.getMode() == WIFI_OFF) WiFi.mode(WIFI_STA);
    if (channel >= 1 && channel <= 14) {
      esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
    }
    esp_wifi_set_promiscuous(true);
    g_radioMode = RADIO_PROMISC_STA;
  } else {
    esp_wifi_set_promiscuous(false);
    g_radioMode = (WiFi.getMode() == WIFI_AP_STA) ? RADIO_AP_STA : RADIO_STA;
  }
}