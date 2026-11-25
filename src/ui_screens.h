#ifndef NISKALA_UI_SCREENS_H
#define NISKALA_UI_SCREENS_H

#include <Arduino.h>
#include <TFT_eSPI.h>

void drawGlowText(String text, int x, int y, uint16_t mainColor, uint16_t glowColor);
void drawCornerBrackets(int x, int y, int w, int h, uint16_t color);
void drawElegantSplash();
void drawScanAnimation(int frame);
int readBatteryPercent();
float readBatteryVoltage();
void drawMainMenu();
void drawSignalBars(int cx, int y, int rssi, uint16_t color);
void drawBackIcon(int cx, int cy, uint16_t color);
void drawTemplateIconBig(int idx, int cx, int cy, uint16_t color);
void drawAPList();
void drawTemplateSelect();
void drawEvilTwinModeSelect();
void drawDeauthMenu();
void drawAuthFlood(int frame);
void drawGlobalDeauth(int frame);
void drawBeaconSpam(int frame);
const char* vendorFromMacBytes(const uint8_t* m);
const char* vendorFromMacStr(const char* macStr);
void drawProbeSniff(int frame);
void drawBTSpam(int frame);
void drawCSARunning(int frame);
int drawWrapped(String text, int x, int y, int maxW, uint16_t color, int lineH);
void drawEvilTwinRunning(int frame);
void drawViewPasswords();
void sortKarmaSSIDs();
void drawKarmaScan(int frame);
void drawKarmaResults();
void drawWebRemoteScreen();
void drawPmfResultScreen(bool pmf, bool found);
void setBacklight(uint8_t duty);
void drawTiltedOrbit(int cx, int cy, int rx, int ry, float angle, uint16_t color);
void drawButterflyScreensaver(int frame);

#endif