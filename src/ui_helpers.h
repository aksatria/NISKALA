#ifndef NISKALA_UI_HELPERS_H
#define NISKALA_UI_HELPERS_H

#include <Arduino.h>
#include <TFT_eSPI.h>

extern TFT_eSPI tft;

inline void drawHeader(String title, uint16_t bgColor) {
  tft.fillRect(0, 0, 240, 24, bgColor);
  tft.setTextDatum(MC_DATUM); tft.setTextFont(2);
  tft.setTextColor(TFT_WHITE, bgColor);
  tft.drawString(title, 120, 12);
  tft.drawFastHLine(0, 24, 240, TFT_DARKGREY);
  tft.setTextDatum(TL_DATUM);
}

inline void drawFooter(String hint) {
  tft.drawFastHLine(0, 115, 240, TFT_DARKGREY);
  tft.fillRect(0, 116, 240, 19, TFT_BLACK);
  tft.setTextDatum(MC_DATUM); tft.setTextFont(1);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawString(hint, 120, 125);
  tft.setTextDatum(TL_DATUM);
}

#endif