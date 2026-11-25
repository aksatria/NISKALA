#include "ui_screens.h"
#include "ui_helpers.h"
#include "state_machine.h"
#include "attack_engine.h"
#include "karma.h"
#include "web_remote.h"
#include "cloud.h"
#include "ble_scan.h"
#include "recon.h"
#include "macro.h"
#include "client_monitor.h"
#include "telemetry.h"
#include "ui_icons.h"
#include "templates.h"
#include "pins.h"

#define MAX_VERIFY_ATTEMPTS 5

extern int menuSelection;
extern int menuScrollPos;
extern const int MENU_COUNT;
extern const char* menuItems[];

extern int selectedIdx;
extern int apCount;
extern int templateSelection;
extern int evilTwinModeSelection;
extern bool g_evilTwinCoop;

extern bool isDeauthRunning;
extern bool isReleasing;
extern String capturedEmail;
extern String capturedPass;
extern unsigned long releaseTimer;
extern bool isVerifying;

extern int viewSelection;
extern bool confirmDeleteAll;

extern unsigned long karmaScanStart;
extern int karmaScrollPos;
extern int karmaSelection;

void drawGlowText(String text, int x, int y, uint16_t mainColor, uint16_t glowColor) {
  tft.setTextColor(glowColor);
  tft.drawString(text, x+1, y); tft.drawString(text, x-1, y);
  tft.drawString(text, x, y+1); tft.drawString(text, x, y-1);
  tft.setTextColor(mainColor);
  tft.drawString(text, x, y);
}

void drawCornerBrackets(int x, int y, int w, int h, uint16_t color) {
  int len = 5;
  tft.drawLine(x, y, x+len, y, color); tft.drawLine(x, y, x, y+len, color);
  tft.drawLine(x+w-len, y, x+w, y, color); tft.drawLine(x+w, y, x+w, y+len, color);
  tft.drawLine(x, y+h-len, x, y+h, color); tft.drawLine(x, y+h, x+len, y+h, color);
  tft.drawLine(x+w-len, y+h, x+w, y+h, color); tft.drawLine(x+w, y+h-len, x+w, y+h, color);
}

// ═══════════════════════════════════════════════
// Splash & scan
// ═══════════════════════════════════════════════
void drawElegantSplash() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextDatum(MC_DATUM);
  
  tft.drawFastHLine(30, 10, 180, TFT_DARKCYAN);
  tft.drawFastHLine(60, 12, 120, TFT_DARKCYAN);
  
  tft.setTextFont(4);
  drawGlowText("N I S K A L A", 120, 35, TFT_GREEN, TFT_DARKGREEN);
  
  tft.setTextFont(1); 
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawString("To be invisible is not to be absent", 120, 55);
  
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawString("Etika > Skill | Adab > Ilmu", 120, 68);
  
  tft.drawFastHLine(30, 78, 180, TFT_DARKCYAN);
  tft.drawFastHLine(60, 80, 120, TFT_DARKCYAN);

  int barX = 40, barY = 95, barW = 160, barH = 10;
  tft.drawRect(barX, barY, barW, barH, TFT_WHITE);
  
  for (int p = 0; p <= 100; p += 4) {
    tft.fillRect(barX+1, barY+1, (p*(barW-2))/100, barH-2, TFT_GREEN);
    
    tft.setTextFont(1); 
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString("Booting " + String(p) + "%", 120, 85);
    delay(15); 
  }
  
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawString("System Ready", 120, 115);
  
  delay(400); 
  tft.setTextDatum(TL_DATUM);
}

void drawScanAnimation(int frame) {
  tft.fillScreen(TFT_BLACK);
  drawHeader("SCANNING WIFI...", TFT_DARKGREEN);
  tft.setTextFont(2); tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setCursor(10, 32);
  tft.print("Mencari jaringan");
  for (int i = 0; i < (frame % 4); i++) tft.print(".");
  int cx = 120, cy = 78;
  for (int i = 0; i < 3; i++) {
    int r = (frame * 3 + i * 18) % 55;
    if (r > 3) tft.drawCircle(cx, cy, r, (r < 25) ? TFT_GREEN : TFT_DARKGREEN);
  }
  tft.fillCircle(cx, cy, 4, (frame % 6 < 3) ? TFT_GREEN : TFT_YELLOW);
  drawFooter("TEKAN TOMBOL = BATAL");
}

