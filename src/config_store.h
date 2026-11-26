#ifndef NISKALA_CONFIG_STORE_H
#define NISKALA_CONFIG_STORE_H

#include <Arduino.h>
#include <Preferences.h>

extern Preferences prefs;
extern int cfg_deauth_burst;
extern int cfg_auth_burst;
extern int cfg_hop_interval_ms;
extern int cfg_beacon_hop_ms;
extern int cfg_deauth_reason;
extern String cfg_wifi_ssid;
extern String cfg_wifi_pass;
extern String cfg_server_url;
extern String cfg_template_url;
extern int cfg_autosync_min;

void savePasswordToNVS(String email, String pass);
void deletePasswordEntry(int idx);
void updatePasswordVerification(bool ok);
void saveConfig();

#endif