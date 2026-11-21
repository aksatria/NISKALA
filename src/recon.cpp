#include "recon.h"
#include "ui_helpers.h"
#include "wifi_state.h"
#include <esp_wifi.h>
#include <ping/ping_sock.h>
volatile bool g_pingOK = false;
volatile bool g_pingDone = false;

static void onReconPingSuccess(esp_ping_handle_t hdl, void* args) {
  (void)hdl; (void)args;
  g_pingOK = true;
}

static void onReconPingTimeout(esp_ping_handle_t hdl, void* args) {
  (void)hdl; (void)args;
  g_pingOK = false;
}

static void onReconPingEnd(esp_ping_handle_t hdl, void* args) {
  (void)hdl; (void)args;
  g_pingDone = true;
}

bool pingHost(IPAddress ip, uint32_t timeout_ms) {
  esp_ping_config_t cfg;
  memset(&cfg, 0, sizeof(cfg));
  cfg.count = 1;
  cfg.interval_ms = 10;
  cfg.timeout_ms = timeout_ms;
  cfg.data_size = 32;
  cfg.tos = 0;
  cfg.ttl = 64;
  cfg.task_stack_size = 2048;
  cfg.task_prio = 2;
  cfg.interface = 0;
  cfg.target_addr.type = IPADDR_TYPE_V4;
  cfg.target_addr.u_addr.ip4.addr = (uint32_t)ip;

  esp_ping_callbacks_t cbs;
  memset(&cbs, 0, sizeof(cbs));
  cbs.on_ping_success = onReconPingSuccess;
  cbs.on_ping_timeout = onReconPingTimeout;
  cbs.on_ping_end = onReconPingEnd;

  esp_ping_handle_t ping = NULL;
  g_pingOK = false;
  g_pingDone = false;
  if (esp_ping_new_session(&cfg, &cbs, &ping) != ESP_OK) return false;
  if (esp_ping_start(ping) != ESP_OK) {
    esp_ping_delete_session(ping);
    return false;
  }

  uint32_t start = millis();
  while (!g_pingDone && (millis() - start) < (timeout_ms + 600)) {
    delay(2);
  }
  esp_ping_stop(ping);
  esp_ping_delete_session(ping);
  return g_pingOK;
}

