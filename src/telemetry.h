#ifndef NISKALA_TELEMETRY_H
#define NISKALA_TELEMETRY_H

#include <Arduino.h>
#include <esp_wifi.h>

extern State currentState;
extern volatile AttackMode g_attackMode;
extern volatile uint32_t g_packetsSent;
extern volatile uint32_t g_txErrors;
extern TaskHandle_t attackTaskHandle;

int readBatteryPercent();
float temperatureRead();

inline String buildSystemStatusLine() {
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

#endif