#ifndef NISKALA_ATTACK_CONTROL_H
#define NISKALA_ATTACK_CONTROL_H

#include <Arduino.h>

// Kontrol start/stop modul serangan & web remote.
// Implementasi di attack_control.cpp; state global tetap di main.cpp.

void applyPromiscuousFilter(bool probeOnly);

void startKarmaScan();
void stopKarmaScan();

void startDeauthEngine();
void stopDeauthEngine();

void startAuthFlood();
void stopAuthFlood();

void startGlobalDeauth();
void stopGlobalDeauth();

void startBeaconSpam();
void stopBeaconSpam();

void startProbeSniffer();
void stopProbeSniffer();

void startBTSpam();
void stopBTSpam();

void startCSAAttack();
void stopCSAAttack();

void startWebRemote();
void stopWebRemote();

String buildSyncJson();
void stopAttacksKeepWebRemote();
void stopAllAttacks();

#endif