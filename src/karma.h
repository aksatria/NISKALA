#ifndef KARMA_H
#define KARMA_H

#include <Arduino.h>
#include <esp_wifi.h>
#include <string.h>
#include "tx_count.h"

#define KARMA_MAX_SSIDS    16
#define KARMA_MAX_CLIENTS  16
#define KARMA_SCAN_DURATION_MS  20000

struct KarmaSSIDEntry {
  char ssid[33];
  uint16_t probeCount;
  uint16_t authCount;
  bool used;
};

struct KarmaClientEntry {
  uint8_t mac[6];
  int8_t lastRSSI;
  uint16_t probeCount;
  unsigned long lastSeen;
  bool used;
};

// Didefinisikan di main.cpp
extern KarmaSSIDEntry g_karmaSSIDs[KARMA_MAX_SSIDS];
extern int g_karmaSSIDCount;
extern KarmaClientEntry g_karmaClients[KARMA_MAX_CLIENTS];
extern int g_karmaClientCount;
extern volatile uint32_t g_karmaResponses;
extern portMUX_TYPE g_karmaMux;
extern volatile bool g_karmaActiveMode;
extern volatile uint32_t g_packetsSent;
extern volatile uint32_t g_txErrors;

// Curated SSID list untuk broadcast probe (didefinisikan di karma.cpp)
extern const char* CURATED_SSIDS[];
extern const int CURATED_COUNT;

void karmaLogSSID(const char* ssid);
void karmaLogClient(const uint8_t* mac, int8_t rssi);
void sendKarmaProbeResponse(const uint8_t* dst, const char* ssid);

#endif
