#ifndef NISKALA_ALLOWLIST_H
#define NISKALA_ALLOWLIST_H

#include <Arduino.h>
#include <Preferences.h>

bool clientAllowed(const uint8_t* mac);
bool allowlistAdd(const uint8_t* mac);
bool allowlistDelete(int idx);
int allowlistCount();
void allowlistPrint();

#endif