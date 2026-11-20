#ifndef NISKALA_UI_ICONS_H
#define NISKALA_UI_ICONS_H

#include <Arduino.h>
#include <TFT_eSPI.h>

extern TFT_eSPI tft;

inline void drawMenuIcon(int idx, int x, int y, uint16_t color) {
  switch(idx) {
    case 0:
      tft.fillTriangle(x+5, y, x, y+8, x+5, y+6, color);
      tft.fillTriangle(x+5, y+6, x+5, y+14, x+10, y+6, color);
      break;
    case 1:
      tft.drawCircle(x+5, y+7, 5, color); tft.drawCircle(x+9, y+7, 5, color);
      break;
    case 2:
      tft.fillCircle(x+5, y+7, 2, color);
      tft.drawCircle(x+5, y+7, 5, color); tft.drawCircle(x+5, y+7, 8, color);
      break;
    case 3:
      tft.drawCircle(x+6, y+6, 4, color); tft.drawLine(x+9, y+9, x+13, y+13, color);
      break;
    case 4:
      tft.drawCircle(x+7, y+7, 5, color);
      tft.drawLine(x+7, y+1, x+7, y+13, color);
      tft.drawLine(x+1, y+7, x+13, y+7, color);
      break;
    case 5:
      tft.fillRect(x+2, y+2, 10, 10, color);
      tft.drawLine(x+4, y+4, x+10, y+10, TFT_BLACK);
      break;
    case 6:
      tft.drawLine(x+6, y, x+6, y+10, color);
      tft.drawLine(x+6, y+10, x+2, y+14, color);
      tft.drawLine(x+6, y+10, x+10, y+14, color);
      tft.drawCircle(x+6, y+4, 2, color);
      break;
    case 7:
      tft.drawCircle(x+4, y+5, 3, color);
      tft.drawLine(x+6, y+7, x+10, y+11, color);
      tft.drawLine(x+10, y+11, x+8, y+13, color);
      break;
    case 8:
      tft.drawCircle(x+7, y+7, 7, color);
      tft.drawCircle(x+7, y+7, 4, color);
      tft.fillCircle(x+7, y+7, 2, color);
      tft.drawLine(x+7, y, x+7, y+14, color);
      tft.drawLine(x, y+7, x+14, y+7, color);
      break;
    case 9:
      tft.drawCircle(x+7, y+7, 7, color);
      tft.drawLine(x+2, y+7, x+12, y+7, color);
      tft.drawLine(x+7, y+2, x+7, y+12, color);
      tft.drawLine(x+4, y+4, x+10, y+10, color);
      tft.drawLine(x+10, y+4, x+4, y+10, color);
      break;
    case 10:
      tft.drawRect(x+1, y+3, 10, 8, color);
      tft.drawLine(x+11, y+5, x+13, y+7, color);
      tft.drawLine(x+11, y+9, x+13, y+7, color);
      tft.fillRect(x+3, y+5, 6, 4, color);
      break;
    case 11:
      tft.drawLine(x+7, y+2, x+7, y+11, color);
      tft.drawLine(x+3, y+6, x+7, y+2, color);
      tft.drawLine(x+11, y+6, x+7, y+2, color);
      tft.drawFastHLine(x+2, y+13, 10, color);
      break;
    case 12:
      tft.drawCircle(x+7, y+7, 7, color);
      tft.drawLine(x+7, y+7, x+12, y+4, color);
      tft.drawLine(x+7, y+7, x+2, y+10, color);
      break;
    case 13:
      tft.drawCircle(x+7, y+4, 3, color);
      tft.drawCircle(x+7, y+4, 6, color);
      tft.drawFastHLine(x+2, y+12, 10, color);
      tft.drawLine(x+7, y+12, x+7, y+9, color);
      break;
    case 14:
      tft.drawRect(x+2, y+4, 10, 7, color);
      tft.drawRect(x+4, y+2, 6, 7, color);
      tft.drawLine(x+7, y+9, x+7, y+13, color);
      break;
    case 15:
      tft.drawCircle(x+7, y+7, 4, color);
      tft.drawCircle(x+7, y+7, 7, color);
      tft.drawFastHLine(x+2, y+13, 10, color);
      break;
    case 16:
      tft.drawCircle(x+7, y+7, 4, color);
      tft.drawCircle(x+7, y+7, 7, color);
      tft.drawLine(x+7, y+1, x+7, y+13, color);
      tft.drawLine(x+1, y+7, x+13, y+7, color);
      break;
    case 17:
      tft.drawCircle(x+4, y+3, 3, color);
      tft.drawLine(x+6, y+5, x+13, y+12, color);
      tft.drawLine(x+9, y+12, x+13, y+12, color);
      tft.drawLine(x+13, y+9, x+13, y+12, color);
      break;
  }
}

#endif