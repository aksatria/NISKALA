#ifndef NISKALA_PACKET_MONITOR_H
#define NISKALA_PACKET_MONITOR_H

#include <Arduino.h>

#define PM_LOG_SIZE 32

struct PmEvent {
  uint8_t src[6];
  uint8_t dst[6];
  uint8_t reason;
  uint8_t channel;
  int8_t rssi;
  uint32_t ts;
  uint16_t count;
  bool isDisassoc;
};

extern PmEvent g_pmLog[PM_LOG_SIZE];
extern volatile int g_pmHead;
extern volatile int g_pmCount;
extern portMUX_TYPE g_pmMux;

// isDisassoc=true berarti frame disassociate, false berarti deauthentication.
void packetMonitorLog(bool isDisassoc, const uint8_t* src, const uint8_t* dst,
                      uint8_t reason, uint8_t channel, int8_t rssi);
String packetMonitorJson();
void packetMonitorClear();

#endif