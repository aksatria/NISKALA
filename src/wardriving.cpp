#include "wardriving.h"
#include <WiFi.h>
#include <LittleFS.h>
#include <ESPAsyncWebServer.h>

AsyncWebServer wardServer(8080);
AsyncWebSocket wardWS("/ws");

bool isWardrivingRunning = false;

static unsigned long lastScan = 0;
static bool scanRunning = false;
static unsigned long lastHeartbeat = 0;
static const unsigned long SCAN_INTERVAL_MS = 5000;
static const unsigned long HEARTBEAT_MS = 10000;
static String saveBuffer; // buffer untuk simpan CSV dari browser

// Escape string untuk JSON. reserve() di luar fungsi caller untuk hindari fragmentasi.
static void jsonEscapeTo(String& out, const String& s) {
  out.reserve(out.length() + s.length() + 8); // alokasi sekali, bukan per char
  for (unsigned int i = 0; i < s.length(); i++) {
    char c = s[i];
    if (c == '\\') out += "\\\\";
    else if (c == '"') out += "\\\"";
    else if (c == '\n') out += "\\n";
    else if (c == '\r') out += "\\r";
    else if (c == '\t') out += "\\t";
    else if ((unsigned char)c < 0x20) out += ' ';
    else out += c;
  }
}

static const char* authName(int auth) {
  switch (auth) {
    case WIFI_AUTH_OPEN: return "OPEN";
    case WIFI_AUTH_WEP: return "WEP";
    case WIFI_AUTH_WPA_PSK: return "WPA";
    case WIFI_AUTH_WPA2_PSK: return "WPA2";
    case WIFI_AUTH_WPA_WPA2_PSK: return "WPA/WPA2";
    case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2-EAP";
    case WIFI_AUTH_WPA3_PSK: return "WPA3";
    case WIFI_AUTH_WPA2_WPA3_PSK: return "WPA2/WPA3";
    case WIFI_AUTH_WPA3_ENT_192: return "WPA3-ENT";
    default: return "UNKNOWN";
  }
}

// Broadcast satu AP. Gunakan satu buffer + reserve untuk meminimalkan fragmentasi heap.
static void broadcastAp(const String& mac, const String& ssid, int ch, int rssi, int auth) {
  if (!isWardrivingRunning || wardWS.count() == 0) return; // skip bila tak ada client

  String json;
  json.reserve(96 + ssid.length()); // perkiraan cukup, alokasi satu kali
  json += "{\"mac\":\"";
  json += mac;
  json += "\",\"ssid\":\"";
  jsonEscapeTo(json, ssid);
  json += "\",\"ch\":";
  json += String(ch);
  json += ",\"rssi\":";
  json += String(rssi);
  json += ",\"enc\":\"";
  json += authName(auth);
  json += "\"}";

  wardWS.textAll(json);
}

static void onWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
                      AwsEventType type, void* arg, uint8_t* data, size_t len) {
  (void)server; (void)arg; (void)data; (void)len;
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("[WARD] WS client #%u connect\n", client->id());
      lastScan = 0; // mulai scan segera saat client baru masuk
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("[WARD] WS client #%u disconnect\n", client->id());
      break;
    case WS_EVT_PONG:
      // Heartbeat: klien masih hidup.
      break;
    default:
      break;
  }
}

void wardrivingSetup() {
  if (isWardrivingRunning) return;
  if (!LittleFS.begin(true)) {
    Serial.println("[WARD] LittleFS mount failed");
  }

  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP("NISKALA_CTRL", "niskala123", 1, 0, 4);
  delay(200);

  wardWS.onEvent(onWsEvent);
  wardServer.addHandler(&wardWS);
  wardServer.serveStatic("/", LittleFS, "/").setDefaultFile("wardriving.html");

  // Simpan CSV (dikirim browser) ke LittleFS agar backup di board.
  wardServer.on("/api/save", HTTP_POST,
    [](AsyncWebServerRequest* request) {
      String fname = "/niskala_" + String(millis()) + ".csv";
      File f = LittleFS.open(fname, "w");
      if (f) {
        f.print(saveBuffer);
        f.close();
        request->send(200, "text/plain", fname);
      } else {
        request->send(500, "text/plain", "write failed");
      }
    },
    NULL,
    [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      (void)request; (void)total;
      if (index == 0) saveBuffer = "";
      if (saveBuffer.length() + len <= 200000) {
        saveBuffer.concat((char*)data, len);
      }
    });
  wardServer.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (LittleFS.exists("/wardriving.html")) {
      request->send(LittleFS, "/wardriving.html", "text/html");
    } else {
      request->send(200, "text/plain", "wardriving.html not found");
    }
  });
  wardServer.begin();

  lastScan = 0;
  scanRunning = false;
  lastHeartbeat = millis();
  isWardrivingRunning = true;
  Serial.println("[WARD] Wardriving aktif di http://192.168.4.1:8080/");
}

void wardrivingStart() {
  wardrivingSetup();
}

void wardrivingStop() {
  if (!isWardrivingRunning) return;
  wardWS.closeAll();
  wardServer.end();
  scanRunning = false;
  isWardrivingRunning = false;
  Serial.println("[WARD] Wardriving berhenti");
}

void wardrivingLoop() {
  if (!isWardrivingRunning) return;

  // Heartbeat: deteksi ghost connection agar resource ESP32 tidak habis.
  if (millis() - lastHeartbeat >= HEARTBEAT_MS) {
    lastHeartbeat = millis();
    wardWS.pingAll();
    wardWS.cleanupClients();
  }

  // Mulai passive scan non-blocking.
  if (!scanRunning && (lastScan == 0 || millis() - lastScan >= SCAN_INTERVAL_MS)) {
    lastScan = millis();
    int n = WiFi.scanNetworks(true, false, true, 300, 0);
    if (n == WIFI_SCAN_RUNNING) {
      scanRunning = true;
    } else if (n == WIFI_SCAN_FAILED) {
      scanRunning = false;
      delay(10); // hindari loop panas saat radio gagal
    }
    yield(); // beri napas ke RTOS/WDT
  }

  // Poll hasil scan.
  if (scanRunning) {
    int n = WiFi.scanComplete();
    if (n >= 0) {
      for (int i = 0; i < n; i++) {
        broadcastAp(WiFi.BSSIDstr(i), WiFi.SSID(i), WiFi.channel(i), WiFi.RSSI(i), (int)WiFi.encryptionType(i));
        yield(); // hindari burst loop panjang yang memicu WDT
      }
      WiFi.scanDelete();
      scanRunning = false;
    }
  }
}