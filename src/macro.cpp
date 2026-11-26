#include "macro.h"
#include "ui_helpers.h"
#include "state_machine.h"
#include "attack_engine.h"
#include "web_remote.h"
#if NISKALA_HAS_TFT
void drawMacroMenu() {
  tft.fillScreen(TFT_BLACK);
  drawHeader("MACRO", TFT_PURPLE);
  tft.setTextFont(2);
  for (int i = 0; i < MACRO_COUNT; i++) {
    int y = 36 + i * 40;
    if (i == macroSelection) {
      tft.fillRoundRect(8, y - 6, 224, 34, 6, TFT_DARKGREEN);
    }
    tft.setTextColor(i == macroSelection ? TFT_WHITE : TFT_DARKGREY, i == macroSelection ? TFT_DARKGREEN : TFT_BLACK);
    tft.setCursor(16, y);
    tft.print(MACRO_NAMES[i]);
    tft.setCursor(16, y + 18);
    tft.setTextFont(1);
    tft.setTextColor(i == macroSelection ? TFT_CYAN : TFT_DARKGREY, i == macroSelection ? TFT_DARKGREEN : TFT_BLACK);
    tft.print(MACRO_DESC[i]);
    tft.setTextFont(2);
  }
  drawFooter("ATAS: Pilih | BAWAH: Jalankan");
}
#else
void drawMacroMenu() {}

#endif

const char* macroStepName(uint8_t a) {
  switch (a) {
    case MACT_BEACON: return "Beacon Spam";
    case MACT_GLOBAL_DEAUTH: return "Global Deauth";
    case MACT_PROBE: return "Probe Sniff";
    case MACT_KARMA: return "Karma Attack";
    case MACT_BT_SPAM: return "BLE Spam";
    case MACT_EAPOL: return "EAPOL Capture";
    default: return "?";
  }
}

#if NISKALA_HAS_TFT
void drawMacroRunning(int frame) {
  (void)frame;
  const MacroStep* steps = (macroIndex == 0) ? MACRO_RECON : (macroIndex == 1 ? MACRO_ASSAULT : MACRO_HANDSHAKE);
  int stepCount = (macroIndex == 0) ? MACRO_RECON_STEPS : (macroIndex == 1 ? MACRO_ASSAULT_STEPS : MACRO_HANDSHAKE_STEPS);
  MacroStep s = steps[macroPhase];

  tft.fillScreen(TFT_BLACK);
  drawHeader("MACRO: " + String(MACRO_NAMES[macroIndex]), TFT_PURPLE);
  tft.setTextFont(2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(10, 34);
  tft.print("Phase "); tft.print(macroPhase + 1); tft.print("/"); tft.print(stepCount);
  tft.setCursor(10, 56);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.print(macroStepName(s.action));

  unsigned long elapsed = millis() - macroPhaseStart;
  unsigned long dur = s.durationMs;
  int pct = (dur > 0) ? (int)(elapsed * 100 / dur) : 0;
  if (pct > 100) pct = 100;
  tft.drawRect(10, 80, 220, 14, TFT_WHITE);
  tft.fillRect(12, 82, (int)(216 * pct / 100), 10, TFT_GREEN);

  tft.setCursor(10, 102);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.print("PKT: "); tft.print(getPacketsSent());
  tft.print("  H:"); tft.print(ESP.getFreeHeap());
  drawFooter("TOMBOL = HENTIKAN MACRO");
}
#else
void drawMacroRunning(int) {}

#endif

void macroStartStep(uint8_t a) {
  switch (a) {
    case MACT_BEACON: startBeaconSpam(); break;
    case MACT_GLOBAL_DEAUTH: startGlobalDeauth(); break;
    case MACT_PROBE: startProbeSniffer(); break;
    case MACT_KARMA: startKarmaScan(); break;
    case MACT_BT_SPAM: startBTSpam(); break;
    case MACT_EAPOL: startEapolCapture(); break;
  }
}

void macroStopStep(uint8_t a) {
  switch (a) {
    case MACT_BEACON: if (isBeaconRunning) stopBeaconSpam(); break;
    case MACT_GLOBAL_DEAUTH: if (isGlobalDeauthRunning) stopGlobalDeauth(); break;
    case MACT_PROBE: if (isProbeRunning) stopProbeSniffer(); break;
    case MACT_KARMA: if (isKarmaRunning) stopKarmaScan(); break;
    case MACT_BT_SPAM: if (isBTSpamRunning) stopBTSpam(); break;
    case MACT_EAPOL: if (isEapolRunning) stopEapolCapture(); break;
  }
}

void startMacro(int idx) {
  stopAllAttacks();
  macroIndex = idx;
  macroPhase = 0;
  macroPhaseStart = millis();
  isMacroRunning = true;
  currentState = MACRO_RUNNING;
  const MacroStep* steps = (idx == 0) ? MACRO_RECON : (idx == 1 ? MACRO_ASSAULT : MACRO_HANDSHAKE);
  macroStartStep(steps[0].action);
  drawMacroRunning(0);
  Serial.printf("[MACRO] Mulai %s\n", MACRO_NAMES[idx]);
}

void macroTick() {
  const MacroStep* steps = (macroIndex == 0) ? MACRO_RECON : (macroIndex == 1 ? MACRO_ASSAULT : MACRO_HANDSHAKE);
  int stepCount = (macroIndex == 0) ? MACRO_RECON_STEPS : (macroIndex == 1 ? MACRO_ASSAULT_STEPS : MACRO_HANDSHAKE_STEPS);
  MacroStep s = steps[macroPhase];

  if (millis() - macroPhaseStart >= s.durationMs) {
    macroStopStep(s.action);
    macroPhase++;
    if (macroPhase >= stepCount) {
      isMacroRunning = false;
      currentState = MAIN_MENU;
      drawMainMenu();
      Serial.println("[MACRO] Selesai");
      return;
    }
    macroPhaseStart = millis();
    macroStartStep(steps[macroPhase].action);
  }

  static unsigned long mLast = 0;
  static int mFrame = 0;
  if (millis() - mLast > 250) { drawMacroRunning(mFrame++); mLast = millis(); }
}

// ------------------------------------------------------------------
// Serial CLI
// ------------------------------------------------------------------
