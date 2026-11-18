# NISKALA

> **N**etwork **I**ntrusion & **S**ecurity **K**it â€” **A**udit **L**ab & **A**ssessment
> 
> *"NISKALA - To be invisible is not to be absent"*
> 
> **"Gunakan dengan bijak. Etika lebih penting dari skill. Adab lebih penting dari ilmu."**

---

## ðŸ“– Apa itu NISKALA?

**NISKALA** adalah firmware *pentest* WiFi & BLE berbasis **ESP32** (dioptimalkan untuk **TTGO T-Display**) yang dirancang untuk **audit keamanan jaringan nirkabel milik sendiri atau yang Anda punya izin tertulis**. 

Nama *Niskala* diambil dari bahasa Sanskerta/Indonesia yang berarti *"tak berwujud"* â€” menggambarkan sifat serangan nirkabel yang tidak kasat mata namun berdampak nyata. *To be invisible is not to be absent* (Tak terlihat bukan berarti tidak ada).

NISKALA menggabungkan **9 modul serangan** (WiFi layer 2 attack, Evil Twin captive portal, Karma attack, BLE spam) dalam satu perangkat genggam dengan antarmuka grafis TFT dan navigasi dua tombol.

> âš ï¸ **PERINGATAN HUKUM â€” BACA SEBELUM MENGGUNAKAN**
> NISKALA hanya boleh digunakan untuk:
> - âœ… Audit keamanan jaringan **milik sendiri**
> - âœ… Pengujian penetrasi dengan **izin tertulis** dari pemilik jaringan
> - âœ… Penelitian & edukasi di laboratorium terisolasi
>
> **Dilarang keras** digunakan untuk menyerang, menyadap, atau mengganggu jaringan milik orang lain tanpa izin. Pelanggaran dapat dikenakan **UU ITE Pasal 30â€“32** (akses ilegal, intersepsi, dan gangguan sistem elektronik) dengan ancaman **6â€“8 tahun penjara** dan/atau denda hingga **Rp 2 miliar**.

---

## âœ¨ Fitur Lengkap

| # | Modul | Deskripsi | Layer |
|---|-------|-----------|-------|
| 1 | **Deauth Manual** | Kirim frame deauthentication terarah ke AP target, lacak korban secara otomatis | 802.11 L2 |
| 2 | **Evil Twin** | Clone SSID AP target + captive portal untuk menangkap password (dengan verifikasi otomatis ke AP asli) | L2 + L7 |
| 3 | **Beacon Spam** | Spam 16 SSID palsu (termasuk nama-nama lucu: *FBI Surveillance*, *Pretty Fly for WiFi*) | 802.11 L2 |
| 4 | **Probe Sniffer** | Tangkap probe request dari HP sekitar (MAC + RSSI + SSID yang dicari) | 802.11 L2 |
| 5 | **Global Deauth** | Broadcast deauth ke semua channel (1â€“13) dengan hopping otomatis | 802.11 L2 |
| 6 | **Auth Flood** | Flood authentication request dengan MAC acak ke AP target (DoS) | 802.11 L2 |
| 7 | **Bluetooth Spam** | Spam BLE advertisement Apple (Sour Apple, Nearby Setup, Nearby WiFi) | BLE |
| 8 | **Password Viewer** | Lihat, verifikasi, dan hapus password hasil tangkapan (tersimpan di NVS) | Storage |
| 9 | **Karma Attack** | Scan probe request 20 detik, ranking SSID terpopuler, auto-cari AP asli lalu jalankan Evil Twin | 802.11 L2 |

### Fitur Pendukung
- âœ… **Disclaimer berlapis** (2 layar konfirmasi sebelum masuk menu)
- âœ… **Deteksi PMF/WPA3** â€” cegah deauth sia-sia ke AP modern
- âœ… **Auto-verifikasi password** â€” coba konek ke AP asli max 5Ã—, password valid â†’ simpan sebagai `OK`
- âœ… **Auto-release korban** â€” setelah password benar, korban dilepas kembali ke AP asli
- âœ… **Multi-template captive portal** (generic WiFi login, dll â€” extensible)
- âœ… **Penyimpanan NVS** â€” password & setting persistent antar reboot
- âœ… **UI TFT beranimasi** â€” radar sweep, pulse effect, spinner, corner brackets

---

## ðŸ› ï¸ Kebutuhan Hardware

Berikut adalah spesifikasi perangkat keras utama yang digunakan:

| Komponen | Spesifikasi | Keterangan |
|----------|-------------|------------|
| **Board** | **TTGO T-Display ESP32** | Sudah terintegrasi dalam satu board. |
| **Display** | TFT ST7789 135Ã—240 | Terpasang langsung di board TTGO. |
| **Tombol** | 2Ã— push button | `BTN_TOP = GPIO 0`, `BTN_BOTTOM = GPIO 35` (Tombol bawaan TTGO T-Display). |
| **Antena** | Antena WiFi/BLE internal | Cukup untuk jangkauan lokal. Bisa dimodifikasi ke antena eksternal jika perlu. |
| **Catu daya** | USB-C / Li-Po 3.7V | TTGO T-Display memiliki konektor baterai JST 1.25mm. Konsumsi ~120 mA saat TX aktif. |

### Wiring Diagram (Bawaan TTGO T-Display)
Karena Anda menggunakan TTGO T-Display, Anda **tidak perlu melakukan wiring manual** untuk layar dan tombol. Semuanya sudah tersolder di dalam board.
*   **Tombol Atas (BTN_TOP)**: GPIO 0
*   **Tombol Bawah (BTN_BOTTOM)**: GPIO 35
*   **Layar TFT**: Terhubung ke pin standar TTGO.

---

## ðŸ’» Kebutuhan Software

- **VS Code** dengan ekstensi **PlatformIO**.
- **Board Package**: `esp32` by Espressif â‰¥ 2.0.14.
- **Libraries** (Tambahkan di `platformio.ini`):
  - `bodmer/TFT_eSPI@^2.5.0`
  - *Library BLE, WiFi, WebServer, DNSServer, Preferences* (sudah bawaan dari core ESP32).

### Konfigurasi `platformio.ini`

Pastikan file `platformio.ini` Anda kurang lebih berisi konfigurasi seperti ini untuk TTGO T-Display:

```ini
[env:ttgo-t-display]
platform = espressif32
board = ttgo-t-display
framework = arduino
monitor_speed = 115200
board_build.f_cpu = 240000000L
board_build.flash_mode = dio
build_flags = 
    -DCORE_DEBUG_LEVEL=0
    -DARDUINO_USB_CDC_ON_BOOT=1
lib_deps = 
    bodmer/TFT_eSPI@^2.5.0


## ?? Struktur Project

```text
TTGO-T-Display-ESP32/
??? .pio/
??? .vscode/
??? lib/
??? src/
?   ??? attack_engine.h
?   ??? bt_spam.h
?   ??? disclaimer.h
?   ??? karma.h
?   ??? main.cpp
?   ??? templates.h
?   ??? wifi_bypass.c
??? .gitignore
??? LICENSE
??? platformio.ini
??? README.md
```

> Catatan: folder `.pio/` dan `.vscode/` diabaikan oleh Git sesuai `.gitignore`, sehingga tidak ikut di-upload ke repository. Struktur di atas menunjukkan susunan project secara lokal.

---

