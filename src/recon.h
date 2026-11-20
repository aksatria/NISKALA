#ifndef NISKALA_RECON_H
#define NISKALA_RECON_H

#include <Arduino.h>
#include <IPAddress.h>
#include <WiFi.h>
#include <Preferences.h>
#include "state_machine.h"

#define RECON_MAX_HOSTS 20
#define RECON_MAX_PORTS 14
#define MAX_AP_WIFI 20
#define RECON_PHASE_CONNECTING 0
#define RECON_PHASE_SWEEP 1
#define RECON_PHASE_HOST_LIST 2
#define RECON_PHASE_PORT_SCAN 3
#define RECON_PHASE_BANNER 4
#define RECON_PHASE_PORT_RESULT 5

extern IPAddress g_reconHosts[RECON_MAX_HOSTS];
extern int g_reconHostCount;
extern int g_reconSelection;
extern int g_reconScroll;
extern int g_reconPhase;
extern int g_reconOpenPorts[RECON_MAX_PORTS];
extern int g_reconOpenPortCount;
extern IPAddress g_reconSelectedIP;
extern bool isReconRunning;
extern int g_reconSelectedAuth;
extern uint32_t g_reconFirst;
extern uint32_t g_reconLast;
extern uint32_t g_reconNext;
extern uint32_t g_reconTotal;
extern uint32_t g_reconScanned;
extern unsigned long g_reconConnectStart;
extern int g_reconPortIndex;
extern int g_reconBannerIndex;
extern IPAddress g_reconLocalIP;
extern IPAddress g_reconGateway;
extern IPAddress g_reconSubnet;
extern char g_reconBanners[RECON_MAX_PORTS][40];
extern WiFiClient g_reconClient;

extern int apWifiSelection;
extern int apWifiScrollPos;
extern bool confirmApWifiDelete;
extern unsigned long lastApWifiDeletePress;

extern Preferences apPrefs;
extern Preferences prefs;
extern String cfg_wifi_ssid;
extern String cfg_wifi_pass;
extern String targetSSID;

bool apWifiAdd(String ssid, String pass);
String apWifiFindPassword(String ssid);
void apWifiList();
bool apWifiDelete(int idx);
String findReconPasswordForSSID(String ssid);
void startNetworkRecon();
void stopNetworkRecon();
void reconTick();
void drawNetworkRecon();
void drawApWifiView();
void reconPortScan(int hostIdx);

void drawMainMenu();

#endif