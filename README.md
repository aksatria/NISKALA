# NISKALA

### Network Intrusion & Security Kit for Analysis, Learning & Assessment

![Version](https://img.shields.io/badge/version-3.1-brightgreen)  
![Platform](https://img.shields.io/badge/platform-ESP32-blue)  
![License](https://img.shields.io/badge/license-Educational-red)

## Project Structure

```text
TTGO-T-Display-ESP32/
|-- .pio/
|-- .vscode/
|-- lib/
|-- src/
|   |-- attack_engine.h
|   |-- bt_spam.h
|   |-- disclaimer.h
|   |-- karma.h
|   |-- main.cpp
|   |-- templates.h
|   `-- wifi_bypass.c
|-- .gitignore
|-- LICENSE
|-- platformio.ini
`-- README.md
```

> Folder `.pio/` dan `.vscode/` digunakan secara lokal dan diabaikan oleh Git.

## PERINGATAN HUKUM — BACA DULU
NISKALA adalah alat audit keamanan WiFi untuk tujuan EDUKASI dan PENGUJIAN.

Alat ini HANYA BOLEH digunakan pada:

Jaringan milik sendiri, ATAU

Jaringan yang Anda punya izin tertulis dari pemiliknya.

DILARANG KERAS menggunakan alat ini untuk:

Menyerang WiFi orang lain tanpa izin

Mencuri kredensial / data pribadi

Mengganggu layanan publik (illegal interference)

Di Indonesia, tindakan tersebut dapat dikenakan UU ITE Pasal 30 & 46 serta UU Telekomunikasi, dengan ancaman pidana penjara hingga 6–8 tahun dan/atau denda miliaran rupiah.

Penulis tidak bertanggung jawab atas penyalahgunaan alat ini. Semua risiko ditanggung pengguna.

## 📖 Tentang NISKALA
NISKALA (dibaca: nis-ka-la) adalah firmware penetration testing WiFi berbasis ESP32 dengan layar TFT dan navigasi 2 tombol. Nama ini diambil dari kata Sanskerta niścala (निश्चल) yang berarti "tenang, tak tergoyahkan" — melambangkan alat yang diam-diam mengumpulkan informasi tanpa terdeteksi.

Alat ini menggabungkan sembilan modul serangan WiFi dan Bluetooth ke dalam satu antarmuka ringkas dengan animasi real-time, cocok untuk:

Audit keamanan jaringan pribadi atau kantor

Pelatihan blue team (red team simulation)

Riset keamanan wireless (WPA2/WPA3, PMF, captive portal)

CTF dan lab cybersecurity

## ✨ Fitur Lengkap
#	Modul	Deskripsi
1	Deauth Manual	Kirim paket deauthentication ke target AP dengan tracking MAC korban otomatis
2	Evil Twin	Clone SSID target + captive portal palsu untuk menangkap password WiFi
3	Beacon Spam	Banjiri udara dengan 16 SSID palsu (FBI Surveillance, Free WiFi, dll)
4	Probe Sniff	Tangkap probe request dari HP sekitar (MAC, RSSI, SSID yang dicari)
5	Global Deauth	Broadcast deauth dengan channel hopping 1–13
6	Auth Flood	Banjiri AP target dengan authentication request palsu (MAC acak)
7	Bluetooth Spam	Rotasi 3 payload Apple BLE (Sour Apple, Nearby Setup, Nearby WiFi)
8	Password Vault	Lihat & kelola password yang berhasil tertangkap (tersimpan di NVS)
9	Karma Attack	Respond ke probe request klien dengan SSID yang mereka cari
### Fitur Pendukung

✅ Deteksi PMF/WPA3 — modul deauth otomatis menolak target yang dilindungi PMF

✅ Verifikasi password — Evil Twin mencoba login ke AP asli untuk memvalidasi hasil tangkapan (maks. 5 percobaan)

✅ Persistent storage — password tersimpan di NVS (Non-Volatile Storage), tetap ada setelah reboot

✅ Auto-release — setelah password benar, korban dilepas kembali ke AP asli

✅ Animasi UI — radar scan, progress bar, corner brackets ala sci-fi

✅ Disclaimer berlapis — 2 layar konfirmasi sebelum masuk menu

## 🛠️ Kebutuhan Hardware
Komponen	Spesifikasi	Keterangan
MCU	ESP32 (ESP32-WROOM / DevKit)	Wajib mendukung WiFi + BLE
Display	TFT 1.8" ST7735 128×160 atau 2.8" ILI9341 240×320	Konfigurasi di TFT_eSPI/User_Setup.h
Tombol	2× tactile button	BTN_TOP = GPIO 0 (boot button), BTN_BOTTOM = GPIO 35
Catu daya	USB 5V atau Li-Po 3.7V + boost	Konsumsi ~200–350 mA saat aktif
Opsional	Antena eksternal 2.4 GHz	Untuk jangkauan lebih luas
### Wiring Tombol

```
BTN_TOP    ──── GPIO 0   ──── GND   (internal pull-up)
BTN_BOTTOM ──── GPIO 35  ──── GND   (internal pull-up)
TFT        ──── SPI (lihat User_Setup.h TFT_eSPI)
## 🚀 Instalasi & Build
1. Prasyarat
Arduino IDE 2.x atau PlatformIO

Board package ESP32 versi 2.0.11 atau lebih baru (untuk esp_wifi_80211_tx)

Library berikut (install via Library Manager):

```
TFT_eSPI       by Bodmer
ESP32 BLE      (bundled dengan board package)
DNSServer, WebServer, Preferences, WiFi, ESPmDNS  (bundled)
2. Konfigurasi TFT_eSPI
Edit libraries/TFT_eSPI/User_Setup.h sesuai layar Anda. Contoh untuk ST7735 1.8":

```cpp
#define ST7735_DRIVER
#define TFT_WIDTH  128
#define TFT_HEIGHT 160
#define ST7735_GREENTAB
#define TFT_MOSI 23
#define TFT_SCLK 18
#define TFT_CS   15
#define TFT_DC    2
#define TFT_RST   4
#define TFT_BL   32
Ubah tft.setRotation(1) di setup() jika orientasi layar berbeda. Firmware dirancang untuk landscape 240×135.

3. Build firmware
Firmware terdiri dari file berikut (semua harus di folder sketch yang sama):

```
NISKALA/
├── NISKALA.ino       (rename dari main.cpp)
├── attack_engine.h   (mesin serangan + frame builder)
├── karma.h           (logika Karma attack)
├── bt_spam.h         (BLE advertising rotator)
├── disclaimer.h      (layar disclaimer & konfirmasi)
├── templates.h       (HTML captive portal)
└── wifi_bypass.c     (bypass sanity check esp_wifi)
Penting: File wifi_bypass.c berisi stub ieee80211_raw_frame_sanity_check yang di-override untuk melewati pemeriksaan internal IDF. Tanpa file ini, esp_wifi_80211_tx akan menolak frame deauth di IDF versi baru. Pastikan file .c ini ikut di-compile.

4. Flash
```bash
# Arduino IDE: pilih board "ESP32 Dev Module", upload.
# PlatformIO:
pio run -t upload
## 📱 Cara Pemakaian
### Boot Sequence
```
1. Splash screen "NISKALA v3.1" — loading bar
2. Disclaimer hukum (layar merah)
   → Tekan tombol apa saja untuk lanjut
3. Konfirmasi tanggung jawab (layar oranye)
   → ATAS = Batal (ulang disclaimer)
   → BAWAH = Setuju (masuk menu utama)
4. Menu Utama muncul
### Navigasi Dasar
Tombol	Fungsi
ATAS (GPIO 0)	Scroll pilihan ke atas / pilih item sebelumnya
BAWAH (GPIO 35)	Konfirmasi / pilih item / start-stop
### Modul per Modul
🎯 1. Deauth Manual
```
Menu → DEAUTH MANUAL → (scan AP) → pilih AP target → tekan BAWAH untuk start
Setelah start, NISKALA akan:

Set MAC AP-nya sama dengan BSSID target
Buat AP tersembunyi Deauther_Fix
Mulai promiscuous mode untuk track MAC korban
Kirim deauth burst (5 paket) ke MAC korban yang terdeteksi
Layar menampilkan paket terkirim & error count

Tekan BAWAH = Stop / Start ulang · ATAS = Keluar

⚠️ Jika target WPA3/PMF terdeteksi, menu akan menolak sebelum start.

🎭 2. Evil Twin (Captive Portal)
```
Menu → EVIL TWIN → pilih AP target → pilih template → auto-start
Alur lengkap:

NISKALA memindai AP di sekitar, menampilkan daftar SSID

Pilih target (SSID akan di-clone persis)

Pilih salah satu template portal (generic, router, dll — lihat templates.h)

AP palsu dibuat dengan SSID sama seperti target

DNS server + web server aktif → portal terbuka otomatis di HP korban yang connect

Deauth dikirim terus ke target untuk memaksa klien pindah ke AP palsu

Korban memasukkan password di portal palsu

NISKALA otomatis:

Simpan password ke NVS

Verifikasi dengan mencoba connect ke AP asli (6 detik/percobaan, maks 5×)

Jika benar → tampilkan PASSWORD BENAR → lepas korban

Jika salah → kembali ke mode Evil Twin (5× salah → target dilepas)

Layar berjalan:

Menampilkan jumlah klien (KORBAN: X HP)

MENEMBAK DEAUTH — animasi radar saat memburu klien

Setelah klien masuk → Menunggu password...

Template HTML ada di templates.h. Anda bisa tambahkan template baru (misalnya halaman login kantor, hotel, kafe) dengan mengikuti pola html_generic.

📡 3. Beacon Spam
```
Menu → BEACON SPAM → auto-start
Broadcast 16 SSID palsu bergiliran tiap BEACON_HOP_MS (100 ms):

Free WiFi, MikroTik, IndiHome, FBI Surveillance, Virus.exe, dll.

Channel hop otomatis 1→13

Bisa membuat HP sekitar menampilkan puluhan jaringan palsu (troll / distraction)

🔍 4. Probe Sniff
```
Menu → PROBE SNIFF → auto-start
Capture probe request dari HP/device sekitar

Menampilkan 4 entri terakhir: MAC — RSSI — SSID yang dicari

Cocok untuk wardriving pasif — mengintip nama WiFi yang dicari orang

Daftar SSID target juga bisa jadi bahan untuk Karma attack (lihat modul 9)

🌐 5. Global Deauth
```
Menu → GLOBAL DEAUTH → auto-start
Broadcast deauth ke alamat broadcast (FF:FF:FF:FF:FF:FF)

Channel hopping 1–13

Tidak menargetkan AP tertentu (wide-area denial)

Dampak luas, gunakan hanya di lab (mudah ketahuan & melanggar hukum)

💥 6. Auth Flood
```
Menu → AUTH FLOOD → pilih AP target → auto-start
Kirim 20 auth frame per 10 ms dengan MAC sumber acak

Membanjiri client table AP target → AP bisa freeze / deny service ke klien asli

Menampilkan hitungan paket real-time

📶 7. Bluetooth Spam
```
Menu → BLUETOOTH SPAM → auto-start
Mematikan WiFi, mengaktifkan BLE

Rotasi 3 payload Apple BLE (mirip Flipper Zero / Sour Apple):

Sour Apple (0x07 0x19) — trigger popup AirPods
Apple Nearby Setup (0x0F 0x05 0xC1 0x09)
Apple Nearby WiFi (0x0F 0x05 0xC1 0x0B)
Interval rotasi 80 ms

Efek: HP di sekitar menampilkan popup pairing palsu / notification spam

🔐 8. Password Vault
```
Menu → PASSWORD
Menampilkan daftar email | password | status

Status: OK (password terverifikasi), XX (salah), ? (belum diverifikasi)

Scroll: ATAS = naik · BAWAH = OK

Menu HAPUS SEMUA DATA membutuhkan konfirmasi ganda (tekan 2×)

Data tersimpan di NVS dengan key pwd_0, pwd_1, dst.

Format data di NVS: email|password|1 (1 = terverifikasi, 0 = salah)

🕸️ 9. Karma Attack
```
Menu → KARMA ATTACK → auto-scan 20 detik → pilih SSID target → auto-start Evil Twin
Cara kerja:

NISKALA masuk mode Karma Scan — mendengarkan probe request dari HP

Setiap SSID yang muncul di probe dicatat di g_karmaSSIDs[] (16 slot, LRU)

Setiap klien juga dicatat di g_karmaClients[] (16 slot)

Setelah 20 detik → hasil diurutkan berdasarkan jumlah probe

Pilih salah satu SSID → NISKALA mencari AP asli dengan SSID itu

Jika ketemu → masuk Evil Twin dengan SSID tersebut

Jika tidak ketemu → tetap masuk Evil Twin (tanpa deauth target — hanya captive portal saja)

Keunggulan:

Tidak perlu scan AP duluan — langsung "menangkap" SSID yang dicari klien

Broadcast probe (SSID kosong) direspons dengan 5 SSID curated: Free WiFi, Starbucks WiFi, IndiHome, MikroTik, TP-Link

Channel hopping 1→6→11 tiap 300 ms (channel umum)

## 🏗️ Arsitektur Kode
```
┌─────────────────────────────────────────────────┐
│                    main.cpp                     │
│  State machine + UI rendering + button handler  │
└──────────────┬──────────────────────────────────┘
               │
   ┌───────────┼───────────────┬─────────────┐
   │           │               │             │
┌──▼───┐  ┌────▼────┐  ┌───────▼────┐  ┌────▼─────┐
│attack│  │ karma.h │  │ bt_spam.h  │  │templates │
│engine│  │         │  │            │  │   .h     │
│  .h  │  │         │  │            │  │          │
└──┬───┘  └────┬────┘  └────────────┘  └──────────┘
   │           │
   ▼           ▼
┌──────────────────────────────────────┐
│   FreeRTOS Task "attack" (Core 0)    │
│   - Loop tiap 10 ms                  │
│   - Baca g_attackMode → dispatch     │
│   - Kirim frame via esp_wifi_80211_tx│
└──────────────────────────────────────┘
### Task split:

Core 0 — attackTask() mengirim frame berkecepatan tinggi

Core 1 — loop() utama, rendering TFT, tombol, web server

Komunikasi — via volatile global (g_attackMode, g_packetsSent, dst.)

Sniffer callback (packetSniffer_cb) dipanggil dari task WiFi IDF. Callback wajib singkat — hanya parse header + delegasi ke karmaLog* / tracking korban.

## 🧩 Tuning & Kustomisasi
Konstanta	File	Default	Efek
DEAUTH_BURST	attack_engine.h	5	Paket deauth per burst
AUTH_BURST	attack_engine.h	20	Auth frame per 10 ms
HOP_INTERVAL_MS	attack_engine.h	120	Interval channel hop global deauth
BEACON_HOP_MS	attack_engine.h	100	Interval channel hop beacon
PROBE_LOG_SIZE	attack_engine.h	8	Buffer probe log
SPAM_SSIDS	attack_engine.h	16 SSID	Pool SSID beacon spam
CURATED_SSIDS	karma.h	5 SSID	Balasan broadcast probe
KARMA_MAX_SSIDS	karma.h	16	Slot karma SSID
KARMA_SCAN_DURATION_MS	karma.h	20000	Durasi karma scan
MAX_VERIFY_ATTEMPTS	main.cpp	5	Batas percobaan verifikasi password
VERIFY_TIMEOUT_MS	main.cpp	6000	Timeout tiap percobaan koneksi
## ⚠️ Batasan & Catatan Teknis
PMF (Protected Management Frames) memblokir deauth — NISKALA mendeteksi WPA3/WPA2-WPA3 dan menolak target (untuk mencegah kegagalan).

esp_wifi_80211_tx butuh AP aktif — semua modul TX memanggil WiFi.softAP() dulu, walau dengan SSID tersembunyi.

Beberapa modul pakai MAC spoof — esp_wifi_set_mac(WIFI_IF_AP, ...) butuh mode WIFI_AP_STA. Bisa gagal jika MAC belum di-disconnect dari driver.

Bluetooth & WiFi tidak bisa jalan bersamaan di ESP32 (satu radio). BT_SPAM mematikan WiFi (WIFI_OFF).

Heap usage — dengan semua modul aktif, sisa heap ~100 KB. Web server + DNS + TFT butuh margin, jangan tambah modul besar tanpa profiling.

wifi_bypass.c wajib ada jika ESP-IDF ≥ 4.4 — tanpa itu, frame akan ditolak dengan ESP_ERR_INVALID_ARG.

## 📋 Roadmap Ide (untuk pengembangan lanjutan)
□ Handshake capture (WPA2 4-way handshake) untuk crack offline
□ PMKID attack
□ Portal template dengan JavaScript yang lebih meyakinkan (bank, kampus)
□ Web dashboard lokal (monitoring dari HP sendiri)
□ Notifikasi Telegram saat password tertangkap
□ Opsi export password ke CSV via SD card
□ Mode "silent" — matikan animasi untuk stealth
## 🧑‍💻 Kredit & Referensi
Nama: NISKALA (निश्चल — "tak tergoyahkan")

Konsep Karma & Evil Twin — inspirasi dari Spacehuhn, Flipper Zero, dan ESP32 Marauder

Frame 802.11 — referensi dari IEEE 802.11-2020 spec

Library — TFT_eSPI by Bodmer, ESP32 Arduino Core

## 📜 Lisensi
Educational Use Only. Kode disediakan "as-is", tanpa jaminan apa pun. Anda bertanggung jawab penuh atas segala konsekuensi hukum dari penggunaan.

```
Copyright (c) 2024 NISKALA
Permission granted for educational, research, and authorized 
security testing purposes only. Any commercial or malicious 
use is strictly prohibited.
<div align="center">
NISKALA v3.1 — "Tenang, tak tergoyahkan, tak terdeteksi."

⚡ Dibuat untuk yang belajar, bukan untuk yang menyakiti ⚡

</div>
```
