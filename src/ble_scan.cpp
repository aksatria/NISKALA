#include "ble_scan.h"
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include "ui_helpers.h"
#include "wifi_state.h"

BleScanEntry g_bleScanLog[BLE_SCAN_MAX];
int g_bleScanCount = 0;
bool g_bleScanning = false;
unsigned long g_bleScanStart = 0;
class BleScanCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice d) {
    String mac = String(d.getAddress().toString().c_str());
    int8_t rssi = d.getRSSI();
    String name = d.haveName() ? String(d.getName().c_str()) : String("");

    for (int i = 0; i < g_bleScanCount; i++) {
      if (strcmp(g_bleScanLog[i].mac, mac.c_str()) == 0) {
        g_bleScanLog[i].rssi = rssi;
        g_bleScanLog[i].lastSeen = millis();
        g_bleScanLog[i].seenCount++;
        if (name.length() > 0) {
          strncpy(g_bleScanLog[i].name, name.c_str(), 32);
          g_bleScanLog[i].name[32] = 0;
        }
        return;
      }
    }
    if (g_bleScanCount < BLE_SCAN_MAX) {
      BleScanEntry* e = &g_bleScanLog[g_bleScanCount];
      strncpy(e->mac, mac.c_str(), 17);
      e->mac[17] = 0;
      strncpy(e->name, name.c_str(), 32);
      e->name[32] = 0;
      e->rssi = rssi;
      e->lastSeen = millis();
      e->firstSeen = millis();
      e->seenCount = 1;
      e->used = true;
      g_bleScanCount++;
    }
  }
};

BLEScan* g_bleScan = nullptr;

void drawBleScan() {
  tft.fillScreen(TFT_BLACK);
  drawHeader("BLE SCANNER", TFT_BLUE);
  tft.setTextFont(1);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setCursor(4, 28);
  tft.print("MAC               RSSI CNT  NAME");

  int shown = 0;
  for (int i = 0; i < g_bleScanCount && shown < 4; i++) {
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(4, 40 + shown * 18);
    char buf[64];
    const char* bname = g_bleScanLog[i].name[0] ? g_bleScanLog[i].name : vendorFromMacStr(g_bleScanLog[i].mac);
    snprintf(buf, sizeof(buf), "%-17s %4d %3u %s",
             g_bleScanLog[i].mac, g_bleScanLog[i].rssi, g_bleScanLog[i].seenCount, bname);
    tft.print(buf);
    shown++;
  }
  if (g_bleScanCount == 0) {
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.setCursor(4, 50);
    tft.print("Menunggu perangkat BLE...");
  }
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setCursor(4, 108);
  tft.print("Total: " + String(g_bleScanCount));
  drawFooter("TOMBOL = STOP");
}

void startBLEScan() {
  WiFi.mode(WIFI_OFF);
  if (!BLEDevice::getInitialized()) BLEDevice::init("");
  g_bleScan = BLEDevice::getScan();
  g_bleScan->setAdvertisedDeviceCallbacks(new BleScanCallbacks());
  g_bleScan->setActiveScan(true);
  g_bleScan->setInterval(100);
  g_bleScan->setWindow(99);
  g_bleScanCount = 0;
  g_bleScanStart = millis();
  g_bleScan->start(0, false);
  g_bleScanning = true;
  Serial.println("[BLE] Scanner aktif");
  drawBleScan();
}

void stopBLEScan() {
  if (g_bleScan) g_bleScan->stop();
  g_bleScanning = false;
  BLEDevice::deinit(true);
  resetWifiStateAfterAttack();
  Serial.println("[BLE] Scanner berhenti");
}

String buildBleJson() {
  String j = "[";
  for (int i = 0; i < g_bleScanCount; i++) {
    if (i) j += ",";
    j += "{\"mac\":\"" + String(g_bleScanLog[i].mac) + "\",";
    j += "\"name\":\"" + String(g_bleScanLog[i].name) + "\",";
    j += "\"rssi\":" + String(g_bleScanLog[i].rssi) + ",";
    j += "\"first_seen\":" + String((unsigned long)g_bleScanLog[i].firstSeen) + ",";
    j += "\"last_seen\":" + String((unsigned long)g_bleScanLog[i].lastSeen) + ",";
    j += "\"seen_count\":" + String((uint16_t)g_bleScanLog[i].seenCount) + "}";
  }
  j += "]";
  return j;
}
