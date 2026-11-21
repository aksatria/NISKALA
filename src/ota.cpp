#include "ota.h"
#include <ArduinoOTA.h>
#include <WiFi.h>
#include "ui_helpers.h"
#include "wifi_state.h"

bool isOtaRunning = false;
void drawOtaScreen() {
  tft.fillScreen(TFT_BLACK);
  drawHeader("OTA UPDATE", TFT_DARKCYAN);
  tft.setTextFont(2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(10, 30); tft.print("SSID: ");
  tft.setTextColor(TFT_CYAN, TFT_BLACK); tft.println("NISKALA_OTA");
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(10, 52); tft.print("Pass: ");
  tft.setTextColor(TFT_YELLOW, TFT_BLACK); tft.println("niskala123");
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(10, 74); tft.print("IP  : ");
  tft.setTextColor(TFT_GREEN, TFT_BLACK); tft.println("192.168.4.1");
  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.setCursor(10, 96); tft.print("pio run -e ota -t upload");
  drawFooter("TOMBOL = KELUAR");
}

void startOTA() {
  radioSetMode(RADIO_AP);
  restoreOriginalApMac();
  WiFi.softAP("NISKALA_OTA", "niskala123", 1, 0, 1);
  delay(300);
  ArduinoOTA.setHostname("niskala");
  ArduinoOTA.onStart([]() { Serial.println("[OTA] Start"); });
  ArduinoOTA.onEnd([]() { Serial.println("[OTA] End"); });
  ArduinoOTA.onError([](ota_error_t error) { Serial.printf("[OTA] Error %u\n", error); });
  ArduinoOTA.begin();
  isOtaRunning = true;
  drawOtaScreen();
  Serial.println("[OTA] Aktif di 192.168.4.1");
}

void stopOTA() {
  isOtaRunning = false;
  resetWifiStateAfterAttack();
  Serial.println("[OTA] Berhenti");
}

