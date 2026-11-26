#ifndef NISKALA_OUI_H
#define NISKALA_OUI_H

#include <Arduino.h>

// Lookup vendor dari 3 byte pertama MAC (OUI).
// Tabel ringan khusus ESP32 (const otomatis ditempatkan di flash/rodata).

struct OuiEntry {
  uint32_t oui;
  const char* vendor;
};

static const OuiEntry OUI_TABLE[] = {
  { 0xACBC32, "Apple" }, { 0x8C8590, "Apple" }, { 0xF01898, "Apple" },
  { 0x308D99, "Apple" }, { 0x3C22FB, "Apple" }, { 0x40CBD0, "Apple" },
  { 0x8C0D76, "Samsung" }, { 0xCC07AB, "Samsung" }, { 0x3C64EA, "Samsung" },
  { 0x640980, "Xiaomi" }, { 0x7811DC, "Xiaomi" }, { 0xF0B429, "Xiaomi" },
  { 0x8C4B14, "Xiaomi" }, { 0x444A91, "Xiaomi" },
  { 0x00E0FC, "Huawei" }, { 0x286ED4, "Huawei" }, { 0xD80BCB, "Huawei" },
  { 0x546009, "Google" }, { 0x3C5AB4, "Google" }, { 0x58CB52, "Google" },
  { 0x0050F2, "Microsoft" }, { 0x2816A8, "Microsoft" },
  { 0x001302, "Intel" }, { 0x340286, "Intel" },
  { 0x50C7BF, "TP-Link" }, { 0xA42BB0, "TP-Link" }, { 0x64B64A, "TP-Link" },
  { 0x502B98, "Realme" }, { 0x086438, "Realme" },
  { 0x08D46A, "Oppo" }, { 0x702C1F, "Oppo" },
  { 0x241177, "Vivo" }, { 0x7802F8, "Vivo" },
  { 0x0024BE, "Sony" }, { 0x405BAD, "Sony" },
  { 0x0C8BFD, "OnePlus" }, { 0xE05B70, "OnePlus" },
  { 0x0015EB, "ZTE" }, { 0x286C07, "ZTE" },
  { 0xC8D15E, "Asus" }, { 0x9C5C8E, "Asus" }, { 0x2451EA, "Asus" },
  { 0x9C97CA, "Netgear" }, { 0x20E52A, "Netgear" },
  { 0xD88039, "D-Link" }, { 0x1C7EE5, "D-Link" },
  { 0x0018F8, "Cisco" }, { 0x7018A7, "Cisco" },
  { 0x6CDD30, "Ubiquiti" }, { 0xFCECDA, "Ubiquiti" },
  { 0x244B03, "Aruba" }, { 0xD8C7C8, "Aruba" },
  { 0x4C5D3C, "Motorola" }, { 0x74DA88, "Lenovo" },
  { 0xB827EB, "Raspberry Pi" }, { 0xDC4EDE, "Raspberry Pi" },
  { 0x00C2C6, "Espressif" }, { 0x24D7EB, "Espressif" },
};

inline const char* ouiVendorFromMac(const uint8_t* m) {
  uint32_t oui = ((uint32_t)m[0] << 16) | ((uint32_t)m[1] << 8) | m[2];
  const size_t n = sizeof(OUI_TABLE) / sizeof(OUI_TABLE[0]);
  for (size_t i = 0; i < n; i++) {
    if (OUI_TABLE[i].oui == oui) return OUI_TABLE[i].vendor;
  }
  return "Unknown";
}

#endif