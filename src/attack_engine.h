#ifndef ATTACK_ENGINE_H
#define ATTACK_ENGINE_H

#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <string.h>

#include "tx_count.h"
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
extern portMUX_TYPE g_stateMux;
inline AttackMode getAttackMode() { AttackMode m; portENTER_CRITICAL(&g_stateMux); m = g_attackMode; portEXIT_CRITICAL(&g_stateMux); return m; }
inline void setAttackMode(AttackMode m) { portENTER_CRITICAL(&g_stateMux); g_attackMode = m; portEXIT_CRITICAL(&g_stateMux); }
inline bool isVictimFound() { bool v; portENTER_CRITICAL(&g_stateMux); v = g_victimFound; portEXIT_CRITICAL(&g_stateMux); return v; }
inline void setVictimFound(bool v) { portENTER_CRITICAL(&g_stateMux); g_victimFound = v; portEXIT_CRITICAL(&g_stateMux); }
inline void getVictimMac(uint8_t out[6]) { portENTER_CRITICAL(&g_stateMux); memcpy(out, g_victimMAC, 6); portEXIT_CRITICAL(&g_stateMux); }
inline void setVictimMac(const uint8_t* mac) { portENTER_CRITICAL(&g_stateMux); memcpy(g_victimMAC, mac, 6); portEXIT_CRITICAL(&g_stateMux); }

extern uint8_t g_targetBSSID[6];
extern volatile uint8_t g_targetChannel;
extern unsigned long evilTwinStartTime;
extern bool g_evilTwinCoop;

// CSA target SSID (diisi oleh main.cpp saat start)
extern char g_csaTargetSSID[33];

// SSID pool beacon spam (definisi di attack_engine.cpp)
extern const char* SPAM_SSIDS[];
extern const int SPAM_COUNT;

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

// Fungsi engine (implementasi di attack_engine.cpp)
void clientMonitorLog(const uint8_t* mac, int8_t rssi);
void sendDeauthFrame(const uint8_t* dst, const uint8_t* bssid, uint8_t reason = 0x01);
void sendDisassocFrame(const uint8_t* dst, const uint8_t* bssid, uint8_t reason = 0x08);
void sendDeauthBurst(const uint8_t* bssid);
void sendBroadcastDeauthOnChannel(uint8_t n);
void globalModeTick();
void sendAuthFrame(const uint8_t* srcMAC, const uint8_t* bssid);
void sendAuthFloodTick();
void sendBeaconFrame(const char* ssid, const uint8_t* bssid, uint8_t ch, uint8_t akm = 0x02);
void sendBeaconSpamTick();
void sendCSABeacon(const uint8_t* bssid, const char* ssid, uint8_t curCh, uint8_t newCh, uint8_t count);
void sendCSAActionFrame(const uint8_t* dst, const uint8_t* bssid, uint8_t newCh, uint8_t count);
void sendCSATick();
void packetSniffer_cb(void* buf, wifi_promiscuous_pkt_type_t type);
void attackTask(void* param);

#endif