// ═══════════════════════════════════════════════
// Main menu
// ═══════════════════════════════════════════════
void drawMainMenu() {
  tft.fillScreen(TFT_BLACK);
  String pwdCount = String(prefs.getInt("pwd_count", 0));
  String statusRight = "BAT:" + String(readBatteryPercent()) + "% T:" + String((int)temperatureRead()) + "C P:" + pwdCount;

  // Header
  tft.fillRect(0, 0, 240, 24, TFT_DARKGREEN);
  tft.setTextDatum(ML_DATUM); tft.setTextFont(2);
  tft.setTextColor(TFT_WHITE, TFT_DARKGREEN);
  tft.drawString("NISKALA", 8, 12);
  tft.setTextDatum(MR_DATUM); tft.setTextFont(1);
  tft.setTextColor(TFT_CYAN, TFT_DARKGREEN);
  tft.drawString(statusRight, 232, 12);
  tft.drawFastHLine(0, 24, 240, TFT_CYAN);
  tft.setTextDatum(TL_DATUM);

  // Satu menu per layar (font besar)
  int idx = menuSelection;
  // Ikon modul (tanpa bingkai siku)
  drawMenuIcon(idx, 112, 48, TFT_GREEN);

  tft.setTextDatum(MC_DATUM);
  String label = String(menuItems[idx]);
  tft.setTextFont(4);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.drawString(label, 120, 86);

  tft.setTextFont(2);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawString(String(idx + 1) + " / " + String(MENU_COUNT), 120, 122);
  tft.setTextDatum(TL_DATUM);

  drawFooter("ATAS: Pilih | BAWAH: OK");
}

// ═══════════════════════════════════════════════
// AP list
// ═══════════════════════════════════════════════
void drawSignalBars(int cx, int y, int rssi, uint16_t color) {
  int pct = 0;
  if (rssi > -60) pct = 4;
  else if (rssi > -70) pct = 3;
  else if (rssi > -80) pct = 2;
  else if (rssi > -90) pct = 1;
  for (int i = 0; i < 4; i++) {
    int bh = 5 + i * 4;
    int bx = cx - 14 + i * 8;
    uint16_t c = (i < pct) ? color : TFT_DARKGREY;
    tft.fillRect(bx, y + 20 - bh, 6, bh, c);
  }
}

void drawBackIcon(int cx, int cy, uint16_t color) {
  tft.drawLine(cx + 8, cy - 6, cx - 4, cy, color);
  tft.drawLine(cx - 4, cy, cx + 8, cy + 6, color);
  tft.drawFastHLine(cx - 8, cy, 16, color);
}

void drawTemplateIconBig(int idx, int cx, int cy, uint16_t color) {
  if (idx == 5) {
    tft.drawCircle(cx - 7, cy + 3, 6, color);
    tft.drawCircle(cx + 1, cy - 1, 7, color);
    tft.drawCircle(cx + 9, cy + 3, 6, color);
    tft.drawFastHLine(cx - 8, cy + 8, 16, color);
  } else {
    tft.drawRect(cx - 9, cy - 7, 18, 14, color);
    tft.drawFastHLine(cx - 9, cy - 3, 18, color);
    tft.drawFastHLine(cx - 9, cy + 1, 18, color);
  }
}

void drawAPList() {
  tft.fillScreen(TFT_BLACK);
  drawHeader("PILIH TARGET AP", TFT_NAVY);

  int idx = selectedIdx;
  tft.setTextDatum(MC_DATUM);

  if (idx < apCount) {
    int rssi = WiFi.RSSI(idx);
    drawSignalBars(120, 30, rssi, TFT_CYAN);

    String ssid = WiFi.SSID(idx);
    bool hidden = (ssid.length() == 0);
    if (hidden) {
      ssid = "(HIDDEN)";
    } else if (ssid.length() > 12) {
      ssid = ssid.substring(0, 12);
    }

    tft.setTextFont(4);
    tft.setTextColor(hidden ? TFT_ORANGE : TFT_CYAN, TFT_BLACK);
    tft.drawString(ssid, 120, 58);

    tft.setTextFont(2);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.drawString("CH " + String(WiFi.channel(idx)) + "  |  " + String(rssi) + " dBm", 120, 84);

    tft.setTextFont(1);
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.drawString(WiFi.BSSIDstr(idx), 120, 104);
  } else {
    drawBackIcon(110, 46, TFT_GREEN);

    tft.setTextFont(4);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.drawString("KEMBALI", 120, 68);

    tft.setTextFont(2);
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.drawString("ke menu utama", 120, 100);
  }

  tft.setTextDatum(TL_DATUM);
  drawFooter("ATAS: Pilih | BAWAH: OK");
}

// ═══════════════════════════════════════════════
// Template select
// ═══════════════════════════════════════════════
void drawEvilTwinModeSelect() {
  tft.fillScreen(TFT_BLACK);
  drawHeader("MODE EVIL TWIN", TFT_DARKCYAN);

  int mode = evilTwinModeSelection;
  const char* names[2] = { "1 BOARD", "2 BOARD" };
  const char* desc[2] = {
    "Deauth + Clone MAC sendiri",
    "Tanpa deauth, tanpa clone MAC"
  };

  // dua kartu pilihan
  for (int i = 0; i < 2; i++) {
    int y = 36 + i * 40;
    bool sel = (i == mode);
    uint16_t border = sel ? TFT_GREEN : TFT_DARKGREY;
    uint16_t txtCol = sel ? TFT_WHITE : TFT_DARKGREY;

    drawCornerBrackets(12, y, 216, 34, border);
    tft.setTextDatum(ML_DATUM);
    tft.setTextFont(2);
    tft.setTextColor(sel ? TFT_GREEN : TFT_WHITE, TFT_BLACK);
    tft.drawString(names[i], 24, y + 10);
    tft.setTextFont(1);
    tft.setTextColor(txtCol, TFT_BLACK);
    tft.drawString(desc[i], 24, y + 26);
  }

  tft.setTextDatum(TL_DATUM);
  drawFooter("ATAS: Pilih | BAWAH: OK");
}

