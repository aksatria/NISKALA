#ifndef NISKALA_BOARD_CONFIG_H
#define NISKALA_BOARD_CONFIG_H

// =============================================================================
// Konfigurasi board terpusat.
//
// Cara memilih board:
//   1. Lewat build_flags PlatformIO, contoh: -DBOARD_TTGO_T_DISPLAY=1
//   2. Atau edit file ini (hapus komentar pada salah satu #define).
//
// Default saat ini tetap TTGO T-Display agar kompatibel dengan build lama.
// =============================================================================

#if !defined(BOARD_TTGO_T_DISPLAY) && !defined(BOARD_ESP32_GENERIC) && !defined(BOARD_ESP32_OLED)
  #define BOARD_TTGO_T_DISPLAY 1
#endif

// -----------------------------------------------------------------------------
// TTGO T-Display (ESP32 + ST7789 135x240 + 2 tombol)
// -----------------------------------------------------------------------------
#if defined(BOARD_TTGO_T_DISPLAY)
  #define NISKALA_HAS_TFT   1
  #define BTN_TOP           0
  #define BTN_BOTTOM        35
  #define BAT_PIN           34
  #define BOARD_NAME        "TTGO T-Display"
#endif

// -----------------------------------------------------------------------------
// ESP32 generic tanpa layar (kendali via Web Remote / Serial CLI)
// Catatan: belum otomatis menonaktifkan seluruh kode TFT. Gunakan hanya
// sebagai penanda target portabilitas berikutnya.
// -----------------------------------------------------------------------------
#if defined(BOARD_ESP32_GENERIC)
  #define NISKALA_HAS_TFT   0
  #define BTN_TOP           0
  #define BTN_BOTTOM        0
  #define BAT_PIN           34
  #define BOARD_NAME        "ESP32 Generic"
#endif

// -----------------------------------------------------------------------------
// ESP32 + OLED SSD1306/SH1106 (fallback display)
// -----------------------------------------------------------------------------
#if defined(BOARD_ESP32_OLED)
  #define NISKALA_HAS_TFT   0
  #define BTN_TOP           0
  #define BTN_BOTTOM        35
  #define BAT_PIN           34
  #define BOARD_NAME        "ESP32 OLED"
#endif

// Validasi: board harus sudah terdefinisi
#ifndef BOARD_NAME
  #error "Board NISKALA belum dikonfigurasi. Pilih salah satu: BOARD_TTGO_T_DISPLAY, BOARD_ESP32_GENERIC, BOARD_ESP32_OLED"
#endif

#endif // NISKALA_BOARD_CONFIG_H