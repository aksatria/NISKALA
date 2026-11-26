#ifndef NISKALA_BOARD_HAL_H
#define NISKALA_BOARD_HAL_H

#include <Arduino.h>
#include "board_config.h"

// Abstraksi tombol agar board tanpa tombol/layar (mis. ESP32 generic)
// bisa berjalan via Serial CLI / Web Remote tanpa input fisik.

inline bool btnTopPressed() {
#if NISKALA_HAS_TFT
  return digitalRead(BTN_TOP) == LOW;
#else
  return false;
#endif
}

inline bool btnBottomPressed() {
#if NISKALA_HAS_TFT
  return digitalRead(BTN_BOTTOM) == LOW;
#else
  return false;
#endif
}

#endif