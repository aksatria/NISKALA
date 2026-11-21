#ifndef BT_SPAM_H
#define BT_SPAM_H

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEAdvertising.h>
#include <string>

// State BLE global (definisi tunggal di bt_spam.cpp)
extern BLEServer*      g_bleServer;
extern BLEAdvertising* g_bleAdv;
extern bool            g_bleActive;
extern bool            g_bleFixed;   // mode payload tetap (raw/iBeacon/Eddystone)
extern std::string     g_bleFixedData;

// UUID/payload default yang juga dipakai CLI
extern const uint8_t IBEACON_DEFAULT_UUID[16];
extern const uint8_t EDDYSTONE_NS[10];
extern const uint8_t EDDYSTONE_INST[6];

size_t buildIBeacon(uint8_t* out, const uint8_t uuid[16], uint16_t major, uint16_t minor, int8_t txPower);
size_t buildEddystoneUID(uint8_t* out, const uint8_t ns[10], const uint8_t inst[6], int8_t txPower);
bool btSpamInit();
void btAdvertisePayload(const std::string& data);
void btSpamTick();
bool btStartFixed(const uint8_t* data, size_t len);
bool btStartIBeacon(const uint8_t uuid[16], uint16_t major, uint16_t minor, int8_t txPower);
bool btStartEddystone(const uint8_t ns[10], const uint8_t inst[6], int8_t txPower);
void btStopFixed();
void btSpamStop();

#endif
