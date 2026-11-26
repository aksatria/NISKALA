#include "script_runner.h"
#include "attack_control.h"
#include "eapol.h"
#include "bt_spam.h"
#include "state_machine.h"
#include "web_remote.h"

static bool startAction(const String& a) {
  if (a == "beacon") startBeaconSpam();
  else if (a == "global") startGlobalDeauth();
  else if (a == "probe") startProbeSniffer();
  else if (a == "karma") startKarmaScan();
  else if (a == "blespam") startBTSpam();
  else if (a == "eapol") startEapolCapture();
  else return false;
  return true;
}

static void stopAction(const String& a) {
  if (a == "beacon") stopBeaconSpam();
  else if (a == "global") stopGlobalDeauth();
  else if (a == "probe") stopProbeSniffer();
  else if (a == "karma") stopKarmaScan();
  else if (a == "blespam") stopBTSpam();
  else if (a == "eapol") stopEapolCapture();
}

bool runAttackScript(const String& script) {
  String s = script;
  s.trim();
  if (s.length() == 0) {
    Serial.println("[SCRIPT] Kosong. Contoh: script \"beacon:5000,global:3000\"");
    return false;
  }

  bool ok = true;
  int pos = 0;
  while (pos < s.length()) {
    int comma = s.indexOf(',', pos);
    if (comma < 0) comma = s.length();
    String step = s.substring(pos, comma);
    step.trim();
    pos = comma + 1;

    int colon = step.indexOf(':');
    if (colon <= 0) {
      Serial.printf("[SCRIPT] Step tidak valid: %s\n", step.c_str());
      ok = false;
      continue;
    }
    String action = step.substring(0, colon);
    action.trim();
    action.toLowerCase();
    unsigned long ms = (unsigned long)step.substring(colon + 1).toInt();
    if (ms > 3600000UL) ms = 3600000UL;

    if (!startAction(action)) {
      Serial.printf("[SCRIPT] Aksi tidak dikenal: %s\n", action.c_str());
      ok = false;
      continue;
    }
    Serial.printf("[SCRIPT] %s %lums\n", action.c_str(), ms);
    delay(ms);
    stopAction(action);
  }

  stopAllAttacks();
  Serial.println(ok ? "[SCRIPT] Selesai" : "[SCRIPT] Selesai (ada step gagal)");
  return ok;
}