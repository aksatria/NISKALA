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
extern volatile bool g_karmaActiveMode;
extern volatile uint32_t g_packetsSent;
extern volatile uint32_t g_txErrors;

// Curated SSID list untuk broadcast probe — 5 SSID paling umum di PNL publik
static const char* CURATED_SSIDS[] = {
  "Free WiFi", "Starbucks WiFi", "IndiHome", "MikroTik", "TP-Link"
};
static const int CURATED_COUNT = 5;

inline void karmaLogSSID(const char* ssid) {
  for (int i = 0; i < g_karmaSSIDCount; i++) {
    if (strcmp(g_karmaSSIDs[i].ssid, ssid) == 0) {
      g_karmaSSIDs[i].probeCount++;
      return;
    }
  }
  if (g_karmaSSIDCount < KARMA_MAX_SSIDS) {
    strncpy(g_karmaSSIDs[g_karmaSSIDCount].ssid, ssid, 32);
    g_karmaSSIDs[g_karmaSSIDCount].ssid[32] = 0;
    g_karmaSSIDs[g_karmaSSIDCount].probeCount = 1;
    g_karmaSSIDs[g_karmaSSIDCount].authCount = 0;
    g_karmaSSIDs[g_karmaSSIDCount].used = true;
    g_karmaSSIDCount++;
  } else {
    int minIdx = 0;
    for (int i = 1; i < KARMA_MAX_SSIDS; i++) {
      if (g_karmaSSIDs[i].probeCount < g_karmaSSIDs[minIdx].probeCount) {
        minIdx = i;
      }
    }
    strncpy(g_karmaSSIDs[minIdx].ssid, ssid, 32);
    g_karmaSSIDs[minIdx].ssid[32] = 0;
    g_karmaSSIDs[minIdx].probeCount = 1;
    g_karmaSSIDs[minIdx].authCount = 0;
  }
}

inline void karmaLogClient(const uint8_t* mac, int8_t rssi) {
  for (int i = 0; i < g_karmaClientCount; i++) {
    if (memcmp(g_karmaClients[i].mac, mac, 6) == 0) {
      g_karmaClients[i].lastRSSI = rssi;
      g_karmaClients[i].probeCount++;
      g_karmaClients[i].lastSeen = millis();
      return;
    }
  }
  if (g_karmaClientCount < KARMA_MAX_CLIENTS) {
    memcpy(g_karmaClients[g_karmaClientCount].mac, mac, 6);
    g_karmaClients[g_karmaClientCount].lastRSSI = rssi;
    g_karmaClients[g_karmaClientCount].probeCount = 1;
    g_karmaClients[g_karmaClientCount].lastSeen = millis();
    g_karmaClients[g_karmaClientCount].used = true;
    g_karmaClientCount++;
  }
}

// Bangun dan kirim probe response frame
inline void sendKarmaProbeResponse(const uint8_t* dst, const char* ssid) {
  uint8_t frame[128];
  int ssidLen = strlen(ssid);
  if (ssidLen > 32) ssidLen = 32;
  int p = 0;

  // Frame Control: Probe Response (subtype 5)
  frame[p++] = 0x50; frame[p++] = 0x00;
  // Duration
  frame[p++] = 0x00; frame[p++] = 0x00;
  // Addr1 = Destination (client MAC)
  memcpy(&frame[p], dst, 6); p += 6;

  // Addr2 = Addr3 = Our MAC
  uint8_t mac[6];
  esp_wifi_get_mac(WIFI_IF_AP, mac);
  memcpy(&frame[p], mac, 6); p += 6;
  memcpy(&frame[p], mac, 6); p += 6;

  // Sequence control
  frame[p++] = 0x00; frame[p++] = 0x00;

  // Timestamp (8 bytes)
  for (int i = 0; i < 8; i++) frame[p++] = 0x00;

  // Beacon interval
  frame[p++] = 0x64; frame[p++] = 0x00;

  // Capability: ESS + Privacy (0x0011)
  frame[p++] = 0x11; frame[p++] = 0x04;

  // SSID IE
  frame[p++] = 0x00;
  frame[p++] = (uint8_t)ssidLen;
  memcpy(&frame[p], ssid, ssidLen); p += ssidLen;

  // Supported rates
  frame[p++] = 0x01; frame[p++] = 0x08;
  frame[p++] = 0x82; frame[p++] = 0x84; frame[p++] = 0x8B; frame[p++] = 0x96;
  frame[p++] = 0x0C; frame[p++] = 0x12; frame[p++] = 0x18; frame[p++] = 0x24;

  // DS Parameter (current channel)
  uint8_t channel;
  wifi_second_chan_t sec;
  esp_wifi_get_channel(&channel, &sec);
  frame[p++] = 0x03; frame[p++] = 0x01; frame[p++] = channel;

  esp_err_t err = sendWifiFrameAndCount(frame, p, true);
  if (err == ESP_OK) g_karmaResponses++;
}

#endif