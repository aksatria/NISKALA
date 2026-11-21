#ifndef NISKALA_BLE_SCAN_H
#define NISKALA_BLE_SCAN_H

#include <Arduino.h>

#define BLE_SCAN_MAX 32

struct BleScanEntry {
  char mac[18];
  char name[33];
  int8_t rssi;
  unsigned long firstSeen;
  uint16_t seenCount;
  unsigned long lastSeen;
  bool used;
};

extern BleScanEntry g_bleScanLog[BLE_SCAN_MAX];
extern int g_bleScanCount;
extern bool g_bleScanning;
extern unsigned long g_bleScanStart;

const char* vendorFromMacStr(const char* macStr);
void drawBleScan();
void startBLEScan();
void stopBLEScan();
String buildBleJson();

#endif