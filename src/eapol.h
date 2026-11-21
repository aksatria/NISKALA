#ifndef NISKALA_EAPOL_H
#define NISKALA_EAPOL_H

#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>

extern bool isEapolRunning;
extern unsigned long eapolStartTime;
extern int eapolM1;
extern int eapolM2;
extern int eapolM3;
extern int eapolM4;
extern int eapolFull;
extern int eapolPacketCount;

void startEapolCapture();
void stopEapolCapture();
void eapolCaptureTick();
void drawEapolCapture(int frame);
String buildEapolJson();

#endif