void drawTemplateSelect() {
  tft.fillScreen(TFT_BLACK);
  drawHeader("PILIH TEMPLATE PORTAL", TFT_DARKCYAN);

  int idx = templateSelection;
  drawTemplateIconBig(idx, 120, 36, TFT_CYAN);

  tft.setTextDatum(MC_DATUM);
  tft.setTextFont(4);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawString(getTemplateName(idx), 120, 68);

  tft.setTextFont(2);
  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.drawString(String(idx + 1) + " / " + String(TEMPLATE_COUNT), 120, 100);

  tft.setTextDatum(TL_DATUM);
  drawFooter("ATAS: Pilih | BAWAH: OK");
}

// ═══════════════════════════════════════════════
// Deauth menu
// ═══════════════════════════════════════════════
void drawDeauthMenu() {
  tft.fillScreen(TFT_BLACK);
  drawHeader("DEAUTH MANUAL", TFT_MAROON);
  tft.setTextFont(2); tft.setTextColor(TFT_WHITE, TFT_BLACK);

  tft.setCursor(10, 26); tft.print("Target: ");
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  String s = targetSSID; if (s.length() > 16) s = s.substring(0, 16);
  tft.println(s);

  tft.setCursor(10, 46); tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.print("Status: ");
  if (isDeauthRunning) { tft.setTextColor(TFT_GREEN, TFT_BLACK); tft.println("MENEMBAK"); }
  else { tft.setTextColor(TFT_RED, TFT_BLACK); tft.println("BERHENTI"); }

  tft.setCursor(10, 66); tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.print("Paket: ");
  tft.setTextColor(TFT_YELLOW, TFT_BLACK); tft.println(getPacketsSent());

  tft.setCursor(10, 86); tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.print("Error: ");
  tft.setTextColor(getTxErrors() > 0 ? TFT_RED : TFT_DARKGREY, TFT_BLACK);
  tft.println(getTxErrors());

  drawFooter("BAWAH: Start/Stop | ATAS: Keluar");
}

void drawAuthFlood(int frame) {
  tft.fillScreen(TFT_BLACK);
  drawHeader("AUTH FLOOD", TFT_MAROON);
  tft.setTextFont(2); tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(10, 26); tft.print("Target: ");
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  String s = targetSSID; if (s.length() > 16) s = s.substring(0, 16);
  tft.println(s);
  tft.setCursor(10, 48); tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.print("Paket: ");
  tft.setTextColor(TFT_YELLOW, TFT_BLACK); tft.println(getPacketsSent());
  tft.setCursor(10, 70); tft.setTextColor(TFT_RED, TFT_BLACK);
  tft.print("Flooding auth request");
  for (int i = 0; i < (frame % 4); i++) tft.print(".");
  int cx = 200, cy = 88;
  for (int i = 0; i < 3; i++) {
    int r = (frame * 4 + i * 12) % 28;
    if (r > 2) tft.drawCircle(cx, cy, r, r < 14 ? TFT_RED : TFT_MAROON);
  }
  drawFooter("TEKAN TOMBOL = STOP");
}

void drawGlobalDeauth(int frame) {
  tft.fillScreen(TFT_BLACK);
  drawHeader("GLOBAL DEAUTH", TFT_RED);
  tft.setTextFont(2); tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(10, 30); tft.print("Mode: ");
  tft.setTextColor(TFT_YELLOW, TFT_BLACK); tft.println("CHANNEL HOP");
  tft.setCursor(10, 52); tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.print("Paket: ");
  tft.setTextColor(TFT_GREEN, TFT_BLACK); tft.println(getPacketsSent());
  int cx = 120, cy = 88;
  for (int i = 0; i < 3; i++) {
    int r = (frame * 4 + i * 15) % 30;
    if (r > 2) tft.drawCircle(cx, cy, r, r < 15 ? TFT_RED : TFT_MAROON);
  }
  tft.fillCircle(cx, cy, 3, TFT_YELLOW);
  drawFooter("TEKAN TOMBOL = STOP");
}

void drawBeaconSpam(int frame) {
  tft.fillScreen(TFT_BLACK);
  drawHeader("BEACON SPAM", TFT_PURPLE);
  tft.setTextFont(2); tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(10, 30); tft.print("SSID pool: ");
  tft.setTextColor(TFT_CYAN, TFT_BLACK); tft.println(SPAM_COUNT);
  tft.setCursor(10, 52); tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.print("Beacon: ");
  tft.setTextColor(TFT_YELLOW, TFT_BLACK); tft.println(getPacketsSent());
  tft.setCursor(10, 74); tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.print("Broadcasting");
  for (int i = 0; i < (frame % 4); i++) tft.print(".");
  int cx = 200, cy = 88;
  for (int i = 0; i < 3; i++) {
    int r = (frame * 3 + i * 8) % 25;
    if (r > 2) tft.drawCircle(cx, cy, r, r < 12 ? TFT_PURPLE : TFT_MAGENTA);
  }
  tft.fillCircle(cx, cy, 2, TFT_PINK);
  drawFooter("TEKAN TOMBOL = STOP");
}