void drawNetworkRecon() {
  tft.fillScreen(TFT_BLACK);
  drawHeader("NETWORK RECON", TFT_NAVY);
  tft.setTextFont(1);

  if (g_reconPhase == RECON_PHASE_HOST_LIST) {
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.setCursor(10, 28);
    tft.print("IP : "); tft.print(g_reconLocalIP.toString());
    tft.setCursor(10, 40);
    tft.print("GW : "); tft.print(g_reconGateway.toString());
    tft.print("  Sub:"); tft.print(g_reconSubnet.toString());
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.setCursor(10, 52);
    tft.printf("Host: %d / %u", g_reconHostCount, (unsigned)g_reconTotal);

    int rows = 3;
    for (int i = 0; i < rows; i++) {
      int idx = g_reconScroll + i;
      int y = 66 + i * 16;
      if (idx < g_reconHostCount) {
        bool selected = (idx == g_reconSelection);
        if (selected) tft.fillRoundRect(4, y - 1, 232, 14, 3, TFT_DARKGREY);
        tft.setTextColor(selected ? TFT_YELLOW : TFT_WHITE, selected ? TFT_DARKGREY : TFT_BLACK);
        tft.setCursor(10, y);
        tft.print("> ");
        tft.print(g_reconHosts[idx].toString());
      } else if (idx == g_reconHostCount) {
        bool selected = (idx == g_reconSelection);
        if (selected) tft.fillRoundRect(4, y - 1, 232, 14, 3, TFT_DARKGREY);
        tft.setTextColor(selected ? TFT_YELLOW : TFT_GREEN, selected ? TFT_DARKGREY : TFT_BLACK);
        tft.setCursor(10, y);
        tft.print("KEMBALI");
      }
    }
    drawFooter("ATAS: Pilih | BAWAH: Scan/Kembali");
  } else {
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.setCursor(10, 30);
    tft.print("Port scan: ");
    tft.print(g_reconSelectedIP.toString());
    tft.setTextColor(TFT_WHITE, TFT_BLACK);

    if (g_reconOpenPortCount == 0) {
      tft.setCursor(10, 50);
      tft.println("Tidak ada port terbuka.");
    } else {
      int rows = 5;
      for (int i = 0; i < g_reconOpenPortCount && i < rows; i++) {
        tft.setCursor(10, 42 + i * 13);
        tft.printf("TCP %d OPEN", g_reconOpenPorts[i]);
        if (g_reconBanners[i][0] != 0) {
          String b = String(g_reconBanners[i]);
          if (b.length() > 16) b = b.substring(0, 16);
          tft.print(" "); tft.print(b);
        }
      }
    }
    drawFooter("TOMBOL: Kembali");
  }
}void drawApWifiView() {
  tft.fillScreen(TFT_BLACK);
  drawHeader("AP WIFI TERSIMPAN", TFT_DARKCYAN);
  tft.setTextFont(1);

  int count = apPrefs.getInt("count", 0);
  if (count == 0) {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.setCursor(10, 50);
    tft.println("Belum ada kredensial.");
    tft.println("Tambah via CLI:");
    tft.println("apwifi add <SSID> <PASS>");
    drawFooter("TOMBOL: Kembali");
    return;
  }

  if (apWifiSelection > count) apWifiSelection = count;
  if (apWifiSelection < apWifiScrollPos) apWifiScrollPos = apWifiSelection;
  else if (apWifiSelection >= apWifiScrollPos + 3) apWifiScrollPos = apWifiSelection - 2;

  if (apWifiSelection < count) {
    String ssid = apPrefs.getString(("ssid_" + String(apWifiSelection)).c_str(), "");
    String pass = apPrefs.getString(("pass_" + String(apWifiSelection)).c_str(), "");
    if (ssid.length() > 20) ssid = ssid.substring(0, 20);
    if (pass.length() > 24) pass = pass.substring(0, 24);
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.setCursor(10, 30);
    tft.print("SSID: "); tft.print(ssid);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.setCursor(10, 42);
    tft.print("PASS: "); tft.print(pass);
  }

  int rows = 3;
  for (int i = 0; i < rows; i++) {
    int idx = apWifiScrollPos + i;
    int y = 56 + i * 16;
    if (idx < count) {
      bool selected = (idx == apWifiSelection);
      if (selected) tft.fillRoundRect(4, y - 1, 232, 14, 3, TFT_DARKGREY);
      String ssid = apPrefs.getString(("ssid_" + String(idx)).c_str(), "");
      if (ssid.length() > 22) ssid = ssid.substring(0, 22);
      tft.setTextColor(selected ? TFT_YELLOW : TFT_WHITE, selected ? TFT_DARKGREY : TFT_BLACK);
      tft.setCursor(10, y);
      tft.print(String(idx) + " " + ssid);
    } else if (idx == count) {
      bool selected = (idx == apWifiSelection);
      if (selected) tft.fillRoundRect(4, y - 1, 232, 14, 3, TFT_DARKGREY);
      tft.setTextColor(selected ? TFT_YELLOW : TFT_GREEN, selected ? TFT_DARKGREY : TFT_BLACK);
      tft.setCursor(10, y);
      tft.print("KEMBALI");
    }
  }

  if (confirmApWifiDelete && apWifiSelection < count) {
    drawFooter("TEKAN LAGI = HAPUS");
  } else {
    drawFooter("ATAS:Pilih | BAWAH:Hapus/Kembali");
  }
}

