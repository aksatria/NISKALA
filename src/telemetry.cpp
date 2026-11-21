#include "telemetry.h"
#include "state_machine.h"
#include "attack_engine.h"

#define BAT_PIN 34

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

String buildSystemStatusLine() {
  char buf[160];
  snprintf(buf, sizeof(buf),
           "State=%d Mode=%d PKT=%u ERR=%u Heap=%u Bat=%d Temp=%.0f Stack=%u Uptime=%lu",
           (int)currentState,
           (int)g_attackMode,
           (unsigned)g_packetsSent,
           (unsigned)g_txErrors,
           (unsigned)ESP.getFreeHeap(),
           readBatteryPercent(),
           temperatureRead(),
           attackTaskHandle ? (unsigned)uxTaskGetStackHighWaterMark(attackTaskHandle) : 0,
           (unsigned long)(millis() / 1000));
  return String(buf);
}

String buildTelemetryJson() {
  String j = "{";
  j += "\"state\":" + String((int)currentState);
  j += ",\"mode\":" + String((int)g_attackMode);
  j += ",\"packets\":" + String((uint32_t)g_packetsSent);
  j += ",\"tx_errors\":" + String((uint32_t)g_txErrors);
  j += ",\"heap\":" + String((uint32_t)ESP.getFreeHeap());
  j += ",\"battery\":" + String(readBatteryPercent());
  j += ",\"battery_v\":" + String(readBatteryVoltage(), 2);
  j += ",\"temp\":" + String((int)temperatureRead());
  j += ",\"uptime_s\":" + String((unsigned long)(millis() / 1000));
  j += ",\"stack\":" + String(attackTaskHandle ? (unsigned)uxTaskGetStackHighWaterMark(attackTaskHandle) : 0);
  j += "}";
  return j;
}