// [FIX] snapshot probe log dengan critical section
const char* vendorFromMacBytes(const uint8_t* m) {
  uint32_t oui = ((uint32_t)m[0] << 16) | ((uint32_t)m[1] << 8) | m[2];
  switch (oui) {
    case 0xACBC32: case 0x8C8590: case 0xF01898: case 0x308D99: case 0x3C22FB:
      return "Apple";
    case 0x8C0D76: case 0xCC07AB: case 0x3C64EA:
      return "Samsung";
    case 0x640980: case 0x7811DC: case 0xF0B429: case 0x8C4B14:
      return "Xiaomi";
    case 0x00E0FC: case 0x286ED4: case 0xD80BCB:
      return "Huawei";
    case 0x546009: case 0x3C5AB4:
      return "Google";
    case 0x0050F2: case 0x2816A8:
      return "Microsoft";
    case 0x001302: case 0x340286:
      return "Intel";
    case 0x50C7BF: case 0xA42BB0:
      return "TP-Link";
    case 0x502B98: case 0x086438:
      return "Realme";
    case 0x08D46A: case 0x702C1F:
      return "Oppo";
    case 0x241177: case 0x7802F8:
      return "Vivo";
    case 0x0024BE: case 0x405BAD:
      return "Sony";
    case 0x0C8BFD: case 0xE05B70:
      return "OnePlus";
    case 0x0015EB: case 0x286C07:
      return "ZTE";
    default:
      return "Unknown";
  }
}

const char* vendorFromMacStr(const char* macStr) {
  int a[6] = {0};
  sscanf(macStr, "%x:%x:%x:%x:%x:%x", &a[0], &a[1], &a[2], &a[3], &a[4], &a[5]);
  uint8_t m[6] = {(uint8_t)a[0], (uint8_t)a[1], (uint8_t)a[2], (uint8_t)a[3], (uint8_t)a[4], (uint8_t)a[5]};
  return vendorFromMacBytes(m);
}

void drawProbeSniff(int frame) {
  tft.fillScreen(TFT_BLACK);
  drawHeader("PROBE SNIFFER", TFT_DARKCYAN);
  tft.setTextFont(1); tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setCursor(4, 28); tft.print("VENDOR    HIT  RSSI  SSID");

  ProbeEntry snapshot[4];
  int count, show;
  portENTER_CRITICAL(&g_probeLogMux);
  count = g_probeLogCount;
  int head = g_probeLogHead;
  show = (count < 4) ? count : 4;
  for (int i = 0; i < show; i++) {
    int idx = (head - show + i + PROBE_LOG_SIZE) % PROBE_LOG_SIZE;
    snapshot[i] = g_probeLog[idx];
  }
  portEXIT_CRITICAL(&g_probeLogMux);

  for (int i = 0; i < show; i++) {
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(4, 40 + (i * 18));
    char buf[64];
    snprintf(buf, sizeof(buf), "%-8.8s %3d %4d  %s",
      vendorFromMacBytes(snapshot[i].mac),
      snapshot[i].count,
      snapshot[i].rssi,
      snapshot[i].ssid[0] ? snapshot[i].ssid : "(hidden)");
    tft.print(buf);
  }
  if (count == 0) {
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.setCursor(4, 45); tft.print("Menunggu probe");
    for (int i = 0; i < (frame % 4); i++) tft.print(".");
  }
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setCursor(4, 108); tft.print("Total: " + String(count));
  drawFooter("TEKAN TOMBOL = STOP");
}

void drawBTSpam(int frame) {
  tft.fillScreen(TFT_BLACK);
  drawHeader("BLUETOOTH SPAM", TFT_BLUE);
  tft.setTextFont(2); tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(10, 30); tft.print("Mode: ");
  tft.setTextColor(TFT_CYAN, TFT_BLACK); tft.println("MULTI VENDOR");
  tft.setCursor(10, 52); tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.print("Payload: ");
  tft.setTextColor(TFT_YELLOW, TFT_BLACK); tft.println("8 rotasi");
  tft.setCursor(10, 74); tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.print("Broadcasting BLE");
  for (int i = 0; i < (frame % 4); i++) tft.print(".");
  int bx = 200, by = 75;
  tft.drawLine(bx, by, bx, by+20, TFT_BLUE);
  tft.drawLine(bx, by+10, bx+8, by+5, TFT_BLUE);
  tft.drawLine(bx+8, by+5, bx+8, by+15, TFT_BLUE);
  tft.drawLine(bx+8, by+15, bx, by+10, TFT_BLUE);
  if (frame % 4 < 2) tft.fillCircle(bx+14, by+10, 2, TFT_CYAN);
  drawFooter("TEKAN TOMBOL = STOP");
}

