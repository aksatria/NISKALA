#include "cli.h"
#include "state_machine.h"
#include "ui_helpers.h"
#include "attack_engine.h"
#include "web_remote.h"
#include "bt_spam.h"
#include "ble_scan.h"
#include "cloud.h"
#include "recon.h"
#include "macro.h"
#include "telemetry.h"

static String cliBuf = "";
int hexNibble(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}

int hexToBytes(const char* hex, uint8_t* out, int maxLen) {
  int len = strlen(hex);
  if (len <= 0 || (len % 2) != 0) return -1;
  int n = len / 2;
  if (n > maxLen) n = maxLen;
  for (int i = 0; i < n; i++) {
    int hi = hexNibble(hex[i * 2]);
    int lo = hexNibble(hex[i * 2 + 1]);
    if (hi < 0 || lo < 0) return -1;
    out[i] = (uint8_t)((hi << 4) | lo);
  }
  return n;
}

bool macFromString(const char* s, uint8_t mac[6]) {
  char tmp[13];
  int j = 0;
  for (int i = 0; s[i] && j < 12; i++) {
    if (s[i] == ':' || s[i] == '-' || s[i] == ' ') continue;
    tmp[j++] = s[i];
  }
  if (j != 12) return false;
  tmp[12] = 0;
  for (int i = 0; i < 6; i++) {
    int hi = hexNibble(tmp[i * 2]);
    int lo = hexNibble(tmp[i * 2 + 1]);
    if (hi < 0 || lo < 0) return false;
    mac[i] = (uint8_t)((hi << 4) | lo);
  }
  return true;
}

bool apWifiAdd(String ssid, String pass);
String apWifiFindPassword(String ssid);
void apWifiList();
bool apWifiDelete(int idx);

void cliPrintHelp() {
  Serial.println("NISKALA CLI");
  Serial.println("  help");
  Serial.println("  status");
  Serial.println("  scan");
  Serial.println("  stop");
  Serial.println("  beacon | probe | karma | global | blespam | blescan");
  Serial.println("  deauth <AA:BB:CC:DD:EE:FF>");
  Serial.println("  ble-raw <hex>");
  Serial.println("  ble-ibeacon [major minor]");
  Serial.println("  ble-eddy");
  Serial.println("  config get | config set <ssid|pass|url|tpl> <nilai>");
  Serial.println("  apwifi add <SSID> <PASS> | apwifi list | apwifi del <index>");
  Serial.println("  reboot");
}

