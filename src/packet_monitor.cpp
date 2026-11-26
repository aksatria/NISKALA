#include "packet_monitor.h"
#include "oui.h"

PmEvent g_pmLog[PM_LOG_SIZE];
volatile int g_pmHead = 0;
volatile int g_pmCount = 0;
portMUX_TYPE g_pmMux = portMUX_INITIALIZER_UNLOCKED;

static void macStr(const uint8_t* m, char out[18]) {
  snprintf(out, 18, "%02X:%02X:%02X:%02X:%02X:%02X",
           m[0], m[1], m[2], m[3], m[4], m[5]);
}

void packetMonitorLog(bool isDisassoc, const uint8_t* src, const uint8_t* dst,
                      uint8_t reason, uint8_t channel, int8_t rssi) {
  portENTER_CRITICAL(&g_pmMux);
  for (int i = 0; i < g_pmCount; i++) {
    int idx = (g_pmHead - g_pmCount + i + PM_LOG_SIZE) % PM_LOG_SIZE;
    PmEvent& e = g_pmLog[idx];
    if (memcmp(e.src, src, 6) == 0 && memcmp(e.dst, dst, 6) == 0 &&
        e.isDisassoc == isDisassoc && e.reason == reason) {
      e.count++;
      e.rssi = rssi;
      e.ts = millis();
      portEXIT_CRITICAL(&g_pmMux);
      return;
    }
  }

  PmEvent& e = g_pmLog[g_pmHead];
  memcpy(e.src, src, 6);
  memcpy(e.dst, dst, 6);
  e.reason = reason;
  e.channel = channel;
  e.rssi = rssi;
  e.ts = millis();
  e.count = 1;
  e.isDisassoc = isDisassoc;
  g_pmHead = (g_pmHead + 1) % PM_LOG_SIZE;
  if (g_pmCount < PM_LOG_SIZE) g_pmCount++;
  portEXIT_CRITICAL(&g_pmMux);
}

String packetMonitorJson() {
  String j = "{\"events\":[";
  portENTER_CRITICAL(&g_pmMux);
  int cnt = g_pmCount;
  int head = g_pmHead;
  PmEvent snap[PM_LOG_SIZE];
  for (int i = 0; i < cnt; i++) {
    int idx = (head - cnt + i + PM_LOG_SIZE) % PM_LOG_SIZE;
    snap[i] = g_pmLog[idx];
  }
  portEXIT_CRITICAL(&g_pmMux);

  for (int i = 0; i < cnt; i++) {
    char s[18], d[18];
    macStr(snap[i].src, s);
    macStr(snap[i].dst, d);
    if (i) j += ",";
    j += "{\"src\":\"" + String(s) + "\",\"dst\":\"" + String(d) +
         "\",\"reason\":" + String(snap[i].reason) +
         ",\"ch\":" + String(snap[i].channel) +
         ",\"rssi\":" + String(snap[i].rssi) +
         ",\"count\":" + String(snap[i].count) +
         ",\"type\":\"" + (snap[i].isDisassoc ? "disassoc" : "deauth") + "\"}";
  }
  j += "]}";
  return j;
}

void packetMonitorClear() {
  portENTER_CRITICAL(&g_pmMux);
  g_pmHead = 0;
  g_pmCount = 0;
  portEXIT_CRITICAL(&g_pmMux);
}