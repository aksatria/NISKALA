#ifndef NISKALA_RADIO_MANAGER_H
#define NISKALA_RADIO_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>

enum RadioMode {
  RADIO_OFF = 0,
  RADIO_STA,
  RADIO_AP,
  RADIO_AP_STA,
  RADIO_PROMISC_STA
};

extern RadioMode g_radioMode;

void radioSetMode(RadioMode mode, uint8_t channel = 0);
void radioSetPromiscuous(bool enable, uint8_t channel = 0);

#endif