void processCliCommand(String line) {
  line.trim();
  if (line.length() == 0) return;

  int sp = line.indexOf(' ');
  String cmd = (sp > 0) ? line.substring(0, sp) : line;
  String arg = (sp > 0) ? line.substring(sp + 1) : "";
  cmd.toLowerCase();

  if (cmd == "help") { cliPrintHelp(); }
  else if (cmd == "status") {
    Serial.println(buildSystemStatusLine());
  }
  else if (cmd == "scan") {
    WiFi.mode(WIFI_STA);
    int n = WiFi.scanNetworks(false, true);
    Serial.printf("[SCAN] %d AP\n", n);
    for (int i = 0; i < n && i < 20; i++) {
      Serial.printf("  %2d  ch=%2d  rssi=%4d  %s  %s\n", i,
        WiFi.channel(i), WiFi.RSSI(i), WiFi.BSSIDstr(i).c_str(), WiFi.SSID(i).c_str());
    }
    WiFi.scanDelete();
  }
  else if (cmd == "stop") {
    stopAllAttacks();
    currentState = MAIN_MENU;
    drawMainMenu();
    Serial.println("[CLI] Semua berhenti");
  }
  else if (cmd == "beacon") {
    stopAllAttacks();
    currentState = BEACON_SPAM; startBeaconSpam(); drawBeaconSpam(0);
    Serial.println("[CLI] Beacon spam ON");
  }
  else if (cmd == "probe") {
    stopAllAttacks();
    currentState = PROBE_SNIFF; startProbeSniffer(); drawProbeSniff(0);
    Serial.println("[CLI] Probe sniff ON");
  }
  else if (cmd == "karma") {
    stopAllAttacks();
    currentState = KARMA_SCAN; startKarmaScan(); drawKarmaScan(0);
    Serial.println("[CLI] Karma ON");
  }
  else if (cmd == "global") {
    stopAllAttacks();
    currentState = GLOBAL_DEAUTH; startGlobalDeauth(); drawGlobalDeauth(0);
    Serial.println("[CLI] Global deauth ON");
  }
  else if (cmd == "blespam") {
    stopAllAttacks();
    btStopFixed();
    currentState = BT_SPAM; startBTSpam(); drawBTSpam(0);
    Serial.println("[CLI] BLE spam ON");
  }
  else if (cmd == "blescan") {
    stopAllAttacks();
    currentState = BLE_SCAN; startBLEScan();
    Serial.println("[CLI] BLE scan ON");
  }
  else if (cmd == "deauth") {
    uint8_t mac[6];
    if (!macFromString(arg.c_str(), mac)) {
      Serial.println("[CLI] MAC tidak valid. Contoh: deauth AA:BB:CC:DD:EE:FF");
      return;
    }
    memcpy(g_targetBSSID, mac, 6);
    g_targetChannel = 1;
    targetSSID = "CLI_Target";
    WiFi.mode(WIFI_STA);
    int n = WiFi.scanNetworks(false, true);
    for (int i = 0; i < n; i++) {
      if (memcmp(WiFi.BSSID(i), mac, 6) == 0) {
        g_targetChannel = WiFi.channel(i);
        targetSSID = WiFi.SSID(i);
        break;
      }
    }
    WiFi.scanDelete();
    stopAllAttacks();
    currentState = DEAUTH_MENU;
    startDeauthEngine();
    drawDeauthMenu();
    Serial.printf("[CLI] Deauth target %02X:%02X:%02X:%02X:%02X:%02X ch=%d\n",
      mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], g_targetChannel);
  }
  else if (cmd == "ble-raw") {
    uint8_t buf[31];
    int n = hexToBytes(arg.c_str(), buf, 31);
    if (n <= 0) { Serial.println("[CLI] hex tidak valid / kosong"); return; }
    stopAllAttacks();
    WiFi.mode(WIFI_OFF);
    if (btStartFixed(buf, n)) {
      isBTSpamRunning = true;
      currentState = BT_SPAM;
      drawBTSpam(0);
      Serial.printf("[CLI] BLE raw advertise %d bytes\n", n);
    } else {
      Serial.println("[CLI] Gagal mulai BLE raw");
    }
  }
  else if (cmd == "ble-ibeacon") {
    int sp2 = arg.indexOf(' ');
    int major = 1;
    int minor = 2;
    if (sp2 > 0) {
      major = arg.substring(0, sp2).toInt();
      minor = arg.substring(sp2 + 1).toInt();
    } else if (arg.length() > 0) {
      major = arg.toInt();
    }
    stopAllAttacks();
    WiFi.mode(WIFI_OFF);
    if (btStartIBeacon(IBEACON_DEFAULT_UUID, (uint16_t)major, (uint16_t)minor, -59)) {
      isBTSpamRunning = true;
      currentState = BT_SPAM;
      drawBTSpam(0);
      Serial.printf("[CLI] iBeacon major=%d minor=%d\n", major, minor);
    } else {
      Serial.println("[CLI] Gagal mulai iBeacon");
    }
  }
  else if (cmd == "ble-eddy") {
    stopAllAttacks();
    WiFi.mode(WIFI_OFF);
    if (btStartEddystone(EDDYSTONE_NS, EDDYSTONE_INST, -59)) {
      isBTSpamRunning = true;
      currentState = BT_SPAM;
      drawBTSpam(0);
      Serial.println("[CLI] Eddystone-UID ON");
    } else {
      Serial.println("[CLI] Gagal mulai Eddystone");
    }
  }
  else if (cmd == "config") {
    if (arg.length() == 0 || arg == "get") {
      Serial.printf("SSID : %s\n", cfg_wifi_ssid.c_str());
      Serial.printf("PASS : %s\n", cfg_wifi_pass.c_str());
      Serial.printf("URL  : %s\n", cfg_server_url.c_str());
      Serial.printf("TPL  : %s\n", cfg_template_url.c_str());
      Serial.printf("burst deauth=%d auth=%d hop=%d beacon=%d reason=%d\n",
        cfg_deauth_burst, cfg_auth_burst, cfg_hop_interval_ms, cfg_beacon_hop_ms, cfg_deauth_reason);
    } else {
      int s1 = arg.indexOf(' ');
      if (s1 > 0) {
        String k = arg.substring(0, s1);
        String v = arg.substring(s1 + 1);
        k.toLowerCase();
        if (k == "ssid") cfg_wifi_ssid = v;
        else if (k == "pass") cfg_wifi_pass = v;
        else if (k == "url") cfg_server_url = v;
        else if (k == "tpl") cfg_template_url = v;
        else { Serial.println("[CLI] key tidak dikenal"); return; }
        saveConfig();
        Serial.printf("[CLI] %s = %s (tersimpan)\n", k.c_str(), v.c_str());
      } else {
        Serial.println("[CLI] Gunakan: config set <ssid|pass|url|tpl> <nilai>");
      }
    }
  }
  else if (cmd == "apwifi") {
    arg.trim();
    int s = arg.indexOf(' ');
    String sub = (s > 0) ? arg.substring(0, s) : arg;
    String rest = (s > 0) ? arg.substring(s + 1) : "";
    sub.toLowerCase();
    if (sub == "add" || sub == "set") {
      rest.trim();
      int s2 = rest.indexOf(' ');
      if (s2 > 0) {
        String ssid = rest.substring(0, s2);
        String pass = rest.substring(s2 + 1);
        pass.trim();
        if (apWifiAdd(ssid, pass)) Serial.printf("[APWIFI] Tersimpan: %s\n", ssid.c_str());
        else Serial.println("[APWIFI] Gagal simpan (penuh/argumen kosong).");
      } else {
        Serial.println("[APWIFI] Gunakan: apwifi add <SSID> <PASS>");
      }
    } else if (sub == "list") {
      apWifiList();
    } else if (sub == "del" || sub == "delete") {
      int idx = rest.toInt();
      if (apWifiDelete(idx)) Serial.printf("[APWIFI] Index %d dihapus\n", idx);
      else Serial.println("[APWIFI] Index tidak valid");
    } else {
      Serial.println("[APWIFI] Gunakan: add <SSID> <PASS> | list | del <index>");
    }
  }
  else if (cmd == "sync" || cmd == "cloudsync") {
    Serial.println("[CLI] Mulai Cloud Sync...");
    startCloudSync();
  }
  else if (cmd == "reboot") {
    Serial.println("[CLI] Reboot...");
    delay(200);
    ESP.restart();
  }
  else {
    Serial.printf("[CLI] Perintah tidak dikenal: %s (ketik help)\n", line.c_str());
  }
}

void handleSerialCLI() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (cliBuf.length() > 0) {
        processCliCommand(cliBuf);
        cliBuf = "";
      }
    } else {
      if (cliBuf.length() < 127) cliBuf += c;
    }
  }
}

// ------------------------------------------------------------------
// Client Monitor (lihat klien/station terhubung ke AP target)
// ------------------------------------------------------------------
