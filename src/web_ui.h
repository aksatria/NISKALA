#ifndef NISKALA_WEB_UI_H
#define NISKALA_WEB_UI_H

#include <Arduino.h>
const char HTML_REMOTE[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html lang="id"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover"><title>NISKALA · Web Remote</title><link href="https://fonts.googleapis.com/css2?family=Nunito:wght@400;600;700;800;900&display=swap" rel="stylesheet"><style>
:root{--bg:#080d13;--panel:#101a26;--panel2:#0c1620;--line:rgba(150,190,210,.12);--text:#edf5fb;--muted:#8aa0b2;--green:#5eead4;--cyan:#67d7f0;--red:#ff7a85;--amber:#ffc96b;--r:14px}
*{box-sizing:border-box}html{background:var(--bg);overflow-x:hidden}body{margin:0;background:radial-gradient(circle at 12% -6%,rgba(94,234,212,.12),transparent 36%),linear-gradient(160deg,var(--bg),#0a121c 65%,#070d13);color:var(--text);font-family:'Nunito',system-ui,sans-serif;font-size:14px;line-height:1.55;font-weight:600;overflow-x:hidden}
.app{max-width:920px;margin:0 auto;padding:16px 16px 56px}
.topbar{display:flex;align-items:center;justify-content:space-between;gap:12px;margin-bottom:12px;flex-wrap:wrap}
.brand{display:flex;align-items:center;gap:10px}
.brand .logo{width:38px;height:38px;border-radius:11px;background:linear-gradient(135deg,var(--green),var(--cyan));display:grid;place-items:center;font-weight:900;color:#04221a;box-shadow:0 8px 22px rgba(94,234,212,.18)}
.brand b{font-size:17px;letter-spacing:.12em;font-weight:900}
.brand small{display:block;color:var(--muted);font-size:10.5px;line-height:1.2}
.top-actions{display:flex;align-items:center;gap:8px;flex-wrap:wrap}
.pill{display:inline-flex;align-items:center;gap:6px;border:1px solid rgba(94,234,212,.3);border-radius:99px;padding:7px 12px;color:var(--green);font-size:10.5px;font-weight:800;background:rgba(94,234,212,.07)}
.pill i{width:7px;height:7px;border-radius:50%;background:currentColor;box-shadow:0 0 10px currentColor}
.token{display:flex;gap:6px}
.token input{width:130px;border:1px solid var(--line);border-radius:10px;background:#0a1420;color:var(--text);padding:8px 10px;font:600 12px 'Nunito',sans-serif;outline:none}
.statusbar{display:flex;align-items:center;justify-content:space-between;gap:12px;background:var(--panel);border:1px solid var(--line);border-radius:var(--r);padding:12px 16px;margin-bottom:14px}
.statusbar b{font-size:13px;color:var(--muted);font-weight:700}
.statusbar b span{color:var(--green)}
.tabs{display:flex;gap:6px;overflow-x:auto;margin-bottom:14px;padding-bottom:2px;scrollbar-width:none}
.tabs::-webkit-scrollbar{display:none}
.tab{flex:0 0 auto;border:1px solid var(--line);background:var(--panel);color:var(--muted);border-radius:99px;padding:10px 16px;font:800 12.5px 'Nunito',sans-serif;cursor:pointer;white-space:nowrap;transition:.15s}
.tab:hover{color:var(--text)}
.tab.active{background:var(--green);color:#04221a;border-color:var(--green)}
.tab-panel{display:none}.tab-panel.active{display:block;animation:fade .2s}
@keyframes fade{from{opacity:0;transform:translateY(4px)}to{opacity:1;transform:none}}
.card{background:var(--panel);border:1px solid var(--line);border-radius:var(--r);padding:16px;margin-bottom:12px}
.card-head{display:flex;align-items:center;justify-content:space-between;gap:10px;margin-bottom:12px}
.card-head h2{font-size:14px;font-weight:900;margin:0}
.card-head span{color:var(--muted);font-size:11px}
.btn{border:0;border-radius:11px;padding:11px 15px;color:var(--text);background:#1a2939;font:800 12.5px 'Nunito',sans-serif;cursor:pointer;transition:transform .15s,background .15s}.btn:hover{transform:translateY(-1px)}.btn-primary{background:var(--green);color:#04221a;box-shadow:0 8px 20px rgba(94,234,212,.14)}.btn-danger{background:rgba(255,122,133,.16);color:#ffb3b9;border:1px solid rgba(255,122,133,.3)}.btn-quiet{background:transparent;border:1px solid var(--line);color:var(--muted)}.btn-quiet:hover{color:var(--text)}.btn.full{width:100%}
.scan-toolbar{display:flex;gap:8px;margin-bottom:11px}
.ap-list{display:grid;gap:9px;max-height:460px;overflow:auto;-webkit-overflow-scrolling:touch;overscroll-behavior:contain;padding:2px}
.ap-list:empty:after{content:"Belum ada hasil. Jalankan scan WiFi.";color:var(--muted);font-size:12.5px;padding:14px 4px;text-align:center}
.ap-item{display:grid;grid-template-columns:minmax(0,1fr) auto;gap:5px 12px;padding:14px 15px;border:1px solid var(--line);border-radius:13px;background:var(--panel2);cursor:pointer;transition:border-color .15s,background .15s}.ap-item:hover,.ap-item:active{border-color:var(--green);background:#0f2130}
.ap-item b{overflow:hidden;text-overflow:ellipsis;white-space:nowrap;color:#fff;font-size:14px;font-weight:800}.ap-item small{color:var(--muted);font-size:11px;font-weight:700}
.ap-item .signal{grid-column:1/-1;height:5px;border-radius:10px;background:#1d3142;overflow:hidden}.ap-item .signal i{display:block;height:100%;border-radius:inherit;background:linear-gradient(90deg,var(--cyan),var(--green))}
.scan-note{color:var(--muted);font-size:11.5px;margin:11px 0 0}.danger-note{color:var(--amber);font-size:11.5px;margin:11px 0 0}
.enc-badge{display:inline-block;padding:2px 8px;border-radius:99px;font-size:9.5px;font-weight:900}.enc-badge.wpa3{background:rgba(255,122,133,.14);color:#ff9aa2}.enc-badge.wpa2{background:rgba(255,201,107,.14);color:#ffd166}.enc-badge.open{background:rgba(94,234,212,.12);color:#5eead4}.enc-badge.other{background:rgba(150,190,210,.1);color:#8aa0b2}
.target-warn{margin-top:11px;padding:12px 13px;border:1px solid rgba(255,122,133,.35);border-radius:11px;background:rgba(255,122,133,.09);color:#ffb6bb;font-size:11.5px;font-weight:800}.target-warn.amber{border-color:rgba(255,201,107,.35);background:rgba(255,201,107,.08);color:#ffd166}
.target-box{margin-top:11px;padding:13px;border:1px solid var(--line);border-radius:12px;background:var(--panel2)}
.target-box .tt{display:flex;align-items:center;justify-content:space-between;gap:10px;margin-bottom:9px}
.target-box .tt b{font-size:13.5px;color:#fff}.target-box .tt span{font-size:11px;color:var(--muted)}
.target-actions{display:flex;gap:8px;flex-wrap:wrap}
.mode-grid{display:grid;grid-template-columns:repeat(4,minmax(0,1fr));gap:7px}
.mode{padding:12px 6px;border:1px solid var(--line);border-radius:11px;background:rgba(255,255,255,.02);color:var(--muted);font-size:11.5px;font-weight:800;cursor:pointer;font-family:'Nunito',sans-serif}.mode:hover{color:var(--text);border-color:rgba(103,215,240,.45)}.mode.active{color:#04221a;border-color:var(--green);background:var(--green)}
.action-grid{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:8px}
.action-grid .btn{text-align:left;min-height:54px}.action-grid .btn b{display:block;font-size:12.5px}.action-grid .btn span{display:block;margin-top:3px;color:var(--muted);font-size:10.5px;font-weight:600}
.chart-wrap{border:1px solid var(--line);border-radius:12px;background:var(--panel2);padding:7px}.chart-wrap canvas{display:block;width:100%;height:100px}
.metrics{display:grid;grid-template-columns:repeat(4,minmax(0,1fr));gap:7px;margin-top:11px}.metric{padding:11px 9px;border:1px solid var(--line);border-radius:11px;background:rgba(255,255,255,.02)}.metric b{display:block;color:#fff;font-size:16px;font-weight:900}.metric span{display:block;color:var(--muted);font-size:10.5px;margin-top:1px}
.data-box{min-height:70px;max-height:200px;overflow:auto;white-space:pre-wrap;color:#b9d6df;font:600 11.5px/1.6 ui-monospace,Consolas,monospace;border:1px solid var(--line);border-radius:12px;background:var(--panel2);padding:12px}
.eapol-grid{display:grid;grid-template-columns:repeat(4,minmax(0,1fr));gap:7px;margin-bottom:11px}.eapol-stat{padding:10px 8px;text-align:center;border:1px solid var(--line);border-radius:10px;background:rgba(255,255,255,.02)}.eapol-stat b{display:block;color:var(--cyan);font-size:16px}.eapol-stat span{color:var(--muted);font-size:10.5px}
.form-grid{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:10px 12px}.field label{display:block;color:var(--muted);font-size:10.5px;font-weight:800;margin:0 0 5px}.field input{width:100%;border:1px solid var(--line);border-radius:10px;background:var(--panel2);color:var(--text);padding:10px 11px;font:600 12.5px 'Nunito',sans-serif;outline:none}.field.full{grid-column:1/-1}.form-actions{display:flex;justify-content:flex-end;gap:8px;margin-top:14px}
.exports{display:grid;grid-template-columns:repeat(3,minmax(0,1fr));gap:8px}.exports .btn{text-align:center}
.toast{position:fixed;left:50%;bottom:18px;z-index:50;transform:translate(-50%,15px);opacity:0;pointer-events:none;transition:.2s;padding:12px 15px;border:1px solid rgba(94,234,212,.3);border-radius:11px;background:#0e2a22;color:#d7ffe2;font-size:12px;font-weight:800}.toast.show{opacity:1;transform:translate(-50%,0)}
.footer-note{text-align:center;color:#6e8795;font-size:10.5px;margin-top:20px}.footer-note b{color:var(--green)}
@media(max-width:640px){.metrics,.eapol-grid,.mode-grid{grid-template-columns:repeat(2,minmax(0,1fr))}.action-grid{grid-template-columns:1fr}.form-grid{grid-template-columns:1fr}.exports{grid-template-columns:1fr}.token input{width:100px}}
</style></head><body><div class="app">
<div class="topbar"><div class="brand"><div class="logo">N</div><div><b>NISKALA</b><small>Wireless Control Console</small></div></div><div class="top-actions"><div class="pill"><i></i> AP CONTROL ONLINE</div><div class="token"><input id="web_token" type="password" placeholder="Token" aria-label="Token Web Remote"><button class="btn btn-quiet" onclick="saveToken()">Simpan</button></div></div></div>
<div class="statusbar"><b>Status: <span id="info">Idle</span></b><button class="btn btn-danger" onclick="stopAll()">Stop Semua</button></div>
<nav class="tabs">
<button class="tab active" data-tab="attack">Scan &amp; Serang</button>
<button class="tab" data-tab="modules">Modul</button>
<button class="tab" data-tab="telemetry">Telemetry</button>
<button class="tab" data-tab="eapol">EAPOL</button>
<button class="tab" data-tab="log">Log</button>
<button class="tab" data-tab="config">Config</button>
</nav>
<main>
<section class="tab-panel active" id="panel-attack">
<div class="card"><div class="card-head"><h2>WiFi Scan</h2><span id="scan-count">Pasif</span></div><div class="scan-toolbar"><button class="btn btn-primary full" onclick="scanWifi()">Scan WiFi</button></div><div id="scanResult" class="ap-list"></div><div id="targetNote"></div><div id="targetPanel" class="target-box" style="display:none"><div class="tt"><b id="t-ssid">-</b><span id="t-meta"></span></div><div id="pmfResult" style="font-size:12px;color:var(--muted);margin-bottom:10px">PMF belum dicek. Tekan "Cek PMF" untuk data asli dari beacon.</div><div class="target-actions"><button class="btn btn-quiet" id="btnPmf" onclick="checkPmf()">Cek PMF</button><button class="btn btn-primary" id="launchBtn" onclick="launchAttack()">Mulai DEAUTH</button></div></div><p class="scan-note">▸ Klik salah satu jaringan untuk memilih target. Board hanya mendeteksi 2.4 GHz (jaringan 5G tidak terpengaruh).</p></div>
<div class="card"><div class="card-head"><h2>Mode Serangan</h2><span id="mode-label">DEAUTH</span></div><div class="mode-grid"><button class="mode active" id="m_deauth" onclick="setMode('deauth')">DEAUTH</button><button class="mode" id="m_evil" onclick="setMode('evil')">EVIL TWIN</button><button class="mode" id="m_auth" onclick="setMode('auth')">AUTH FLOOD</button><button class="mode" id="m_csa" onclick="setMode('csa')">CSA</button></div><p class="danger-note">Mode aktif memutus Web Remote. Gunakan lab terisolasi; berhenti pakai tombol board.</p></div>
</section>
<section class="tab-panel" id="panel-modules">
<div class="card"><div class="card-head"><h2>Modul Cepat</h2><span>1 klik</span></div><div class="action-grid"><button class="btn" onclick="start('beacon')"><b>Beacon Spam</b><span>Memutus Web Remote</span></button><button class="btn" onclick="start('probe')"><b>Probe Sniff</b><span>Memutus Web Remote</span></button><button class="btn" onclick="start('global')"><b>Global Deauth</b><span>Memutus Web Remote</span></button><button class="btn" onclick="start('karma')"><b>Karma</b><span>Memutus Web Remote</span></button><button class="btn" onclick="start('ble')"><b>BLE Spam</b><span>WiFi OFF + Remote putus</span></button><button class="btn btn-danger" onclick="stopAll()"><b>Stop All</b><span>Hentikan modul</span></button></div></div>
</section>
<section class="tab-panel" id="panel-telemetry">
<div class="card"><div class="card-head"><h2>System Telemetry</h2><button class="btn btn-quiet" onclick="loadTelemetry()">Refresh</button></div><div class="chart-wrap"><canvas id="chart" width="640" height="180"></canvas></div><div class="metrics"><div class="metric"><b id="metric-packets">0</b><span>Packets</span></div><div class="metric"><b id="metric-errors">0</b><span>TX errors</span></div><div class="metric"><b id="metric-heap">—</b><span>Free heap</span></div><div class="metric"><b id="metric-battery">—</b><span>Battery</span></div></div><div id="telemetryPanel" class="data-box" style="margin-top:11px">Tekan Refresh untuk membaca telemetri.</div></div>
</section>
<section class="tab-panel" id="panel-eapol">
<div class="card"><div class="card-head"><h2>EAPOL Capture</h2><button class="btn btn-quiet" onclick="loadEapol()">Refresh</button></div><div class="eapol-grid"><div class="eapol-stat"><b id="e-m1">0</b><span>M1</span></div><div class="eapol-stat"><b id="e-m2">0</b><span>M2</span></div><div class="eapol-stat"><b id="e-m3">0</b><span>M3</span></div><div class="eapol-stat"><b id="e-m4">0</b><span>M4</span></div></div><div id="eapolPanel" class="data-box">Belum ada data EAPOL.</div></div>
</section>
<section class="tab-panel" id="panel-log">
<div class="card"><div class="card-head"><h2>Event Log</h2><button class="btn btn-quiet" onclick="loadLog()">Refresh</button></div><div id="logPanel" class="data-box">Log masih kosong.</div></div>
</section>
<section class="tab-panel" id="panel-config">
<div class="card"><div class="card-head"><h2>Konfigurasi</h2><span>NVS</span></div><div class="form-grid"><div class="field"><label>Deauth burst</label><input id="cfg_deauth" type="number" min="1" max="100"></div><div class="field"><label>Auth burst</label><input id="cfg_auth" type="number" min="1" max="200"></div><div class="field"><label>Hop interval (ms)</label><input id="cfg_hop" type="number" min="20" max="2000"></div><div class="field"><label>Beacon hop (ms)</label><input id="cfg_beacon" type="number" min="20" max="2000"></div><div class="field"><label>Deauth reason</label><input id="cfg_reason" type="number" min="0" max="16"></div><div class="field"><label>Auto-sync (menit)</label><input id="cfg_autosync" type="number" min="0" max="1440"></div><div class="field"><label>WiFi SSID sync</label><input id="cfg_wifi" type="text"></div><div class="field"><label>WiFi password sync</label><input id="cfg_wifipass" type="password"></div><div class="field full"><label>Server URL</label><input id="cfg_url" type="url" placeholder="https://domain/sync.php"></div><div class="field full"><label>Template URL</label><input id="cfg_tpl" type="url" placeholder="https://domain/template.html"></div></div><div class="form-actions"><button class="btn btn-quiet" onclick="loadConfig()">Muat ulang</button><button class="btn btn-primary" onclick="saveConfig()">Simpan</button></div></div>
<div class="card"><div class="card-head"><h2>Export Data</h2><span>CSV</span></div><div class="exports"><button class="btn" onclick="downloadExport('/api/export/probe','probe.csv')">Probe</button><button class="btn" onclick="downloadExport('/api/export/karma','karma.csv')">Karma</button><button class="btn" onclick="downloadExport('/api/export/passwords','passwords.csv')">Password</button></div></div>
</section>
</main>
<div class="footer-note"><b>AUTHORIZED USE ONLY</b> · NISKALA local control panel · WiFi &amp; BLE berbagi radio</div></div><div id="toast" class="toast"></div>
<script>
document.querySelectorAll('.tab').forEach(t=>t.addEventListener('click',()=>{document.querySelectorAll('.tab').forEach(x=>x.classList.toggle('active',x===t));document.querySelectorAll('.tab-panel').forEach(x=>x.classList.toggle('active',x.id==='panel-'+t.dataset.tab));}));

let WEB_TOKEN=localStorage.getItem('niskala_token')||'';let attackMode='deauth';const $=id=>document.getElementById(id);const pktSamples=[];
function apiFetch(url,opts){opts=opts||{};opts.headers=Object.assign({},opts.headers||{}, {'X-NISKALA-TOKEN':WEB_TOKEN});return fetch(url,opts)}
function toast(msg){const el=$('toast');el.textContent=msg;el.classList.add('show');clearTimeout(window.__toast);window.__toast=setTimeout(()=>el.classList.remove('show'),2400)}
function esc(s){return String(s??'').replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]))}
function saveToken(){WEB_TOKEN=$('web_token').value.trim();localStorage.setItem('niskala_token',WEB_TOKEN);toast('Token disimpan di browser')}
function setMode(m){attackMode=m;['deauth','evil','auth','csa'].forEach(x=>{const b=$('m_'+x);if(b)b.classList.toggle('active',x===m)});$('mode-label').textContent=m.toUpperCase();if($('launchBtn'))$('launchBtn').textContent='Mulai '+m.toUpperCase()}
function rssiPct(v){return Math.max(4,Math.min(100,((Number(v)+90)/60)*100))}
function scanWifi(){
  const list=$('scanResult');
  $('scan-count').textContent='Memindai...';
  list.innerHTML='<div class="ap-item"><div><b>Memindai jaringan...</b><small>Tunggu hingga 5 detik</small></div></div>';
  apiFetch('/api/scan').then(r=>{
    if(r.status===403)throw Object.assign(new Error('auth'),{auth:true});
    if(!r.ok)throw Error();
    return r.json();
  }).then(data=>{
    if(!Array.isArray(data))throw Error();
    let html='';
    data.forEach(ap=>{const ssid=ap.ssid||'(hidden)';const auth=Number(ap.auth||0);const enc=ap.enc||'?';let cls='other';let label=enc;if(auth===6||auth===7||auth===9){cls='wpa3';label='PMF · '+enc;}else if(auth===3||auth===4||auth===5){cls='wpa2';label=enc;}else if(auth===0){cls='open';label='OPEN';}html+=`<div class="ap-item" data-mac="${esc(ap.mac)}" data-ch="${esc(ap.ch)}" data-ssid="${esc(ssid)}" data-auth="${auth}" data-enc="${esc(enc)}"><div><b>${esc(ssid)}</b><small>${esc(ap.mac)} | Ch ${esc(ap.ch)}</small></div><div style="display:flex;flex-direction:column;align-items:flex-end;gap:3px"><small>${esc(ap.rssi)} dBm</small><span class="enc-badge ${cls}">${esc(label)}</span></div><div class="signal"><i style="width:${rssiPct(ap.rssi)}%"></i></div></div>`});
    list.innerHTML=html;
    $('scan-count').textContent=(data.length?data.length+' AP ditemukan':'Tidak ada AP');
    if(!data.length)list.innerHTML='<div class="ap-item"><div><b>Tidak ada jaringan ditemukan</b><small>Board hanya mendeteksi 2.4 GHz. Pindah posisi lalu scan ulang.</small></div></div>';
    toast('Scan selesai');
  }).catch(err=>{
    if(err && err.auth){
      list.innerHTML='<div class="ap-item"><div><b>Token Web Remote belum sesuai</b><small>Isi token di kolom atas, lalu tekan Simpan, baru scan ulang.</small></div></div>';
      $('scan-count').textContent='Perlu token';
      toast('Token Web Remote diperlukan');
    }else{
      list.innerHTML='<div class="ap-item"><div><b>Scan gagal</b><small>Pastikan perangkat terhubung ke NISKALA_CTRL.</small></div></div>';
      $('scan-count').textContent='Gagal scan';
      toast('Tidak dapat membaca scan');
    }
  });
}

function stopAll(){apiFetch('/api/stop').then(()=>{toast('Semua modul dihentikan');$('info').textContent='Stopped';}).catch(()=>toast('Stop gagal'))}
function drawChart(data){const c=$('chart'),ctx=c.getContext('2d'),w=c.width,h=c.height;ctx.clearRect(0,0,w,h);ctx.strokeStyle='rgba(184,215,232,.1)';ctx.lineWidth=1;for(let y=1;y<4;y++){ctx.beginPath();ctx.moveTo(0,y*h/4);ctx.lineTo(w,y*h/4);ctx.stroke()}if(data.length<2)return;const max=Math.max(...data,1);ctx.beginPath();data.forEach((v,i)=>{const x=8+i/(data.length-1)*(w-16),y=h-10-(v/max)*(h-20);i?ctx.lineTo(x,y):ctx.moveTo(x,y)});ctx.strokeStyle='#78f09b';ctx.lineWidth=3;ctx.stroke()}
function loadTelemetry(){apiFetch('/api/telemetry').then(r=>r.json()).then(t=>{$('metric-packets').textContent=t.packets??0;$('metric-errors').textContent=t.tx_errors??0;$('metric-heap').textContent=(t.heap??0)+' B';$('metric-battery').textContent=(t.battery??0)+'%';$('telemetryPanel').textContent='State: '+(t.state??'-')+'  ·  Mode: '+(t.mode??'-')+'\nHeap min: '+(t.heap_min??0)+' B  ·  Stack: '+(t.stack??0)+' B\nVoltage: '+(t.battery_v??0)+' V  ·  Temp: '+(t.temp??0)+' °C\nWiFi mode: '+(t.wifi_mode??'-')+'  ·  Channel: '+(t.wifi_channel??'-')+'\nUptime: '+(t.uptime_s??0)+' s'}).catch(()=>{$('telemetryPanel').textContent='Gagal mengambil telemetri'})}
function loadEapol(){apiFetch('/api/eapol').then(r=>r.json()).then(e=>{$('e-m1').textContent=e.m1??0;$('e-m2').textContent=e.m2??0;$('e-m3').textContent=e.m3??0;$('e-m4').textContent=e.m4??0;$('eapolPanel').textContent='Target: '+(e.target||'-')+'\nBSSID: '+(e.bssid||'-')+'  ·  Channel: '+(e.channel??0)+'\nFULL: '+(e.full??0)+'  ·  EAPOL: '+(e.packets??0)+'\nUptime: '+(e.uptime_s??0)+' s'}).catch(()=>{$('eapolPanel').textContent='Gagal mengambil data EAPOL'})}
function loadLog(){apiFetch('/api/log').then(r=>r.text()).then(t=>{$('logPanel').textContent=t||'(kosong)'}).catch(()=>{$('logPanel').textContent='Gagal mengambil event log'})}
function downloadExport(url,name){apiFetch(url).then(r=>{if(!r.ok)throw Error();return r.blob()}).then(blob=>{const a=document.createElement('a');a.href=URL.createObjectURL(blob);a.download=name;a.click();setTimeout(()=>URL.revokeObjectURL(a.href),1000);toast('Export '+name+' siap')}).catch(()=>toast('Export gagal'))}
function poll(){apiFetch('/api/status').then(r=>r.json()).then(s=>{$('info').textContent='Mode '+(s.mode??'-')+' · '+(s.packets??0)+' pkt · '+(s.heap??0)+' B';pktSamples.push(Number(s.packets)||0);if(pktSamples.length>60)pktSamples.shift();drawChart(pktSamples);loadTelemetry();loadEapol()}).catch(()=>{$('info').textContent='Menunggu board…'})}
let selTarget=null,pmfPollTimer=null;
$('scanResult').addEventListener('click',e=>{
  const item=e.target.closest('.ap-item');if(!item)return;
  const mac=item.dataset.mac,ch=item.dataset.ch,ssid=item.dataset.ssid;
  const auth=Number(item.dataset.auth||0),enc=item.dataset.enc||'?';
  selTarget={mac:mac,ch:ch,ssid:ssid,auth:auth,enc:enc};
  const isWpa3=(auth===6||auth===7||auth===9);
  const isWpa2=(auth===3||auth===4||auth===5);
  $('t-ssid').textContent=ssid;$('t-meta').textContent=mac+' | Ch '+ch+' | '+enc;
  $('targetPanel').style.display='block';
  $('launchBtn').textContent='Mulai '+attackMode.toUpperCase();
  if(isWpa3){$('pmfResult').innerHTML='<span style="color:#ff9aa2">PMF: YA (WPA3 wajib PMF). Deauth tidak akan berhasil.</span>';}
  else{$('pmfResult').innerHTML='PMF belum dicek. Tekan "Cek PMF" untuk data asli dari beacon.';}
  let warn='';
  if(isWpa3){warn='WPA3/PMF wajib aktif. Deauth 2.4 GHz TIDAK akan berhasil. Gunakan CSA dari board.';}
  else if(isWpa2){warn='WPA2 bisa saja mengaktifkan PMF. Cek PMF dulu sebelum deauth.';}
  $('targetNote').innerHTML=warn?`<div class="target-warn${isWpa2?' amber':''}">${esc(warn)}</div>`:'';
  $('info').textContent='Target: '+ssid;
});
function checkPmf(){
  if(!selTarget)return;
  const b=$('btnPmf');b.disabled=true;b.textContent='Mengecek...';
  $('pmfResult').innerHTML='<span style="color:var(--amber)">Menangkap beacon... AP kontrol sempat turun ~1 dtk. Hasil muncul di layar board & web.</span>';
  apiFetch('/api/pmf?mac='+encodeURIComponent(selTarget.mac)+'&ch='+encodeURIComponent(selTarget.ch)+'&ssid='+encodeURIComponent(selTarget.ssid)).then(r=>r.json()).then(d=>{if(d&&typeof d.pmf==='boolean'){renderPmf(d.pmf,d.found);stopPmfPoll();}else{startPmfPoll();}}).catch(()=>startPmfPoll());
}
function startPmfPoll(){let tries=0;pmfPollTimer=setInterval(()=>{tries++;apiFetch('/api/pmf-result').then(r=>r.json()).then(d=>{if(d&&d.checked){renderPmf(d.pmf,d.found);stopPmfPoll();}else if(tries>=24){renderPmfUnknown();stopPmfPoll();}}).catch(()=>{if(tries>=24){renderPmfUnknown();stopPmfPoll();}});},1500);}
function stopPmfPoll(){if(pmfPollTimer){clearInterval(pmfPollTimer);pmfPollTimer=null;}const b=$('btnPmf');if(b){b.disabled=false;b.textContent='Cek PMF';}}
function renderPmf(pmf,found){if(!found){$('pmfResult').innerHTML='<span style="color:var(--amber)">Beacon tidak ditemukan pada kanal itu. Pastikan target 2.4 GHz lalu scan ulang.</span>';return;}if(pmf){$('pmfResult').innerHTML='<span style="color:#ff9aa2">PMF: YA (802.11w aktif). Deauth tidak akan memutus.</span>';}else{$('pmfResult').innerHTML='<span style="color:#78f09b">PMF: TIDAK (data asli beacon). Deauth bisa dicoba.</span>';}}
function renderPmfUnknown(){$('pmfResult').innerHTML='<span style="color:var(--amber)">Cek PMF gagal / timeout. Periksa koneksi ke NISKALA_CTRL lalu coba lagi.</span>';}
function launchAttack(){
  if(!selTarget)return;
  if((selTarget.auth===6||selTarget.auth===7||selTarget.auth===9)&&!confirm('Target WPA3/PMF. Deauth TIDAK akan berhasil. Tetap jalankan '+attackMode.toUpperCase()+'?'))return;
  apiFetch('/api/attack?mac='+encodeURIComponent(selTarget.mac)+'&ch='+encodeURIComponent(selTarget.ch)+'&ssid='+encodeURIComponent(selTarget.ssid)+'&mode='+encodeURIComponent(attackMode)).then(()=>toast(attackMode.toUpperCase()+' dimulai')).catch(()=>toast('Gagal memulai mode'));
}

function start(m){$('info').textContent='Starting '+m+'…';apiFetch('/api/start/'+m).then(()=>toast(m.toUpperCase()+' dimulai')).catch(()=>toast('Gagal memulai '+m))}
function loadConfig(){apiFetch('/api/config').then(r=>r.json()).then(c=>{['deauth','auth','hop','beacon','reason','autosync'].forEach(k=>{if($("cfg_"+k))$("cfg_"+k).value=c[k]??0});$('cfg_wifi').value=c.wifi||'';$('cfg_wifipass').value=c.wifipass||'';$('cfg_url').value=c.url||'';$('cfg_tpl').value=c.tpl||''}).catch(()=>{})}
function saveConfig(){const p=new URLSearchParams();[['deauth','cfg_deauth'],['auth','cfg_auth'],['hop','cfg_hop'],['beacon','cfg_beacon'],['reason','cfg_reason'],['wifi','cfg_wifi'],['wifipass','cfg_wifipass'],['url','cfg_url'],['tpl','cfg_tpl'],['autosync','cfg_autosync']].forEach(([k,id])=>p.set(k,$(id).value));apiFetch('/api/config',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:p.toString()}).then(()=>toast('Konfigurasi tersimpan')).catch(()=>toast('Gagal menyimpan konfigurasi'))}
$('web_token').value=WEB_TOKEN;setMode('deauth');loadConfig();loadTelemetry();loadEapol();setInterval(poll,3000);

</script></body></html>

)rawliteral";

#endif
