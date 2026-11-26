#include "wigle_upload.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <LittleFS.h>
#if __has_include("secrets.h")
  #include "secrets.h"
#endif

#ifndef NISKALA_WIGLE_API_NAME
  #define NISKALA_WIGLE_API_NAME ""
#endif
#ifndef NISKALA_WIGLE_API_TOKEN
  #define NISKALA_WIGLE_API_TOKEN ""
#endif

bool wigleUploadFile(const String& path) {
  if (NISKALA_WIGLE_API_NAME[0] == '\0' || NISKALA_WIGLE_API_TOKEN[0] == '\0') {
    Serial.println("[WIGLE] API credentials belum diisi di secrets.h");
    return false;
  }
  if (!LittleFS.exists(path)) {
    Serial.printf("[WIGLE] File tidak ditemukan: %s\n", path.c_str());
    return false;
  }

  File f = LittleFS.open(path, "r");
  if (!f) return false;
  String csv = f.readString();
  f.close();

  if (csv.length() == 0) return false;

  String boundary = "----NISKALA" + String(millis());
  String body = "--" + boundary + "\r\n";
  body += "Content-Disposition: form-data; name=\"file\"; filename=\"niskala.csv\"\r\n";
  body += "Content-Type: text/csv\r\n\r\n";
  body += csv;
  body += "\r\n--" + boundary + "--\r\n";

  HTTPClient http;
  http.setTimeout(20000);
  if (!http.begin("https://api.wigle.net/api/v2/file/upload")) {
    Serial.println("[WIGLE] begin failed");
    return false;
  }
  http.setAuthorization(NISKALA_WIGLE_API_NAME, NISKALA_WIGLE_API_TOKEN);
  http.addHeader("Accept", "application/json");
  http.addHeader("Content-Type", "multipart/form-data; boundary=" + boundary);

  int code = http.POST(body);
  Serial.printf("[WIGLE] Upload %s -> HTTP %d\n", path.c_str(), code);
  http.end();
  return code >= 200 && code < 300;
}