// ═══════════════════════════════════════════════
// CSA UI  [PATCH 2]
// ═══════════════════════════════════════════════
void drawCSARunning(int frame) {
  tft.fillScreen(TFT_BLACK);
  drawHeader("CSA ATTACK", TFT_ORANGE);
  tft.setTextFont(2); tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(10, 26); tft.print("Target: ");
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  String s = targetSSID; if (s.length() > 16) s = s.substring(0, 16);
  tft.println(s);

  tft.setCursor(10, 46); tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.print("Ch asli: ");
  tft.setTextColor(TFT_YELLOW, TFT_BLACK); tft.print(g_targetChannel);

  tft.setCursor(10, 66); tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.print("Paket: ");
  tft.setTextColor(TFT_GREEN, TFT_BLACK); tft.println(getPacketsSent());

  tft.setCursor(10, 86); tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.print("Korban: ");
  if (isVictimFound()) {
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    char buf[24];
    uint8_t vm[6]; getVictimMac(vm);
    snprintf(buf, sizeof(buf), "%02X:%02X:%02X...",
             vm[0], vm[1], vm[2]);
    tft.println(buf);
  } else {
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.println("scanning");
  }

  int cx = 205, cy = 90;
  const char* spin = "|/-\\";
  tft.setTextColor(TFT_ORANGE, TFT_BLACK);
  tft.setCursor(190, 90);
  tft.print("CH ");
  tft.print(spin[frame % 4]);

  drawFooter("TEKAN TOMBOL = STOP");
}

int drawWrapped(String text, int x, int y, int maxW, uint16_t color, int lineH) {
  tft.setTextFont(2);
  tft.setTextColor(color, TFT_BLACK);
  while (text.length() > 0) {
    int n = text.length();
    while (n > 1 && tft.textWidth(text.substring(0, n), 2) > maxW) n--;
    tft.setCursor(x, y);
    tft.print(text.substring(0, n));
    text = text.substring(n);
    y += lineH;
  }
  return y;
}

void drawEvilTwinRunning(int frame) {
  if (isReleasing) {
    tft.fillScreen(TFT_BLACK);
    drawHeader("RELEASING VICTIM", TFT_GREEN);
    tft.setTextFont(2);
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.setCursor(10, 30); tft.print("Email:");
    int ny = drawWrapped(capturedEmail, 80, 30, 150, TFT_WHITE, 18);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.setCursor(10, ny); tft.print("Pass:");
    ny = drawWrapped(capturedPass, 80, ny, 150, TFT_WHITE, 18);
    int secsLeft = 2 - ((millis() - releaseTimer) / 1000);
    if (secsLeft < 0) secsLeft = 0;
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.setCursor(10, ny + 2);
    tft.print("Verify: "); tft.print(secsLeft); tft.println(" detik");
    int barX = 10, barY = ny + 20, barW = 220, barH = 12;
    tft.drawRect(barX, barY, barW, barH, TFT_WHITE);
    tft.fillRect(barX+1, barY+1, (secsLeft*(barW-2))/2, barH-2, TFT_GREEN);
    drawFooter("VERIFIKASI PASSWORD...");
    return;
  }

  if (isVerifying) {
    tft.fillScreen(TFT_BLACK);
    drawHeader("VERIFIKASI PASSWORD", TFT_ORANGE);
    tft.setTextFont(2); tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(10, 40); tft.println("Mencoba koneksi ke");
    tft.setCursor(10, 60); tft.println("AP target asli...");
    tft.setCursor(10, 82); tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.print("Percobaan: "); tft.print(verifyAttempts + 1);
    tft.print("/"); tft.println(MAX_VERIFY_ATTEMPTS);
    int spinIdx = (millis() / 200) % 4;
    const char* spin = "|/-\\";
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.setCursor(10, 100);
    tft.print("Progress: "); tft.print(spin[spinIdx]);
    drawFooter("MOHON TUNGGU (6s)...");
    return;
  }

  tft.fillScreen(TFT_BLACK);
  drawHeader(g_evilTwinCoop ? "EVIL TWIN 2 BOARD" : "EVIL TWIN AKTIF", g_evilTwinCoop ? TFT_DARKCYAN : TFT_MAGENTA);
  tft.setTextFont(2); tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setCursor(10, 30); tft.print(g_evilTwinCoop ? "AP: " : "Clone: ");
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  String ssid = targetSSID;
  if (ssid.length() > 14) ssid = ssid.substring(0, 14);
  tft.println(ssid);

  if (verifyAttempts > 0) {
    tft.setTextColor(TFT_ORANGE, TFT_BLACK);
    tft.setCursor(10, 45);
    tft.print("Percobaan: "); tft.print(verifyAttempts);
    tft.print("/"); tft.println(MAX_VERIFY_ATTEMPTS);
  }

  int clients = WiFi.softAPgetStationNum();

  if (clients > 0) {
    int pulse = ((frame % 8) - 4); if (pulse < 0) pulse = -pulse;
    uint16_t pc = (frame % 8 < 4) ? TFT_GREEN : TFT_DARKGREEN;
    drawCornerBrackets(4 + pulse, 61 - pulse, 232 - pulse*2, 30 + pulse*2, pc);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.setCursor(10, 65);
    tft.print("KORBAN: "); tft.print(clients); tft.println(" HP");
    tft.setTextColor(TFT_ORANGE, TFT_BLACK);
    tft.setCursor(10, 85);
    tft.print("Menunggu password");
    for (int i = 0; i < (frame % 4); i++) tft.print(".");
  } else {
    tft.setTextColor(TFT_ORANGE, TFT_BLACK);
    tft.setCursor(10, 65); tft.println("Korban: Belum ada");

    if (millis() - evilTwinStartTime < 3000) {
      tft.setTextColor(TFT_CYAN, TFT_BLACK);
      tft.setCursor(10, 85);
      tft.print("Menyiarkan SSID...");
    } else if (g_evilTwinCoop) {
      tft.setTextColor(TFT_CYAN, TFT_BLACK);
      tft.setCursor(10, 85);
      tft.print("Menunggu korban...");
    } else {
      tft.setTextColor(TFT_RED, TFT_BLACK);
      tft.setCursor(10, 85);
      tft.print("MENEMBAK DEAUTH");
      for (int i = 0; i < (frame % 4); i++) tft.print(".");
      const char* spin = "|/-\\";
      tft.setTextColor(TFT_YELLOW, TFT_BLACK);
      tft.setCursor(10, 100);
      tft.print("Radar: "); tft.print(spin[frame % 4]);
    }
  }
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextFont(1);
  tft.setCursor(150, 30);
  tft.print("P:" + String((uint32_t)getPacketsSent()) + " E:" + String((uint32_t)getTxErrors()));
  tft.setTextFont(2);
  drawFooter("TEKAN TOMBOL = STOP");
}