void reconPortScan(int hostIdx) {
  if (hostIdx < 0 || hostIdx >= g_reconHostCount) return;
  g_reconSelectedIP = g_reconHosts[hostIdx];
  g_reconOpenPortCount = 0;
  g_reconPortIndex = 0;
  g_reconPhase = RECON_PHASE_PORT_SCAN;

  tft.fillScreen(TFT_BLACK);
  drawHeader("PORT SCAN", TFT_NAVY);
  tft.setTextFont(1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(10, 40);
  tft.print(g_reconSelectedIP.toString());
  tft.setCursor(10, 60);
  tft.print("Mulai scan...");
}

void reconTick() {
  if (g_reconPhase == RECON_PHASE_CONNECTING) {
    if (WiFi.status() == WL_CONNECTED) {
      g_reconLocalIP = WiFi.localIP();
      g_reconGateway = WiFi.gatewayIP();
      g_reconSubnet = WiFi.subnetMask();
      uint32_t ipU = (uint32_t)g_reconLocalIP;
      uint32_t maskU = (uint32_t)g_reconSubnet;
      uint32_t netU = ipU & maskU;
      uint32_t bcU = netU | ~maskU;
      g_reconFirst = netU + 1;
      g_reconLast = bcU - 1;
      g_reconTotal = (g_reconLast >= g_reconFirst) ? (g_reconLast - g_reconFirst + 1) : 0;
      g_reconNext = g_reconFirst;
      g_reconScanned = 0;

      tft.fillScreen(TFT_BLACK);
      drawHeader("PING SWEEP", TFT_NAVY);
      tft.setTextFont(1);
      tft.setTextColor(TFT_CYAN, TFT_BLACK);
      tft.setCursor(10, 40);
      tft.printf("IP  : %s\n", g_reconLocalIP.toString().c_str());
      tft.setCursor(10, 52);
      tft.printf("GW  : %s\n", g_reconGateway.toString().c_str());
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      tft.setCursor(10, 64);
      tft.print("Scan: ");
      g_reconPhase = RECON_PHASE_SWEEP;
    } else if (millis() - g_reconConnectStart > 12000) {
      tft.setTextColor(TFT_RED, TFT_BLACK);
      tft.setCursor(10, 60);
      tft.println("Gagal konek WiFi.");
      tft.println("Periksa password target.");
      stopNetworkRecon();
      currentState = MAIN_MENU;
      drawMainMenu();
      return;
    }
  } else if (g_reconPhase == RECON_PHASE_SWEEP) {
    for (int i = 0; i < 3 && g_reconNext <= g_reconLast && g_reconNext != 0; i++) {
      uint32_t h = g_reconNext++;
      if (h == (uint32_t)g_reconLocalIP) continue;
      g_reconScanned++;
      IPAddress host((uint32_t)h);
      bool alive = pingHost(host, 100);
      if (alive && g_reconHostCount < RECON_MAX_HOSTS) {
        g_reconHosts[g_reconHostCount++] = host;
      }
    }

    tft.fillRect(50, 64, 180, 10, TFT_BLACK);
    tft.setCursor(50, 64);
    tft.printf("%u/%u host:%u", (unsigned)g_reconScanned, (unsigned)g_reconTotal, (unsigned)g_reconHostCount);

    if (g_reconNext > g_reconLast || g_reconNext == 0) {
      g_reconPhase = RECON_PHASE_HOST_LIST;
      drawNetworkRecon();
    }
  } else if (g_reconPhase == RECON_PHASE_PORT_SCAN) {
    static const int ports[RECON_MAX_PORTS] = {21, 22, 23, 25, 53, 80, 110, 143, 443, 445, 3306, 3389, 8080, 9200};
    if (g_reconPortIndex < RECON_MAX_PORTS) {
      int port = ports[g_reconPortIndex++];
      tft.fillRect(0, 60, 240, 12, TFT_BLACK);
      tft.setCursor(10, 60);
      tft.printf("Cek TCP %d...", port);
      if (g_reconClient.connect(g_reconSelectedIP, port, 150)) {
        if (g_reconOpenPortCount < RECON_MAX_PORTS) {
          g_reconOpenPorts[g_reconOpenPortCount++] = port;
        }
        g_reconClient.stop();
      }
    } else {
      g_reconBannerIndex = 0;
      g_reconPhase = RECON_PHASE_BANNER;
      tft.fillRect(0, 60, 240, 12, TFT_BLACK);
      tft.setCursor(10, 60);
      tft.print("Ambil banner...");
    }
  } else if (g_reconPhase == RECON_PHASE_BANNER) {
    if (g_reconBannerIndex < g_reconOpenPortCount) {
      int port = g_reconOpenPorts[g_reconBannerIndex];
      tft.fillRect(0, 60, 240, 12, TFT_BLACK);
      tft.setCursor(10, 60);
      tft.printf("Banner TCP %d...", port);

      WiFiClient bc;
      if (bc.connect(g_reconSelectedIP, port, 300)) {
        unsigned long st = millis();
        int n = 0;
        while (bc.available() && n < 39 && (millis() - st) < 250) {
          char ch = bc.read();
          if (ch == '\n' || ch == '\r') break;
          g_reconBanners[g_reconBannerIndex][n++] = ch;
        }
        g_reconBanners[g_reconBannerIndex][n] = 0;
        bc.stop();
      } else {
        g_reconBanners[g_reconBannerIndex][0] = 0;
      }
      g_reconBannerIndex++;
    } else {
      g_reconPhase = RECON_PHASE_PORT_RESULT;
      drawNetworkRecon();
    }
  }
}
void stopNetworkRecon() {
  isReconRunning = false;
  g_reconClient.stop();
  g_reconPhase = RECON_PHASE_HOST_LIST;
  resetWifiStateAfterAttack();
  Serial.println("[RECON] Berhenti");
}

bool apWifiAdd(String ssid, String pass) {
  if (ssid.length() == 0 || pass.length() == 0) return false;
  int count = apPrefs.getInt("count", 0);
  for (int i = 0; i < count; i++) {
    String key = "ssid_" + String(i);
    if (apPrefs.getString(key.c_str(), "") == ssid) {
      apPrefs.putString(("pass_" + String(i)).c_str(), pass);
      return true;
    }
  }
  if (count >= MAX_AP_WIFI) return false;
  apPrefs.putString(("ssid_" + String(count)).c_str(), ssid);
  apPrefs.putString(("pass_" + String(count)).c_str(), pass);
  apPrefs.putInt("count", count + 1);
  return true;
}

String apWifiFindPassword(String ssid) {
  if (ssid.length() == 0) return "";
  int count = apPrefs.getInt("count", 0);
  for (int i = 0; i < count; i++) {
    if (apPrefs.getString(("ssid_" + String(i)).c_str(), "") == ssid) {
      return apPrefs.getString(("pass_" + String(i)).c_str(), "");
    }
  }
  return "";
}

void apWifiList() {
  int count = apPrefs.getInt("count", 0);
  Serial.printf("[APWIFI] %d kredensial\n", count);
  for (int i = 0; i < count; i++) {
    Serial.printf("  %2d  %s  %s\n", i,
      apPrefs.getString(("ssid_" + String(i)).c_str(), "(?)").c_str(),
      apPrefs.getString(("pass_" + String(i)).c_str(), "").c_str());
  }
}

bool apWifiDelete(int idx) {
  int count = apPrefs.getInt("count", 0);
  if (idx < 0 || idx >= count) return false;
  for (int i = idx; i < count - 1; i++) {
    apPrefs.putString(("ssid_" + String(i)).c_str(), apPrefs.getString(("ssid_" + String(i + 1)).c_str(), ""));
    apPrefs.putString(("pass_" + String(i)).c_str(), apPrefs.getString(("pass_" + String(i + 1)).c_str(), ""));
  }
  apPrefs.remove(("ssid_" + String(count - 1)).c_str());
  apPrefs.remove(("pass_" + String(count - 1)).c_str());
  apPrefs.putInt("count", count - 1);
  return true;
}

String findReconPasswordForSSID(String ssid) {
  if (ssid.length() == 0) return "";
  String apPass = apWifiFindPassword(ssid);
  if (apPass.length() > 0) return apPass;
  if (ssid == cfg_wifi_ssid && cfg_wifi_pass.length() > 0) return cfg_wifi_pass;
  int total = prefs.getInt("pwd_count", 0);
  for (int i = 0; i < total; i++) {
    String entry = prefs.getString(("pwd_" + String(i)).c_str(), "");
    int p1 = entry.indexOf('|');
    if (p1 <= 0) continue;
    String email = entry.substring(0, p1);
    String pass = entry.substring(p1 + 1);
    if (email == ssid && pass.length() > 0) return pass;
  }
  return "";
}

void startNetworkRecon() {
  isReconRunning = true;
  g_reconHostCount = 0;
  g_reconSelection = 0;
  g_reconScroll = 0;
  g_reconOpenPortCount = 0;
  g_reconPortIndex = 0;
  g_reconBannerIndex = 0;
  memset(g_reconBanners, 0, sizeof(g_reconBanners));
  g_reconPhase = RECON_PHASE_CONNECTING;

  String ssid = targetSSID;
  tft.fillScreen(TFT_BLACK);
  drawHeader("NETWORK RECON", TFT_NAVY);
  tft.setTextFont(1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(10, 40);

  if (ssid.length() == 0) {
    tft.println("SSID hidden/tidak valid.");
    tft.println("Tidak bisa recon.");
    isReconRunning = false;
    currentState = MAIN_MENU;
    drawMainMenu();
    return;
  }

  String pass = findReconPasswordForSSID(ssid);
  bool openNetwork = (g_reconSelectedAuth == 0);
  if (pass.length() == 0 && !openNetwork) {
    tft.println("Password target belum");
    tft.println("tersimpan.");
    tft.println("");
    tft.println("Set via CLI:");
    tft.println("config set ssid=<SSID> pass=<PASS>");
    isReconRunning = false;
    currentState = MAIN_MENU;
    drawMainMenu();
    return;
  }

  tft.print("Hubung WiFi: ");
  tft.println(ssid.substring(0, 18));
  radioSetMode(RADIO_STA);
  WiFi.disconnect(true);
  WiFi.begin(ssid.c_str(), pass.c_str());
  g_reconConnectStart = millis();
}
