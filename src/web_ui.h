#ifndef NISKALA_WEB_UI_H
#define NISKALA_WEB_UI_H

#include <Arduino.h>

// UI utama Web Remote sekarang disimpan di `data/index.html` (LittleFS).
// String ini hanya dipakai sebagai fallback saat file belum di-upload.
// Upload dengan: pio run -t uploadfs
const char HTML_REMOTE_FALLBACK[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="id">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>NISKALA Web Remote</title>
<style>
body{margin:0;min-height:100vh;display:grid;place-items:center;background:#0b1018;color:#e8edf4;font-family:system-ui,sans-serif;text-align:center;padding:24px}
code{background:#141b28;padding:6px 10px;border-radius:8px;color:#22d3ee}
</style>
</head>
<body>
<div>
  <h1>NISKALA Web Remote</h1>
  <p>File UI belum tersedia di filesystem.</p>
  <p>Upload dengan menjalankan:</p>
  <code>pio run -t uploadfs</code>
</div>
</body>
</html>
)rawliteral";

#endif