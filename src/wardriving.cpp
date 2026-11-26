#include "wardriving.h"
#include <WiFi.h>
#include <LittleFS.h>
#include <ESPAsyncWebServer.h>

AsyncWebServer wardServer(80);
AsyncWebSocket wardWS("/ws");

bool isWardrivingRunning = false;

static unsigned long lastScan = 0;
static bool scanRunning = false;
static const unsigned long SCAN_INTERVAL_MS = 5000;

static String jsonEscape(const String& s) {
  String out;
  out.reserve(s.length() + 8);
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
  return out;
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

static void broadcastAp(const String& mac, const String& ssid, int ch, int rssi, int auth) {
  if (!isWardrivingRunning) return;
  String json = "{\"mac\":\"" + mac + "\",\"ssid\":\"" + jsonEscape(ssid) +
                "\",\"ch\":" + String(ch) + ",\"rssi\":" + String(rssi) +
                ",\"enc\":\"" + authName(auth) + "\"}";
  wardWS.textAll(json);
}

static void onWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
                      AwsEventType type, void* arg, uint8_t* data, size_t len) {
  (void)server; (void)arg; (void)data; (void)len;
  if (type == WS_EVT_CONNECT) {
    Serial.printf("[WARD] WS client #%u connect\n", client->id());
    lastScan = 0;
  } else if (type == WS_EVT_DISCONNECT) {
    Serial.printf("[WARD] WS client #%u disconnect\n", client->id());
  }
}

void wardrivingStart() {
  if (isWardrivingRunning) return;
  if (!LittleFS.begin(true)) {
    Serial.println("[WARD] LittleFS mount failed");
  }

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  wardWS.onEvent(onWsEvent);
  wardServer.addHandler(&wardWS);
  wardServer.serveStatic("/", LittleFS, "/").setDefaultFile("wardriving.html");
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
  isWardrivingRunning = true;
  Serial.println("[WARD] Wardriving aktif di http://192.168.4.1/");
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

  if (!scanRunning && (lastScan == 0 || millis() - lastScan >= SCAN_INTERVAL_MS)) {
    lastScan = millis();
    int n = WiFi.scanNetworks(true, false, true, 300, 0);
    if (n == WIFI_SCAN_RUNNING) scanRunning = true;
  }

  if (scanRunning) {
    int n = WiFi.scanComplete();
    if (n >= 0) {
      for (int i = 0; i < n; i++) {
        broadcastAp(WiFi.BSSIDstr(i), WiFi.SSID(i), WiFi.channel(i), WiFi.RSSI(i), (int)WiFi.encryptionType(i));
      }
      WiFi.scanDelete();
      scanRunning = false;
    }
  }
}