#ifndef NISKALA_CLI_H
#define NISKALA_CLI_H

#include <Arduino.h>

void cliPrintHelp();
void processCliCommand(String line);
void handleSerialCLI();

void drawMainMenu();
void drawBeaconSpam(int frame);
void drawProbeSniff(int frame);
void drawKarmaScan(int frame);
void drawGlobalDeauth(int frame);
void drawBTSpam(int frame);
void drawDeauthMenu();

#endif