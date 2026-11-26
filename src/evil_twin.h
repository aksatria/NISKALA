#ifndef NISKALA_EVIL_TWIN_H
#define NISKALA_EVIL_TWIN_H

#include <Arduino.h>

// Modul Evil Twin + verifikasi password, dipisah dari main.cpp
// agar tanggung jawab tidak menumpuk di satu file.

#define MAX_VERIFY_ATTEMPTS 5
#define VERIFY_TIMEOUT_MS 6000
#define RELEASE_DELAY_MS 2000
#define SUCCESS_HOLD_MS 4000

// State Evil Twin (definisi di evil_twin.cpp)
extern bool isEvilTwinRunning;
extern bool isReleasing;
extern bool isVerifying;
extern bool verificationResult;
extern unsigned long releaseTimer;
extern unsigned long verifyStart;
extern int verifyState;
extern int verifyAttempts;
extern unsigned long releaseAfterSuccess;
extern unsigned long evilTwinStartTime;
extern bool g_evilTwinCoop;
extern int evilTwinModeSelection;
extern String capturedEmail;
extern String capturedPass;

String getBestDiscoveredSSID();
void startEvilTwinWithDeauth();
void stopEvilTwin();
void beginVerification();
void updateVerification();
void handleEvilTwinModeSelect();

#endif
