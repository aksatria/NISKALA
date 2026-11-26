#ifndef NISKALA_TFT_COMPAT_H
#define NISKALA_TFT_COMPAT_H

// Abstraksi layar TFT: saat NISKALA_HAS_TFT=1 pakai TFT_eSPI asli,
// saat 0 pakai stub no-op agar firmware benar-benar "headless" tanpa
// menarik library TFT_eSPI dan tanpa menyentuh SPI/display sama sekali.

#include <Arduino.h>
#include "board_config.h"

#if NISKALA_HAS_TFT
  #include <TFT_eSPI.h>
#else

// ---- Konstanta warna & datum (kompatibel dengan TFT_eSPI) ----
#define TFT_BLACK       0x0000
#define TFT_NAVY        0x000F
#define TFT_DARKGREEN   0x03E0
#define TFT_DARKCYAN    0x03EF
#define TFT_MAROON      0x7800
#define TFT_PURPLE      0x780F
#define TFT_OLIVE       0x7BE0
#define TFT_LIGHTGREY   0xC618
#define TFT_DARKGREY    0x7BEF
#define TFT_BLUE        0x001F
#define TFT_GREEN       0x07E0
#define TFT_CYAN        0x07FF
#define TFT_RED         0xF800
#define TFT_MAGENTA     0xF81F
#define TFT_YELLOW      0xFFE0
#define TFT_WHITE       0xFFFF
#define TFT_ORANGE      0xFDA0
#define TFT_GREENYELLOW 0xB7E0
#define TFT_PINK        0xFC9F
#define TFT_BL          0x0000

#define TL_DATUM 0
#define TC_DATUM 1
#define TR_DATUM 2
#define ML_DATUM 3
#define CL_DATUM 3
#define MC_DATUM 4
#define CC_DATUM 4
#define MR_DATUM 5
#define CR_DATUM 5
#define BL_DATUM 6
#define BC_DATUM 7
#define BR_DATUM 8

// Stub TFT_eSPI headless: seluruh metode no-op.
class TFT_eSPI {
public:
  void init() {}
  void begin() {}
  void fillScreen(uint32_t) {}
  void fillRect(int32_t, int32_t, int32_t, int32_t, uint32_t) {}
  void fillRoundRect(int32_t, int32_t, int32_t, int32_t, int32_t, uint32_t) {}
  void drawRect(int32_t, int32_t, int32_t, int32_t, uint32_t) {}
  void drawCircle(int32_t, int32_t, int32_t, uint32_t) {}
  void fillCircle(int32_t, int32_t, int32_t, uint32_t) {}
  void fillTriangle(int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, uint32_t) {}
  void drawLine(int32_t, int32_t, int32_t, int32_t, uint32_t) {}
  void drawFastHLine(int32_t, int32_t, int32_t, uint32_t) {}
  void drawFastVLine(int32_t, int32_t, int32_t, uint32_t) {}
  void setCursor(int32_t, int32_t) {}
  void setRotation(uint8_t) {}
  void setTextColor(uint32_t) {}
  void setTextColor(uint32_t, uint32_t) {}
  void setTextFont(uint8_t) {}
  void setTextDatum(uint8_t) {}
  void drawString(const String&, int32_t, int32_t) {}
  void drawString(const char*, int32_t, int32_t) {}
  int textWidth(const String&) { return 0; }
  int textWidth(const char*) { return 0; }
  template<typename... A> void print(A&&...) {}
  template<typename... A> void println(A&&...) {}
  void printf(const char*, ...) {}
};

#endif // NISKALA_HAS_TFT
#endif // NISKALA_TFT_COMPAT_H
