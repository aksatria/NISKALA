#include "telemetry.h"
#include "state_machine.h"
#include "attack_engine.h"
#include "pins.h"
#include <WiFi.h>
#include <stdarg.h>
#include <string.h>

int readBatteryPercent() {
  float v = (float)analogReadMilliVolts(BAT_PIN) / 1000.0f * 2.0f;
  int pct = (int)((v - 3.3f) / (4.2f - 3.3f) * 100.0f);
  if (pct < 0) pct = 0;
  if (pct > 100) pct = 100;
  return pct;
}

float readBatteryVoltage() {
  return (float)analogReadMilliVolts(BAT_PIN) / 1000.0f * 2.0f;
}

// ---------------------------------------------------------------------------
// Event log ring buffer
// ---------------------------------------------------------------------------
#define EVENT_LOG_SIZE 32
#define EVENT_LOG_LEN  52

static char g_eventLog[EVENT_LOG_SIZE][EVENT_LOG_LEN];
static volatile int g_eventHead = 0;
static volatile int g_eventCount = 0;
static portMUX_TYPE g_eventMux = portMUX_INITIALIZER_UNLOCKED;

void logEvent(const char* fmt, ...) {
  char tmp[EVENT_LOG_LEN];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(tmp, sizeof(tmp), fmt, ap);
  va_end(ap);

  portENTER_CRITICAL(&g_eventMux);
  strncpy(g_eventLog[g_eventHead], tmp, EVENT_LOG_LEN - 1);
  g_eventLog[g_eventHead][EVENT_LOG_LEN - 1] = 0;
  g_eventHead = (g_eventHead + 1) % EVENT_LOG_SIZE;
  if (g_eventCount < EVENT_LOG_SIZE) g_eventCount++;
  portEXIT_CRITICAL(&g_eventMux);
}

String dumpEventLog() {
  static char buf[EVENT_LOG_SIZE * (EVENT_LOG_LEN + 1)];
  buf[0] = 0;
  size_t off = 0;

  portENTER_CRITICAL(&g_eventMux);
  int start = (g_eventCount < EVENT_LOG_SIZE) ? 0 : g_eventHead;
  int n = g_eventCount;
  for (int i = 0; i < n && off + EVENT_LOG_LEN + 2 < sizeof(buf); i++) {
    int idx = (start + i) % EVENT_LOG_SIZE;
    if (i) buf[off++] = '\n';
    off += snprintf(buf + off, sizeof(buf) - off, "%s", g_eventLog[idx]);
  }
  portEXIT_CRITICAL(&g_eventMux);

  return String(buf);
}

String buildSystemStatusLine() {
  uint8_t ch = 0;
  wifi_second_chan_t sec;
  esp_wifi_get_channel(&ch, &sec);

  char buf[192];
  snprintf(buf, sizeof(buf),
           "State=%d Mode=%d PKT=%u ERR=%u Heap=%u Min=%u Bat=%d Temp=%.0f Stack=%u Ch=%d Up=%lus",
           (int)currentState,
           (int)getAttackMode(),
           (unsigned)getPacketsSent(),
           (unsigned)getTxErrors(),
           (unsigned)ESP.getFreeHeap(),
           (unsigned)ESP.getMinFreeHeap(),
           readBatteryPercent(),
           temperatureRead(),
           attackTaskHandle ? (unsigned)uxTaskGetStackHighWaterMark(attackTaskHandle) : 0,
           (int)ch,
           (unsigned long)(millis() / 1000));
  return String(buf);
}

String buildTelemetryJson() {
  uint8_t ch = 0;
  wifi_second_chan_t sec;
  esp_wifi_get_channel(&ch, &sec);

  String j = "{";
  j += "\"state\":" + String((int)currentState);
  j += ",\"mode\":" + String((int)getAttackMode());
  j += ",\"packets\":" + String((uint32_t)getPacketsSent());
  j += ",\"tx_errors\":" + String((uint32_t)getTxErrors());
  j += ",\"heap\":" + String((uint32_t)ESP.getFreeHeap());
  j += ",\"heap_min\":" + String((uint32_t)ESP.getMinFreeHeap());
  j += ",\"battery\":" + String(readBatteryPercent());
  j += ",\"battery_v\":" + String(readBatteryVoltage(), 2);
  j += ",\"temp\":" + String((int)temperatureRead());
  j += ",\"uptime_s\":" + String((unsigned long)(millis() / 1000));
  j += ",\"stack\":" + String(attackTaskHandle ? (unsigned)uxTaskGetStackHighWaterMark(attackTaskHandle) : 0);
  j += ",\"wifi_mode\":" + String((int)WiFi.getMode());
  j += ",\"wifi_channel\":" + String((int)ch);
  j += "}";
  return j;
}
