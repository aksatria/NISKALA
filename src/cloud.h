#ifndef NISKALA_CLOUD_H
#define NISKALA_CLOUD_H

#include <Arduino.h>
#include <WiFi.h>
#if __has_include("secrets.h")
  #include "secrets.h"
#endif

#ifndef NISKALA_SYNC_API_KEY
  #define NISKALA_SYNC_API_KEY ""
#endif
#ifndef SYNC_API_KEY
  #define SYNC_API_KEY NISKALA_SYNC_API_KEY
#endif

#include "state_machine.h"
#include "ui_helpers.h"

#define CLOUD_PHASE_IDLE 0
#define CLOUD_PHASE_CONNECTING 1
#define CLOUD_PHASE_SENDING 2
#define CLOUD_PHASE_DONE 3
#define TPL_PHASE_IDLE 0
#define TPL_PHASE_CONNECTING 1
#define TPL_PHASE_GETTING 2

extern int g_cloudPhase;
extern unsigned long g_cloudTimer;
extern int g_tplPhase;
extern unsigned long g_tplTimer;

extern String cfg_wifi_ssid;
extern String cfg_wifi_pass;
extern String cfg_server_url;
extern String cfg_template_url;
extern String g_customTemplate;
extern bool useCustomTemplate;
extern int selectedTemplateIdx;
extern int verifyAttempts;
extern int verifyState;

void startCloudSync();
void cloudSyncTick();
void startFetchCustomTemplate();
void templateFetchTick();
String buildSyncJson();
void drawMainMenu();
void drawEvilTwinRunning(int frame);
void startEvilTwinWithDeauth();

#endif