void drawViewPasswords() {
  tft.fillScreen(TFT_BLACK);
  drawHeader("PASSWORD TERSIMPAN", TFT_DARKCYAN);
  int total = prefs.getInt("pwd_count", 0);
  tft.setTextFont(2);

  if (total == 0) {
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(10, 40); tft.println("Belum ada data.");
    drawFooter("TOMBOL = KEMBALI");
    return;
  }

  if (viewSelection >= total) {
    int y = 42;
    if (viewSelection == total) {
      tft.fillRect(0, y, 240, 28, TFT_RED);
      tft.setTextColor(TFT_WHITE, TFT_RED);
      tft.setCursor(10, y + 6);
      tft.print(confirmDeleteAll ? "YAKIN? TEKAN LAGI!" : "HAPUS SEMUA DATA");
    } else {
      tft.fillRect(0, y, 240, 28, TFT_DARKGREEN);
      tft.setTextColor(TFT_WHITE, TFT_DARKGREEN);
      tft.setCursor(10, y + 6);
      tft.print("< KEMBALI >");
    }
    drawFooter("ATAS: Pilih | BAWAH: OK");
    return;
  }

  int idx = viewSelection;
  String entry = prefs.getString(("pwd_" + String(idx)).c_str(), "");
  int p1 = entry.indexOf('|');
  String email = (p1 > 0) ? entry.substring(0, p1) : entry;
  String rest = (p1 > 0) ? entry.substring(p1 + 1) : "";
  int p2 = rest.indexOf('|');
  String pass = (p2 > 0) ? rest.substring(0, p2) : rest;
  String ver = (p2 > 0) ? rest.substring(p2 + 1) : "";

  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setCursor(10, 30); tft.print("Email:");
  int ny = drawWrapped(email, 80, 30, 150, TFT_WHITE, 18);

  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setCursor(10, ny); tft.print("Pass:");
  ny = drawWrapped(pass, 80, ny, 150, TFT_GREEN, 18);

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(10, ny + 4);
  tft.print("Status: ");
  tft.setTextColor((ver == "1") ? TFT_GREEN : TFT_RED, TFT_BLACK);
  tft.print((ver == "1") ? "BENAR" : ((ver == "0") ? "SALAH" : "?"));

  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setCursor(10, ny + 22);
  tft.print("Data "); tft.print(idx + 1); tft.print("/"); tft.print(total);

  drawFooter("ATAS: Pilih | BAWAH: Hapus");
}


// ═══════════════════════════════════════════════
// Karma UI
// ═══════════════════════════════════════════════
void sortKarmaSSIDs() {
  for (int i = 0; i < g_karmaSSIDCount - 1; i++) {
    for (int j = i + 1; j < g_karmaSSIDCount; j++) {
      if (g_karmaSSIDs[j].probeCount > g_karmaSSIDs[i].probeCount) {
        KarmaSSIDEntry tmp = g_karmaSSIDs[i];
        g_karmaSSIDs[i] = g_karmaSSIDs[j];
        g_karmaSSIDs[j] = tmp;
      }
    }
  }
}

