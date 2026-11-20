#ifndef NISKALA_MACRO_H
#define NISKALA_MACRO_H

#include <Arduino.h>

enum MacroAction {
  MACT_BEACON,
  MACT_GLOBAL_DEAUTH,
  MACT_PROBE,
  MACT_KARMA,
  MACT_BT_SPAM
};

struct MacroStep {
  uint8_t action;
  uint32_t durationMs;
};

static const MacroStep MACRO_RECON[] = {
  {MACT_PROBE, 8000},
  {MACT_KARMA, 8000},
};
static const int MACRO_RECON_STEPS = 2;

static const MacroStep MACRO_ASSAULT[] = {
  {MACT_BEACON, 6000},
  {MACT_GLOBAL_DEAUTH, 6000},
};
static const int MACRO_ASSAULT_STEPS = 2;

static const int MACRO_COUNT = 2;
static const char* MACRO_NAMES[] = { "RECON", "ASSAULT" };
static const char* MACRO_DESC[] = { "Probe + Karma (pasif)", "Beacon + Deauth (aktif)" };

extern bool isMacroRunning;
extern int macroIndex;
extern int macroPhase;
extern int macroSelection;
extern unsigned long macroPhaseStart;

extern bool isBeaconRunning;
extern bool isGlobalDeauthRunning;
extern bool isProbeRunning;
extern bool isKarmaRunning;
extern bool isBTSpamRunning;

void startBeaconSpam();
void stopBeaconSpam();
void startGlobalDeauth();
void stopGlobalDeauth();
void startProbeSniffer();
void stopProbeSniffer();
void startKarmaScan();
void stopKarmaScan();
void startBTSpam();
void stopBTSpam();
void stopAllAttacks();

void drawMainMenu();
void drawMacroMenu();
void drawMacroRunning(int frame);
void startMacro(int idx);
void macroTick();

#endif