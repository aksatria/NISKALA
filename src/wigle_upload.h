#ifndef NISKALA_WIGLE_UPLOAD_H
#define NISKALA_WIGLE_UPLOAD_H

#include <Arduino.h>

// Upload file CSV (path di LittleFS) ke WiGLE.net.
// Board harus sudah terhubung ke WiFi dengan internet (STA).
bool wigleUploadFile(const String& path);

#endif