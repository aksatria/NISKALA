#ifndef DISCLAIMER_H
#define DISCLAIMER_H

#include <Arduino.h>
#include <TFT_eSPI.h>

inline void drawDisclaimerScreen(TFT_eSPI &tft, const char* fwVersion) {
  tft.fillScreen(TFT_BLACK);
  tft.fillRect(0, 0, 240, 24, TFT_RED);
  tft.setTextDatum(MC_DATUM);
  tft.setTextFont(2);
  tft.setTextColor(TFT_WHITE, TFT_RED);
  tft.drawString("NISKALA - PERINGATAN HUKUM", 120, 12);
  tft.setTextDatum(TL_DATUM);

  tft.setTextFont(1);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setCursor(6, 30); tft.println("Alat ini HANYA untuk edukasi,");
  tft.setCursor(6, 42); tft.println("audit keamanan, dan pengujian");

  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setCursor(6, 54); tft.println("pada jaringan MILIK SENDIRI");
  tft.setCursor(6, 66); tft.println("atau yang Anda punya IZIN.");

  tft.setTextColor(TFT_RED, TFT_BLACK);
  tft.setCursor(6, 80); tft.println("DILARANG KERAS:");

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(6, 92); tft.println("- Menyerang WiFi orang lain");
  tft.setCursor(6, 104); tft.println("- Tanpa izin tertulis pemilik");

  tft.drawFastHLine(0, 116, 240, TFT_DARKGREY);
  tft.fillRect(0, 117, 240, 18, TFT_MAROON);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_MAROON);
  tft.drawString("TEKAN TOMBOL = LANJUT", 120, 126);
  tft.setTextDatum(TL_DATUM);

  tft.setTextFont(1);
  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.setCursor(200, 116);
  tft.print(fwVersion);
}

inline void drawConfirmationScreen(TFT_eSPI &tft) {
  tft.fillScreen(TFT_BLACK);
  tft.fillRect(0, 0, 240, 24, TFT_ORANGE);
  tft.setTextDatum(MC_DATUM);
  tft.setTextFont(2);
  tft.setTextColor(TFT_BLACK, TFT_ORANGE);
  tft.drawString("KONFIRMASI TANGGUNG JAWAB", 120, 12);
  tft.setTextDatum(TL_DATUM);

  tft.setTextFont(1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(10, 34); tft.println("Dengan melanjutkan, saya");
  tft.setCursor(10, 48); tft.println("menyatakan bahwa:");

  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setCursor(10, 70); tft.println("- Saya berwenang atas target");
  tft.setCursor(10, 84); tft.println("- Saya bertanggung jawab penuh");
  
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setCursor(10, 98); tft.println("- Etika > Skill, Adab > Ilmu");

  tft.drawFastHLine(0, 116, 240, TFT_DARKGREY);
  tft.fillRect(0, 117, 240, 18, TFT_DARKGREEN);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_DARKGREEN);
  tft.drawString("ATAS=BATAL | BAWAH=SETUJU", 120, 126);
  tft.setTextDatum(TL_DATUM);
}

#endif