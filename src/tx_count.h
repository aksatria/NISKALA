#ifndef TX_COUNT_H
#define TX_COUNT_H

#include <Arduino.h>
#include <esp_wifi.h>
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"

extern volatile uint32_t g_packetsSent;
extern volatile uint32_t g_txErrors;
extern portMUX_TYPE g_txMux;

void countTxResult(esp_err_t err);
esp_err_t sendWifiFrameAndCount(const uint8_t* frame, int len, bool enSys);
uint32_t getPacketsSent();
uint32_t getTxErrors();
void resetPacketCounters();

#endif