void drawKarmaScan(int frame) {
  tft.fillScreen(TFT_BLACK);
  drawHeader("KARMA SCAN", TFT_MAGENTA);
  tft.setTextFont(2); tft.setTextColor(TFT_WHITE, TFT_BLACK);

  unsigned long elapsed = millis() - karmaScanStart;
  int remain = (KARMA_SCAN_DURATION_MS > elapsed)
               ? (KARMA_SCAN_DURATION_MS - elapsed) / 1000 : 0;

  tft.setCursor(10, 28); tft.print("Waktu: ");
  tft.setTextColor(TFT_YELLOW, TFT_BLACK); tft.print(remain); tft.println("s");

  tft.setCursor(10, 44); tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.print("Klien: ");
  tft.setTextColor(TFT_CYAN, TFT_BLACK); tft.print(g_karmaClientCount);
  tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.print("  SSID: ");
  tft.setTextColor(TFT_CYAN, TFT_BLACK); tft.println(g_karmaSSIDCount);

  tft.setCursor(10, 60); tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.print("Response: ");
  tft.setTextColor(TFT_GREEN, TFT_BLACK); tft.println((uint32_t)g_karmaResponses);

  int cx = 200, cy = 55;
  for (int i = 0; i < 3; i++) {
    int r = (frame * 3 + i * 10) % 25;
    if (r > 2) tft.drawCircle(cx, cy, r, r < 12 ? TFT_MAGENTA : TFT_PURPLE);
  }
  tft.fillCircle(cx, cy, 2, TFT_PINK);

  tft.drawFastHLine(0, 78, 240, TFT_DARKGREY);
  tft.setTextFont(1); tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setCursor(4, 80); tft.print("TOP SSID (live):");

  KarmaSSIDEntry temp[KARMA_MAX_SSIDS];
  memcpy(temp, g_karmaSSIDs, sizeof(temp));
  for (int i = 0; i < g_karmaSSIDCount - 1; i++) {
    for (int j = i + 1; j < g_karmaSSIDCount; j++) {
      if (temp[j].probeCount > temp[i].probeCount) {
        KarmaSSIDEntry t = temp[i]; temp[i] = temp[j]; temp[j] = t;
      }
    }
  }
  for (int i = 0; i < 3 && i < g_karmaSSIDCount; i++) {
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(4, 90 + (i * 10));
    char buf[40];
    snprintf(buf, sizeof(buf), "%d. %-16s (%d)",
             i + 1, temp[i].ssid, temp[i].probeCount);
    tft.print(buf);
  }
  if (g_karmaSSIDCount == 0) {
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.setCursor(4, 90);
    tft.print("Menunggu probe");
    for (int i = 0; i < (frame % 4); i++) tft.print(".");
  }
  drawFooter("TUNGGU / TOMBOL = BATAL");
}

void drawKarmaResults() {
  tft.fillScreen(TFT_BLACK);
  drawHeader("KARMA RESULTS", TFT_GREEN);
  tft.setTextFont(2);

  if (g_karmaSSIDCount == 0) {
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(10, 40); tft.println("Tidak ada probe.");
    tft.setCursor(10, 60); tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.println("Kembali tekan OK");
  } else {
    for (int i = 0; i < 4; i++) {
      int idx = karmaScrollPos + i;
      if (idx > g_karmaSSIDCount) break;
      int y = 28 + (i * 22);

      bool isSelected = (idx == karmaSelection);
      bool isBack = (idx == g_karmaSSIDCount);

      if (isSelected) {
        tft.fillRect(0, y, 240, 20, TFT_DARKGREEN);
        drawCornerBrackets(2, y+1, 236, 18, TFT_GREEN);
        tft.setTextColor(TFT_GREEN, TFT_DARKGREEN);
      } else {
        tft.fillRect(0, y, 240, 20, TFT_BLACK);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
      }

      tft.setCursor(10, y + 2);
      if (idx < g_karmaSSIDCount) {
        String s = String(g_karmaSSIDs[idx].ssid);
        if (s.length() > 14) s = s.substring(0, 14);
        char buf[40];
        snprintf(buf, sizeof(buf), "%-14s %d",
                 s.c_str(), g_karmaSSIDs[idx].probeCount);
        tft.print(buf);
      } else if (isBack) {
        tft.print("<< KEMBALI");
      }
    }

    if (karmaScrollPos > 0) tft.fillTriangle(228, 26, 236, 26, 232, 22, TFT_CYAN);
    if (karmaScrollPos + 4 <= g_karmaSSIDCount) tft.fillTriangle(228, 112, 236, 112, 232, 116, TFT_CYAN);
  }
  drawFooter("ATAS: Pilih | BAWAH: OK");
}

