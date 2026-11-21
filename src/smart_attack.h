#ifndef NISKALA_SMART_ATTACK_H
#define NISKALA_SMART_ATTACK_H

#include <Arduino.h>

extern int g_smartRecommendation;
extern int selectedIdx;
extern int templateSelection;
bool detectPMF(int idx);

void drawSmartProfile();
void startSmartRecommendedAttack();

#endif