# NISKALA

<div align="center">

### Network Intrusion & Security Kit for Analysis, Learning & Assessment

![Version](https://img.shields.io/badge/version-3.1-brightgreen)
![Platform](https://img.shields.io/badge/platform-ESP32-blue)
![Framework](https://img.shields.io/badge/framework-Arduino-00979D)
![License](https://img.shields.io/badge/license-Educational-red)

**Firmware audit keamanan wireless berbasis ESP32 untuk lab, edukasi, dan pengujian berizin.**

</div>

> [!CAUTION]
> **Gunakan hanya pada perangkat dan jaringan milik sendiri atau dengan izin tertulis.**
> Jangan gunakan untuk mengganggu jaringan publik, mencuri data, atau menguji target tanpa otorisasi. Pengguna bertanggung jawab penuh atas penggunaan project ini.

## Daftar Isi

- [Tentang Project](#tentang-project)
- [Fitur](#fitur)
- [Struktur Project](#struktur-project)
- [Hardware](#hardware)
- [Instalasi](#instalasi)
- [Penggunaan](#penggunaan)
- [Arsitektur](#arsitektur)
- [Konfigurasi](#konfigurasi)
- [Batasan](#batasan)
- [Roadmap](#roadmap)
- [Lisensi](#lisensi)

## Tentang Project

**NISKALA** (dibaca: *nis-ka-la*) adalah firmware ESP32 dengan layar TFT dan navigasi dua tombol. Project ini dibuat untuk membantu pembelajaran keamanan WiFi/BLE melalui eksperimen terkontrol di laboratorium.

Cocok untuk:

- Audit keamanan jaringan pribadi.
- Pelatihan blue team dan simulasi red team.
- Riset keamanan wireless dan manajemen frame.
- CTF serta lab cybersecurity.

## Fitur

| Modul | Ringkasan |
|---|---|
| Deauth Manual | Pengujian management frame pada jaringan lab. |
| Evil Twin | Simulasi captive portal untuk security awareness. |
| Beacon Spam | Pengujian respons perangkat terhadap SSID uji. |
| Probe Sniff | Observasi probe request (channel hop 1/6/11, buffer 32). |
| Global Deauth | Pengujian ketahanan jaringan di lab terisolasi. |
| Auth Flood | Pengujian ketahanan tabel autentikasi AP lab. |
| Bluetooth Spam | Pengujian respons perangkat terhadap BLE advertisement. |
| Password Vault | Penyimpanan hasil pengujian di NVS. |
| Karma Attack | Simulasi respons terhadap probe request. |
| CSA Attack | Pengujian channel switch announcement (beacon/action frame). |
| Web Remote | Kendali scan & serang dari browser HP via softAP. |
| BLE Scan | Pemetaan perangkat BLE sekitar + MAC tracking. |
| OTA Update | Flashing firmware via WiFi tanpa cabut USB. |
| Cloud Sync | Kirim hasil pengujian (Probe/Karma/Password) ke hosting. |
| Client Monitor | Lihat klien/station terhubung ke AP target + deauth presisi. |

### Fitur Pendukung

- Serial CLI via Serial Monitor 115200 (ketik help).
- Macro attack sequence (RECON & ASSAULT).
- BLE raw advertisement replay, iBeacon & Eddystone spoof.
- Client Monitor: lihat klien terhubung & deauth presisi per MAC.

- Deteksi PMF/WPA3 presisi (authmode + parsing RSN capabilities beacon) sebelum pengujian management frame.
- Persistent storage menggunakan NVS.
- UI TFT dengan radar, progress bar, dan animasi.
- Disclaimer berlapis sebelum menu utama.
- Konfigurasi modul terpusat di folder `src/`.
- Web remote control dari browser HP.
- OTA update via WiFi (menu `OTA UPDATE`).
- Cloud Sync hasil pengujian ke hosting (menu `CLOUD SYNC`).

## Struktur Project

```text
TTGO-T-Display-ESP32/
├── .pio/                 # Output build PlatformIO (diabaikan Git)
├── .vscode/              # Konfigurasi lokal VS Code (diabaikan Git)
├── lib/
├── src/
│   ├── attack_engine.h   # Engine dan frame builder
│   ├── bt_spam.h         # Modul BLE
│   ├── disclaimer.h      # Layar disclaimer
│   ├── karma.h           # Logika Karma
│   ├── main.cpp          # State machine dan UI utama
│   ├── templates.h       # Template HTML
│   ├── web_remote.h      # Web control (remote scan/serang)
│   └── wifi_bypass.c     # Komponen kompatibilitas build
├── server/
│   ├── sync.php          # Endpoint Cloud Sync (POST JSON)
│   ├── view.php          # Dashboard viewer hasil sync
│   ├── template.html     # Template captive portal dari hosting
│   └── data/
│       └── .htaccess     # Proteksi akses langsung folder data
├── .gitignore
├── LICENSE
├── platformio.ini
└── README.md
```

## Hardware

| Komponen | Spesifikasi |
|---|---|
| Board | TTGO T-Display / ESP32 yang kompatibel |
| Display | TFT ST7789 135×240 |
| Tombol atas | GPIO 0 |
| Tombol bawah | GPIO 35 |
| Daya | USB 5 V atau Li-Po sesuai board |

Tidak perlu wiring display tambahan pada TTGO T-Display karena layar dan tombol sudah terpasang pada board.

## Instalasi

### Prasyarat

- VS Code + PlatformIO atau Arduino IDE 2.x.
- Platform ESP32 versi 2.0.11 atau lebih baru.
- Library `TFT_eSPI` oleh Bodmer.

### Build dengan PlatformIO

1. Clone repository.
2. Buka folder project di VS Code.
3. Buka PlatformIO dan pilih environment `ttgo-t-display`.
4. Jalankan build:

```bash
pio run
```

5. Hubungkan board, lalu upload:

```bash
pio run -t upload
pio device monitor
```

Konfigurasi board, pin TFT, partition, dan dependency berada di `platformio.ini`.

## Penggunaan

### Navigasi

| Tombol | Fungsi |
|---|---|
| GPIO 0 | Pilih item sebelumnya / scroll ke atas |
| GPIO 35 | Konfirmasi / start / stop |

### Urutan Boot

1. Splash screen NISKALA.
2. Disclaimer hukum.
3. Konfirmasi tanggung jawab.
4. Menu utama.

> Jalankan modul hanya setelah memastikan kanal, perangkat, dan jaringan yang digunakan memang berada dalam scope pengujian.

<details>
<summary><strong>Detail modul pengujian (16 modul)</strong></summary>

### 1. Deauth Manual
Mengirim frame deautentikasi ke satu AP target untuk memutus klien yang terhubung.

**Contoh kasus:** memeriksa apakah AP di lab rentan terhadap deauth; atau memaksa satu perangkat tertentu putus lalu mengamati proses reconnect-nya.

**Cara pakai:** pilih `DEAUTH MANUAL` → scan → pilih AP target → tombol bawah untuk start/stop. Perhatikan angka `P:` (paket terkirim) dan `E:` (error) di kanan layar.

**Catatan:** target yang terdeteksi PMF/WPA3 akan ditolak karena deauth tidak efektif.

### 2. Evil Twin / Captive Portal
Membuat AP palsu dengan SSID yang sama dengan target dan captive portal untuk menangkap kredensial.

**Contoh kasus:** simulasi serangan phishing WiFi untuk melatih karyawan/klien agar tidak asal memasukkan password ke portal mencurigakan.

**Cara pakai:** pilih `EVIL TWIN` → scan → pilih AP target → pilih template (Generic/IndiHome/Facebook/MikroTik/Google/Hosting) → korban terhubung ke AP palsu → password yang dimasukkan diverifikasi ke AP asli. Template dikelola di `src/templates.h`.

**Catatan:** AP target yang SSID-nya tersembunyi otomatis memakai SSID hasil `PROBE SNIFF`/`KARMA ATTACK` bila tersedia.

### 3. Beacon Spam
Menyiarkan banyak SSID palsu secara bergantian (channel hopping) untuk menguji respons perangkat terhadap SSID uji.

**Contoh kasus:** mengisi daftar WiFi korban dengan SSID palsu untuk menguji ketahanan driver WiFi atau sekadar simulasi di lab.

**Cara pakai:** pilih `BEACON SPAM`. Tekan tombol untuk berhenti.

### 4. Probe Sniff
Menangkap probe request yang dipancarkan perangkat, menampilkan MAC, vendor, hitungan, RSSI, dan SSID.

**Contoh kasus:** mengungkap SSID tersembunyi yang tersimpan di HP korban (HP otomatis mem-probe SSID tersimpan); atau memetakan perangkat yang mencari jaringan tertentu.

**Cara pakai:** pilih `PROBE SNIFF` dan biarkan berjalan. SSID kosong (broadcast probe) ditampilkan sebagai `(hidden)`.

### 5. Global Deauth
Mengirim deauth broadcast ke semua channel secara berputar.

**Contoh kasus:** pengujian ketahanan seluruh AP di lab terisolasi terhadap gangguan management frame.

**Cara pakai:** pilih `GLOBAL DEAUTH`. Hanya gunakan di lingkungan yang benar-benar terisolasi.

### 6. Auth Flood
Membanjiri tabel autentikasi AP target dengan frame authentication ber-MAC acak.

**Contoh kasus:** menguji apakah AP target mampu menahan serangan auth flood.

**Cara pakai:** pilih `AUTH FLOOD` → scan → pilih AP target.

### 7. Bluetooth Spam
Mengirim BLE advertisement dari berbagai vendor (Apple, Fast Pair, Swift Pair, Samsung, Amazon, AirPods, iBeacon, Eddystone) secara bergantian.

**Contoh kasus:** menguji respons perangkat terhadap popup pairing BLE (mis. popup Fast Pair/Swift Pair).

**Cara pakai:** pilih `BLUETOOTH SPAM`. WiFi dimatikan sementara karena radio dipakai BLE.

### 8. Password Vault
Menyimpan dan menampilkan kredensial yang tertangkap dari Evil Twin, lengkap dengan status verifikasi (OK/XX/?).

**Contoh kasus:** mereview hasil simulasi phishing (email/username, password, status benar/salah).

**Cara pakai:** pilih `PASSWORD` → tombol atas untuk navigasi, tombol bawah untuk hapus entri/hapus semua/kembali.

### 9. Karma Attack
Merespons probe request dengan probe response, membuat perangkat mengira SSID yang dicarinya tersedia.

**Contoh kasus:** memancing klien terhubung otomatis ke AP yang dikontrol untuk penelitian probe request.

**Cara pakai:** pilih `KARMA ATTACK`, tunggu hasil, lalu pilih SSID untuk melanjutkan ke Evil Twin.

### 10. CSA Attack
Mengirim Channel Switch Announcement (beacon + action frame) untuk memaksa klien pindah kanal.

**Contoh kasus:** menguji ketahanan klien terhadap CSA, termasuk target dengan PMF/WPA3 yang kebal deauth.

**Cara pakai:** pilih `CSA ATTACK` → scan → pilih AP target.

### 11. Web Remote
Mengendalikan NISKALA dari browser HP/laptop via softAP `NISKALA_CTRL`.

**Contoh kasus:** scan dan serang WiFi dari jarak dekat tanpa menyentuh tombol board.

**Cara pakai:** pilih `WEB REMOTE` → hubungkan HP ke WiFi `NISKALA_CTRL` (password `niskala123`) → buka `http://192.168.4.1`. Tersedia scan, attack mode, quick-start, chart paket, config runtime, dan export CSV.

### 12. OTA Update
Update firmware via WiFi tanpa mencabut USB.

**Contoh kasus:** flashing firmware baru saat board sudah terpasang di lokasi yang sulit dijangkau.

**Cara pakai:** pilih `OTA UPDATE` → hubungkan laptop ke `NISKALA_OTA` (password `niskala123`) → jalankan `pio run -e ota -t upload`.

### 13. BLE Scan
Memindai perangkat BLE di sekitar dan melacak MAC-nya.

**Contoh kasus:** memetakan AirTag/smartwatch/perangkat BLE di area lab, atau melacak kehadiran MAC tertentu.

**Cara pakai:** pilih `BLE SCAN`. Tekan tombol untuk berhenti.

### 14. Cloud Sync
Mengirim hasil Probe/Karma/Password/BLE ke hosting sebagai alternatif penyimpanan (pengganti SD card).

**Contoh kasus:** menyimpan hasil pengujian ke dashboard web agar bisa diakses dari mana saja.

**Cara pakai:** isi WiFi SSID/password dan Server URL lewat `WEB REMOTE > Config`, lalu jalankan `CLOUD SYNC`. Upload `server/sync.php` dan `server/config.php` ke hosting.

### 15. Macro
Menjalankan rangkaian serangan secara otomatis.

**Contoh kasus:** `RECON` (Probe Sniff → Karma) untuk intelijen pasif, atau `ASSAULT` (Beacon Spam → Global Deauth) untuk tekanan aktif.

**Cara pakai:** pilih `MACRO` → pilih `RECON` atau `ASSAULT` → tombol bawah untuk menjalankan.

### 16. Client Monitor
Memantau klien/station yang terhubung ke AP target secara real-time (MAC, RSSI, jumlah paket).

**Contoh kasus:** melihat perangkat yang sedang terhubung ke AP target, lalu melakukan deauth presisi ke salah satu MAC.

**Cara pakai:** pilih `CLIENT MONITOR` → scan → pilih AP target → tombol atas untuk pilih klien, tombol bawah untuk deauth klien terpilih / kembali.

</details>

### Serial CLI

Hubungkan via USB dan buka Serial Monitor 115200, lalu ketik perintah:

- help - daftar perintah
- status - info state, paket, heap, uptime
- scan - scan WiFi dan tampilkan hasil
- stop - hentikan semua modul
- beacon / probe / karma / global / blespam / blescan - mulai modul
- deauth AA:BB:CC:DD:EE:FF - deauth target MAC
- ble-raw 020106... - replay advertisement BLE mentah (hex)
- ble-ibeacon [major minor] - spoof iBeacon
- ble-eddy - spoof Eddystone-UID
- config get / config set ssid Nilai - baca/ubah konfigurasi
- reboot - restart perangkat

## Arsitektur

```text
┌──────────────────────────────────────────────┐
│                    main.cpp                  │
│       State machine • UI • button input      │
└───────────────────────┬──────────────────────┘
                        │
      ┌─────────────────┼─────────────────┐
      │                 │                 │
 attack_engine.h     karma.h          bt_spam.h
      │                 │                 │
      └─────────────────┴─────────────────┘
                        │
              FreeRTOS / WiFi callbacks
```

- **Core 0:** task pengujian dan pekerjaan radio.
- **Core 1:** loop utama, UI TFT, tombol, dan layanan pendukung.
- **State:** counter dan mode modul dikelola dari state global project.

## Konfigurasi

| Konstanta | File | Kegunaan |
|---|---|---|
| `cfg_deauth_burst` | `attack_engine.h` + Web Remote | Jumlah frame deauth per burst (runtime, tersimpan di NVS). |
| `cfg_auth_burst` | `attack_engine.h` + Web Remote | Jumlah frame autentikasi per interval (runtime). |
| `cfg_hop_interval_ms` | `attack_engine.h` + Web Remote | Interval perpindahan channel (runtime). |
| `cfg_beacon_hop_ms` | `attack_engine.h` + Web Remote | Interval perubahan beacon (runtime). |
| `PROBE_LOG_SIZE` | `attack_engine.h` | Ukuran buffer probe log. |
| `KARMA_MAX_SSIDS` | `karma.h` | Jumlah slot SSID Karma. |
| `MAX_VERIFY_ATTEMPTS` | `main.cpp` | Batas percobaan verifikasi. |


### Kredensial (jangan commit)

WiFi SSID/password dan API key Cloud Sync dipisah ke file lokal yang di-ignore Git:

| File | Status | Keterangan |
|---|---|---|
| src/secrets.h | Tidak di-commit | Berisi NISKALA_WIFI_SSID, NISKALA_WIFI_PASS, NISKALA_SYNC_API_KEY. |
| src/secrets.example.h | Di-commit | Template, salin menjadi secrets.h. |
| server/config.php | Tidak di-commit | Berisi define('API_KEY', ...). |
| server/config.example.php | Di-commit | Template, salin menjadi config.php. |

Cara pakai:

1. Salin src/secrets.example.h menjadi src/secrets.h, lalu isi nilai asli.
2. Salin server/config.example.php menjadi server/config.php, lalu isi API key.
3. Jangan commit secrets.h dan config.php (sudah di-ignore di .gitignore).
4. (Opsional) Jalankan tools/update-secrets.ps1 untuk mengubah nilai lewat prompt interaktif.

## Batasan

- PMF dapat memblokir management frame tertentu.
- WiFi dan BLE berbagi satu radio pada ESP32.
- Web server, DNS, TFT, dan radio aktif meningkatkan penggunaan heap.
- Hasil pengujian bergantung pada versi ESP32 core, board, dan driver.
- Jangan menyimpan password, API key, atau kredensial nyata di repository.

## Roadmap

- [x] Dashboard lokal via Web Remote (status live + grafik RSSI pada hasil scan).
- [x] Export hasil pengujian ke CSV (Probe, Karma, Password via Web Remote).
- [x] Template awareness tambahan (Generic, IndiHome, Facebook, MikroTik, Google).
- [x] Profil penggunaan heap, suhu, dan baterai (header TFT + Web Remote + serial).
- [x] Runtime config attack (Deauth/Auth burst, Hop/Beacon interval) via Web Remote + NVS.
- [x] Refactor delay blocking -> non-blocking pada flow verifikasi Evil Twin.
- [x] Deteksi PMF/WPA3 presisi (authmode + parsing RSN capabilities beacon).
- [x] BLE Scan + MAC tracking perangkat sekitar.

## Lisensi

**Educational Use Only.** Kode disediakan “as-is”, tanpa jaminan apa pun. Pengguna bertanggung jawab atas seluruh konsekuensi hukum dan teknis dari penggunaannya.

```text
Copyright (c) 2024 NISKALA
Permission granted for educational, research, and authorized
security testing purposes only. Any commercial or malicious
use is strictly prohibited.
```

<div align="center">

**NISKALA v3.1** — *Being invisible doesn't mean being absent.*

⚡ Dibuat untuk yang belajar, bukan untuk yang menyakiti ⚡

</div>
