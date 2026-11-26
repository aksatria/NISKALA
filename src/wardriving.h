#ifndef NISKALA_WARDRIVING_H
#define NISKALA_WARDRIVING_H

#include <Arduino.h>

extern bool isWardrivingRunning;

void wardrivingSetup();
void wardrivingStart(); // alias untuk wardrivingSetup (kompatibilitas CLI)
void wardrivingStop();
void wardrivingLoop();

#endif