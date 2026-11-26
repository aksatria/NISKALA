#ifndef NISKALA_SCRIPT_RUNNER_H
#define NISKALA_SCRIPT_RUNNER_H

#include <Arduino.h>

// Jalankan rangkaian serangan sederhana dari CLI:
//   script "beacon:5000,global:3000,probe:4000"
// Aksi yang didukung: beacon, global, probe, karma, blespam, eapol, deauth
bool runAttackScript(const String& script);

#endif