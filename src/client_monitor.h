#ifndef NISKALA_CLIENT_MONITOR_H
#define NISKALA_CLIENT_MONITOR_H

#include <Arduino.h>

extern bool isClientMonitorRunning;
extern int clientSelection;
extern int clientScroll;
extern bool isWebRemoteRunning;
void applyPromiscuousFilter(bool probeOnly);

void drawClientMonitor();
void startClientMonitor();
void stopClientMonitor();

#endif