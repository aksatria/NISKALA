#ifndef NISKALA_WEB_UTILS_H
#define NISKALA_WEB_UTILS_H

#include <Arduino.h>

inline String jsonEscape(const String& s) {
  String out;
  out.reserve(s.length() + 8);
  for (unsigned int i = 0; i < s.length(); i++) {
    char c = s[i];
    if (c == '\\') out += "\\\\";
    else if (c == '"') out += "\\\"";
    else if (c == '\n') out += "\\n";
    else if (c == '\r') out += "\\r";
    else if (c == '\t') out += "\\t";
    else if ((unsigned char)c < 0x20) {
      char buf[8];
      snprintf(buf, sizeof(buf), "\\u%04x", (unsigned char)c);
      out += buf;
    } else out += c;
  }
  return out;
}

inline String csvField(const String& s) {
  String out = "\"";
  for (unsigned int i = 0; i < s.length(); i++) {
    if (s[i] == '"') out += "\"\"";
    else out += s[i];
  }
  out += "\"";
  return out;
}

#endif