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
| Probe Sniff | Observasi probe request pada lingkungan terkontrol. |
| Global Deauth | Pengujian ketahanan jaringan di lab terisolasi. |
| Auth Flood | Pengujian ketahanan tabel autentikasi AP lab. |
| Bluetooth Spam | Pengujian respons perangkat terhadap BLE advertisement. |
| Password Vault | Penyimpanan hasil pengujian di NVS. |
| Karma Attack | Simulasi respons terhadap probe request. |

### Fitur Pendukung

- Deteksi PMF/WPA3 sebelum pengujian management frame.
- Persistent storage menggunakan NVS.
- UI TFT dengan radar, progress bar, dan animasi.
- Disclaimer berlapis sebelum menu utama.
- Konfigurasi modul terpusat di folder `src/`.

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
│   └── wifi_bypass.c     # Komponen kompatibilitas build
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
<summary><strong>Detail modul pengujian</strong></summary>

### Deauth Manual

Digunakan untuk menguji respons management frame pada AP lab. Modul menampilkan status paket dan menolak target yang terdeteksi menggunakan PMF/WPA3.

### Evil Twin / Captive Portal

Digunakan untuk simulasi awareness dan pengujian captive portal pada perangkat milik sendiri. Template HTML berada di `src/templates.h`.

### Beacon Spam dan Probe Sniff

Digunakan untuk menguji bagaimana perangkat menangani SSID uji dan probe request dalam lingkungan terkontrol.

### Global Deauth dan Auth Flood

Hanya untuk lab terisolasi. Kedua modul dapat mengganggu konektivitas perangkat lain jika digunakan di luar scope.

### Bluetooth Spam

Menguji respons perangkat terhadap BLE advertisement. WiFi dan BLE berbagi radio pada ESP32 sehingga keduanya tidak berjalan penuh secara bersamaan.

### Password Vault

Menampilkan hasil pengujian yang disimpan pada NVS. Hapus data hanya setelah memastikan data tersebut tidak lagi diperlukan.

### Karma Attack

Digunakan untuk penelitian probe request dan simulasi respons SSID pada lab wireless.

</details>

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
| `DEAUTH_BURST` | `attack_engine.h` | Jumlah frame per burst. |
| `AUTH_BURST` | `attack_engine.h` | Jumlah frame autentikasi per interval. |
| `HOP_INTERVAL_MS` | `attack_engine.h` | Interval perpindahan channel. |
| `BEACON_HOP_MS` | `attack_engine.h` | Interval perubahan beacon. |
| `PROBE_LOG_SIZE` | `attack_engine.h` | Ukuran buffer probe log. |
| `KARMA_MAX_SSIDS` | `karma.h` | Jumlah slot SSID Karma. |
| `MAX_VERIFY_ATTEMPTS` | `main.cpp` | Batas percobaan verifikasi. |

## Batasan

- PMF dapat memblokir management frame tertentu.
- WiFi dan BLE berbagi satu radio pada ESP32.
- Web server, DNS, TFT, dan radio aktif meningkatkan penggunaan heap.
- Hasil pengujian bergantung pada versi ESP32 core, board, dan driver.
- Jangan menyimpan password, API key, atau kredensial nyata di repository.

## Roadmap

- [ ] Dashboard lokal untuk monitoring perangkat sendiri.
- [ ] Export hasil pengujian ke CSV.
- [ ] Template awareness tambahan.
- [ ] Profil penggunaan heap dan performa radio.
- [ ] Mode UI minimal untuk lab.

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
