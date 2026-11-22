#ifndef NISKALA_SMART_ATTACK_H
#define NISKALA_SMART_ATTACK_H

#include <Arduino.h>
#include <stdint.h>

extern int g_smartRecommendation;
extern int selectedIdx;
extern int templateSelection;
bool detectPMF(int idx);
bool detectPMFByBssid(const uint8_t* bssid, uint8_t channel, bool* found);
extern volatile bool g_pmfChecked;
extern volatile bool g_pmfResult;
extern volatile bool g_pmfFoundFlag;

void drawSmartProfile();
void startSmartRecommendedAttack();

#endif