// ═══════════════════════════════════════════════
// NVS helpers
// ═══════════════════════════════════════════════
void drawWebRemoteScreen() {
  tft.fillScreen(TFT_BLACK);
  drawHeader("WEB REMOTE", TFT_DARKGREEN);
  tft.setTextFont(2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(10, 28); tft.print("SSID: ");
  tft.setTextColor(TFT_CYAN, TFT_BLACK); tft.println("NISKALA_CTRL");
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(10, 48); tft.print("Pass: ");
  tft.setTextColor(TFT_YELLOW, TFT_BLACK); tft.println("niskala123");
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(10, 68); tft.print("URL : ");
  tft.setTextColor(TFT_GREEN, TFT_BLACK); tft.println("192.168.4.1");
  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.setCursor(10, 90); tft.print("BAT:"); tft.print(readBatteryPercent());
  tft.print("% T:"); tft.print((int)temperatureRead()); tft.print("C H:"); tft.print(ESP.getFreeHeap());
  drawFooter("TOMBOL = STOP & KEMBALI");
}

void drawPmfResultScreen(bool pmf, bool found) {
  tft.fillScreen(TFT_BLACK);
  drawHeader("CEK PMF", pmf ? TFT_RED : TFT_GREEN);
  tft.setTextFont(2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(10, 30);
  tft.print("Target: ");
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.println(targetSSID.length() ? targetSSID.substring(0, 18) : "(target)");

  tft.setCursor(10, 54);
  if (!found) {
    tft.setTextColor(TFT_ORANGE, TFT_BLACK);
    tft.println("Beacon: TIDAK ADA");
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(10, 74);
    tft.println("Pastikan 2.4 GHz");
  } else if (pmf) {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.println("PMF: YA (802.11w)");
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(10, 74);
    tft.println("Deauth tdk memutus");
  } else {
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.println("PMF: TIDAK");
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(10, 74);
    tft.println("Deauth bisa dicoba");
  }
  drawFooter("TOMBOL = KEMBALI");
}

void setBacklight(uint8_t duty) {
  static bool blInit = false;
  if (!blInit) {
    ledcSetup(0, 5000, 8);
    ledcAttachPin(TFT_BL, 0);
    blInit = true;
  }
  ledcWrite(0, duty);
}

void drawTiltedOrbit(int cx, int cy, int rx, int ry, float angle, uint16_t color) {
  const int STEPS = 28;
  float cosT = cos(angle), sinT = sin(angle);
  for (int i = 0; i < STEPS; i++) {
    float a0 = i * 2.0f * PI / STEPS;
    float a1 = (i + 1) * 2.0f * PI / STEPS;
    int x0 = cx + (int)(cos(a0) * rx * cosT - sin(a0) * ry * sinT);
    int y0 = cy + (int)(cos(a0) * rx * sinT + sin(a0) * ry * cosT);
    int x1 = cx + (int)(cos(a1) * rx * cosT - sin(a1) * ry * sinT);
    int y1 = cy + (int)(cos(a1) * rx * sinT + sin(a1) * ry * cosT);
    tft.drawLine(x0, y0, x1, y1, color);
  }
}

void drawButterflyScreensaver(int frame) {
  static const uint16_t BF_COLORS[] = { TFT_RED, TFT_GREEN, TFT_CYAN, TFT_MAGENTA, TFT_YELLOW, TFT_ORANGE, TFT_WHITE, TFT_BLUE };
  static const int BF_COUNT = 10;
  struct Bfly { float x; float y; float vx; float vy; float phase; uint16_t color; };
  static Bfly b[BF_COUNT];
  static bool bInit = false;
  if (!bInit) {
    for (int i = 0; i < BF_COUNT; i++) {
      b[i].x = random(20, 220);
      b[i].y = random(20, 110);
      b[i].vx = ((random(100) - 50) / 18.0f);
      b[i].vy = ((random(100) - 50) / 18.0f);
      if (b[i].vx == 0) b[i].vx = 0.6f;
      if (b[i].vy == 0) b[i].vy = 0.4f;
      b[i].phase = random(628) / 100.0f;
      b[i].color = BF_COLORS[i % 8];
    }
    bInit = true;
  }

  tft.fillScreen(TFT_BLACK);
  for (int i = 0; i < BF_COUNT; i++) {
    b[i].x += b[i].vx;
    b[i].y += b[i].vy;
    if (b[i].x < 8) { b[i].x = 8; b[i].vx = -b[i].vx; }
    if (b[i].x > 232) { b[i].x = 232; b[i].vx = -b[i].vx; }
    if (b[i].y < 8) { b[i].y = 8; b[i].vy = -b[i].vy; }
    if (b[i].y > 110) { b[i].y = 110; b[i].vy = -b[i].vy; }

    int x = (int)b[i].x;
    int y = (int)b[i].y;
    float flap = sin((float)frame * 0.35f + b[i].phase) * 4.0f;
    int w = (int)flap + 3;
    uint16_t col = b[i].color;

    tft.drawLine(x - 2, y, x + 2, y, col);
    tft.fillTriangle(x - 1, y, x - 5, y - w, x - 9, y, col);
    tft.fillTriangle(x + 1, y, x + 5, y - w, x + 9, y, col);
    tft.fillTriangle(x - 1, y, x - 4, y + w, x - 8, y, col);
    tft.fillTriangle(x + 1, y, x + 4, y + w, x + 8, y, col);
  }

  tft.setTextFont(1);
  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.setCursor(4, 122);
  tft.print("IDLE  BAT:");
  tft.print(readBatteryPercent());
  tft.print("%  H:");
  tft.print(ESP.getFreeHeap());
}
