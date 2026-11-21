#ifndef NISKALA_TELEMETRY_H
#define NISKALA_TELEMETRY_H

#include <Arduino.h>
#include <esp_wifi.h>
#include "state_machine.h"
#include "attack_engine.h"

extern State currentState;
extern volatile AttackMode g_attackMode;
extern volatile uint32_t g_packetsSent;
extern volatile uint32_t g_txErrors;
extern TaskHandle_t attackTaskHandle;

int readBatteryPercent();
float readBatteryVoltage();
float temperatureRead();
String buildSystemStatusLine();
String buildTelemetryJson();

#endif