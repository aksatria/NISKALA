#ifndef NISKALA_WARDRIVING_H
#define NISKALA_WARDRIVING_H

#include <Arduino.h>

extern bool isWardrivingRunning;

void wardrivingStart();
void wardrivingStop();
void wardrivingLoop();

#endif