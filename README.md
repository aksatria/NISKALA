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

---

## Daftar Isi

- [Apa itu NISKALA?](#apa-itu-niskala)
- [Panduan Cepat untuk Pemula](#panduan-cepat-untuk-pemula)
- [Daftar Fitur](#daftar-fitur)
- [Hardware yang Dibutuhkan](#hardware-yang-dibutuhkan)
- [Instalasi & Upload](#instalasi--upload)
- [Cara Pakai Lengkap](#cara-pakai-lengkap)
- [Contoh Kasus Nyata](#contoh-kasus-nyata)
- [Serial CLI Lengkap](#serial-cli-lengkap)
- [Web Remote Lengkap](#web-remote-lengkap)
- [Struktur Project](#struktur-project)
- [Konfigurasi & Secrets](#konfigurasi--secrets)
- [Glosarium (Istilah Awam)](#glosarium-istilah-awam)
- [Troubleshooting / FAQ](#troubleshooting--faq)
- [Batasan Teknis](#batasan-teknis)
- [Arsitektur](#arsitektur)
- [Roadmap](#roadmap)
- [Lisensi](#lisensi)

---

## Apa itu NISKALA?

**NISKALA** adalah firmware untuk board **ESP32 + layar TFT** (seperti TTGO T-Display) yang berisi berbagai alat pengujian keamanan **WiFi dan Bluetooth (BLE)**.

Bayangkan ini seperti "pisau Swiss Army" untuk belajar keamanan jaringan nirkabel:

- Bisa **memindai** WiFi/BLE di sekitar.
- Bisa **memutuskan koneksi** perangkat (deauth) di jaringan milik sendiri.
- Bisa membuat **AP palsu** (Evil Twin) untuk simulasi phishing.
- Bisa **menangkap** data frame (probe, handshake EAPOL).
- Bisa **dikendalikan dari HP/browser** lewat WiFi.

Tujuannya **edukasi dan pengujian keamanan yang sah** — bukan untuk merugikan orang lain.

---

## Panduan Cepat untuk Pemula

Jika ini pertama kalinya kamu memakai NISKALA, ikuti langkah berikut secara berurutan.

### Langkah 1 — Siapkan board

1. Colokkan board TTGO T-Display ke laptop/PC pakai kabel **USB-C/Data** (bukan kabel charge-only).
2. Layar akan menyala menampilkan logo **NISKALA**.
3. Akan muncul layar **Disclaimer**. Tekan **tombol bawah** dua kali untuk menyetujui dan masuk menu utama.

> **Tombol pada board:**
> - **Tombol atas** (dekat logo/atas board) = pilih/geser ke atas.
> - **Tombol bawah** = konfirmasi / mulai / berhenti / pilih ke bawah.

### Langkah 2 — Install software di laptop

1. Install **VS Code** + ekstensi **PlatformIO**.
2. Download/clone project ini.
3. Buka folder project di VS Code.
4. Tunggu PlatformIO selesai memuat, lalu klik **Build** (atau jalankan `pio run`).
5. Klik **Upload** (atau `pio run -t upload`) untuk memasukkan firmware ke board.

### Langkah 3 — Uji coba pertama (paling aman)

1. Di menu utama, pilih **BLE SCAN** lalu tekan tombol bawah.
2. Layar akan menampilkan perangkat BLE di sekitar (mis. smartwatch, HP, earphone).
3. Tekan tombol bawah lagi untuk berhenti.

Ini membuktikan board, layar, tombol, dan radio berfungsi.

### Langkah 4 — Uji scan WiFi

1. Kembali ke menu utama.
2. Pilih **DEAUTH MANUAL** → akan otomatis scan WiFi.
3. Lihat daftar WiFi di sekitar lengkap dengan RSSI dan channel.

Sampai di sini kamu sudah paham dasar navigasi NISKALA.

---

## Daftar Fitur

| Modul | Fungsi singkat |
|---|---|
| Deauth Manual | Memutus satu AP target. |
| Evil Twin | AP palsu + captive portal (simulasi phishing). |
| Beacon Spam | Menyiarkan banyak SSID palsu. |
| Probe Sniff | Menangkap "panggilan" perangkat mencari WiFi. |
| Global Deauth | Memutus semua AP di semua channel. |
| Auth Flood | Membanjiri tabel autentikasi AP. |
| Bluetooth Spam | Mengirim iklan BLE palsu (Apple/Google/Samsung, dll). |
| Password Vault | Menyimpan kredensial hasil Evil Twin. |
| Karma Attack | Menjawab probe request agar perangkat terkoneksi. |
| CSA Attack | Memaksa klien pindah channel. |
| Web Remote | Kendali dari browser HP. |
| BLE Scan | Memetakan perangkat BLE di sekitar. |
| OTA Update | Update firmware via WiFi. |
| Cloud Sync | Kirim hasil ke hosting. |
| Client Monitor | Lihat klien yang terhubung ke AP + deauth presisi. |
| EAPOL Capture | Menangkap handshake WPA2 (M1-M4). |
| Smart Attack | Profil target + rekomendasi serangan otomatis. |
| Network Recon | Ping sweep + port scan + banner grab. |
| AP WiFi | Menyimpan password AP untuk recon. |
| Macro | Rangkaian serangan otomatis (RECON/ASSAULT/HANDSHAKE). |

**Fitur pendukung:**
- Serial CLI (ketik perintah di Serial Monitor 115200).
- Event log ring-buffer (riwayat kejadian, lihat via `log` atau Web UI).
- Telemetri: heap, heap watermark, suhu, baterai, uptime, channel WiFi.
- Deteksi PMF/WPA3 presisi (sebelum deauth).
- Persistent storage (NVS).
- Allowlist (daftar perangkat sendiri agar tidak ikut terputus).

---

## Hardware yang Dibutuhkan

| Komponen | Spesifikasi |
|---|---|
| Board | TTGO T-Display (ESP32) atau ESP32 + TFT ST7789 135x240 |
| Tombol atas | GPIO 0 |
| Tombol bawah | GPIO 35 |
| Baterai (opsional) | Li-Po sesuai board |
| Daya | USB 5V |

> Board TTGO T-Display sudah menyertakan layar + tombol, jadi tidak perlu wiring tambahan.

---

## Instalasi & Upload

### Prasyarat
- VS Code + PlatformIO (atau Arduino IDE 2.x).
- Driver USB CH340/CP210x (biasanya otomatis terpasang).
- Library `TFT_eSPI` (otomatis diunduh PlatformIO lewat `platformio.ini`).

### Langkah
1. Clone repository: `git clone https://github.com/aksatria/NISKALA.git`
2. Buka folder di VS Code.
3. Pilih environment `ttgo-t-display`.
4. Build: `pio run`
5. Upload: `pio run -t upload`
6. Buka monitor serial: `pio device monitor -p COM4 -b 115200`

> Ganti `COM4` sesuai port board kamu (lihat di Device Manager Windows / `/dev/ttyUSB*` di Linux).

---

## Cara Pakai Lengkap

### Navigasi Dasar

| Tombol | Fungsi |
|---|---|
| Tombol atas (GPIO 0) | Pilih item ke atas / pindah pilihan |
| Tombol bawah (GPIO 35) | Konfirmasi / mulai / berhenti / pindah ke bawah |

### Urutan Boot
1. Splash screen.
2. Disclaimer hukum.
3. Konfirmasi tanggung jawab.
4. Menu utama (20 modul).

### Detail 20 Modul

<details>
<summary><strong>Klik untuk membuka detail tiap modul</strong></summary>

#### 1. DEAUTH MANUAL
Memutus koneksi klien dari satu AP target.

- **Contoh kasus:** Menguji apakah router sendiri rentan deauth, atau memaksa satu HP tertentu putus lalu mengamati reconnect.
- **Cara pakai:** Pilih `DEAUTH MANUAL` → scan → pilih AP → tombol bawah start/stop. Lihat `P:` (paket) dan `E:` (error) di layar.
- **Catatan:** Target PMF/WPA3 akan ditolak otomatis karena deauth tidak efektif. Deauth hanya bekerja di 2.4 GHz.

#### 2. EVIL TWIN
Membuat AP palsu dengan SSID sama seperti target + portal login palsu.

- **Contoh kasus:** Simulasi phishing WiFi untuk melatih karyawan agar tidak asal memasukkan password.
- **Cara pakai:** Pilih `EVIL TWIN` → scan → pilih AP → pilih template → korban hubung ke AP palsu → password yang dimasukkan diverifikasi ke AP asli.
- **Catatan:** Template default ada di `src/templates.h`; template kustom bisa diambil dari hosting (URL di konfigurasi).

#### 3. BEACON SPAM
Menyiarkan banyak SSID palsu bergantian.

- **Contoh kasus:** Menguji ketahanan driver WiFi atau mengisi daftar WiFi dengan nama palsu di lab.
- **Cara pakai:** Pilih `BEACON SPAM`. Tombol bawah untuk berhenti.

#### 4. PROBE SNIFF
Menangkap probe request perangkat (MAC, vendor, SSID yang dicari, RSSI, jumlah).

- **Contoh kasus:** Mengungkap SSID tersembunyi yang tersimpan di HP (HP otomatis mem-probe SSID tersimpan).
- **Cara pakai:** Pilih `PROBE SNIFF`. SSID kosong ditampilkan `(hidden)`.

#### 5. GLOBAL DEAUTH
Deauth broadcast ke semua channel berputar.

- **Contoh kasus:** Uji ketahanan jaringan di lab terisolasi.
- **Cara pakai:** Pilih `GLOBAL DEAUTH`. **Hanya di lingkungan terisolasi.**

#### 6. AUTH FLOOD
Membanjiri tabel autentikasi AP dengan MAC acak.

- **Contoh kasus:** Menguji ketahanan AP terhadap auth flood.
- **Cara pakai:** Pilih `AUTH FLOOD` → scan → pilih AP.

#### 7. BLUETOOTH SPAM
Mengirim iklan BLE palsu (Apple Sour Apple/AirPods, Google Fast Pair, Microsoft Swift Pair, Samsung, Amazon, iBeacon, Eddystone).

- **Contoh kasus:** Memunculkan popup pairing BLE di HP target.
- **Cara pakai:** Pilih `BLUETOOTH SPAM`. WiFi mati sementara karena radio dipakai BLE.

#### 8. PASSWORD VAULT
Menyimpan kredensial hasil Evil Twin + status verifikasi (OK/XX/?).

- **Contoh kasus:** Review hasil simulasi phishing.
- **Cara pakai:** Pilih `PASSWORD` → navigasi dengan tombol → hapus entri/hapus semua.

#### 9. KARMA ATTACK
Menjawab probe request agar perangkat mengira SSID yang dicarinya tersedia.

- **Contoh kasus:** Memancing klien terhubung otomatis ke AP yang dikontrol.
- **Cara pakai:** Pilih `KARMA ATTACK` → tunggu → pilih SSID → lanjut ke Evil Twin.

#### 10. CSA ATTACK
Mengirim Channel Switch Announcement (beacon + action frame).

- **Contoh kasus:** Menguji ketahanan klien terhadap CSA, termasuk target PMF/WPA3 yang kebal deauth.
- **Cara pakai:** Pilih `CSA ATTACK` → scan → pilih AP.

#### 11. WEB REMOTE
Mengendalikan NISKALA dari browser HP/laptop.

- **Contoh kasus:** Scan & serang dari jarak dekat tanpa menyentuh tombol board.
- **Cara pakai:** Pilih `WEB REMOTE` → HP hubung ke WiFi `NISKALA_CTRL` (password `niskala123`) → buka `http://192.168.4.1/rc`.

#### 12. OTA UPDATE
Update firmware via WiFi.

- **Contoh kasus:** Flashing firmware baru tanpa cabut USB.
- **Cara pakai:** Pilih `OTA UPDATE` → hubung laptop ke `NISKALA_OTA` (password `niskala123`) → `pio run -e ota -t upload`.

#### 13. BLE SCAN
Memindai perangkat BLE sekitar + MAC tracking.

- **Contoh kasus:** Memetakan AirTag/smartwatch di area lab.
- **Cara pakai:** Pilih `BLE SCAN`. Tombol bawah untuk berhenti.

#### 14. CLOUD SYNC
Mengirim hasil (Probe/Karma/Password/BLE) ke hosting.

- **Contoh kasus:** Menyimpan hasil ke dashboard web agar bisa diakses dari mana saja.
- **Cara pakai:** Isi WiFi & Server URL di `WEB REMOTE > Config`, lalu `CLOUD SYNC`. Upload `server/sync.php` + `server/config.php` ke hosting.

#### 15. MACRO
Menjalankan rangkaian serangan otomatis.

- **RECON:** Probe Sniff → Karma (intelijen pasif).
- **ASSAULT:** Beacon Spam → Global Deauth (tekanan aktif).
- **HANDSHAKE:** EAPOL capture + deauth.
- **Cara pakai:** Pilih `MACRO` → pilih jenis → tombol bawah jalankan.

#### 16. CLIENT MONITOR
Melihat klien yang terhubung ke AP target (MAC, RSSI, paket).

- **Contoh kasus:** Mengetahui perangkat yang sedang online, lalu deauth presisi ke satu MAC.
- **Cara pakai:** Pilih `CLIENT MONITOR` → scan → pilih AP → tombol atas pilih klien → tombol bawah deauth klien/kembali.

#### 17. EAPOL CAPTURE
Menangkap frame EAPOL M1-M4 saat klien reconnect.

- **Contoh kasus:** Audit handshake WPA2 di lab.
- **Cara pakai:** Pilih `EAPOL CAPTURE` → scan → pilih AP. Hasil dilihat di layar, Web Remote `/api/eapol`, atau Cloud Sync.

#### 18. SMART ATTACK
Memprofil target otomatis (enkripsi, PMF, vendor) lalu merekomendasikan serangan.

- **Contoh kasus:** Mempercepat keputusan serangan terbaik.
- **Cara pakai:** Pilih `SMART ATTACK` → scan → pilih target → lihat rekomendasi → tombol bawah jalankan.

#### 19. NETWORK RECON
Setelah terhubung ke jaringan: ping sweep, port scan TCP, banner grab, info gateway/subnet.

- **Contoh kasus:** Memetakan host aktif di lab internal.
- **Cara pakai:** Pilih `NETWORK RECON` → scan → pilih AP. Set password dulu via `config set ssid=<SSID> pass=<PASS>` atau `AP WIFI`.

#### 20. AP WIFI
Menyimpan daftar password AP WiFi di NVS.

- **Contoh kasus:** Menyiapkan target lab sebelum recon/koneksi ulang.
- **Cara pakai:** Pilih `AP WIFI` → tambah/hapus/list password.

</details>

---

## Contoh Kasus Nyata

### Kasus A — Menguji deauth pada router sendiri
1. Pastikan router sendiri punya jaringan **2.4 GHz** (jangan 5 GHz).
2. HP/laptop hubungkan ke WiFi router tersebut.
3. Di NISKALA pilih `DEAUTH MANUAL` → scan → pilih SSID router.
4. Amati HP/laptop: koneksinya akan terputus lalu reconnect.
5. Kalau tidak putus, kemungkinan target di 5 GHz atau PMF aktif (lihat FAQ).

### Kasus B — Simulasi Evil Twin (phishing awareness)
1. Pilih `EVIL TWIN` → scan → pilih SSID target.
2. Pilih template login (mis. `Generic`).
3. Minta "korban" (dengan izin) menghubungkan ke AP palsu.
4. Saat korban memasukkan password, NISKALA memverifikasi ke AP asli.
5. Hasil tersimpan di `PASSWORD` (email/password + status benar/salah).

### Kasus C — Melihat perangkat BLE di sekitar
1. Pilih `BLE SCAN`.
2. Layar menampilkan MAC, nama (jika ada), RSSI, dan jumlah kemunculan.
3. Gunakan untuk memetakan perangkat di area lab.

### Kasus D — Audit handshake WPA2 (EAPOL)
1. Pilih `EAPOL CAPTURE` → scan → pilih AP target.
2. NISKALA otomatis kirim deauth agar klien reconnect.
3. Amati M1-M4 di layar. Jika `FULL` bertambah, handshake tertangkap.

---

## Serial CLI Lengkap

Hubungkan board via USB, buka Serial Monitor **115200**, lalu ketik salah satu perintah:

| Perintah | Fungsi |
|---|---|
| `help` | Daftar perintah |
| `status` | State, mode, paket, error, heap, min heap, baterai, suhu, stack, channel, uptime |
| `log` | Lihat event log (riwayat kejadian) |
| `scan` | Scan WiFi + tampilkan SSID/BSSID/channel/RSSI |
| `stop` | Hentikan semua modul |
| `beacon` | Mulai Beacon Spam |
| `probe` | Mulai Probe Sniff |
| `karma` | Mulai Karma Attack |
| `global` | Mulai Global Deauth |
| `blespam` | Mulai Bluetooth Spam |
| `blescan` | Mulai BLE Scan |
| `deauth AA:BB:CC:DD:EE:FF` | Deauth target AP (MAC) |
| `client AA:BB:CC:DD:EE:FF` | Lihat klien yang terhubung ke AP (MAC) |
| `ble-raw 020106...` | Replay iklan BLE mentah (hex) |
| `ble-ibeacon [major minor]` | Spoof iBeacon |
| `ble-eddy` | Spoof Eddystone-UID |
| `config get` | Lihat konfigurasi |
| `config set ssid <Nilai>` | Ubah konfigurasi (ssid/pass/url/tpl) |
| `apwifi add <SSID> <PASS>` | Simpan password AP |
| `apwifi list` | Lihat daftar AP tersimpan |
| `apwifi del <index>` | Hapus AP tersimpan |
| `sync` / `cloudsync` | Mulai Cloud Sync |
| `export` | Cetak data hasil sebagai JSON |
| `reboot` | Restart perangkat |

> Contoh: `deauth C0:E1:BE:9F:F8:BC` lalu `stop`.

---

## Web Remote Lengkap

1. Pilih menu `WEB REMOTE`.
2. Hubungkan HP/laptop ke WiFi **`NISKALA_CTRL`** (password `niskala123`).
3. Buka browser ke **`http://192.168.4.1/rc`**.

### Yang bisa dilakukan di Web Remote
- **Scan WiFi** dan pilih target langsung dari browser.
- **Mode attack**: Deauth / Evil Twin / Auth Flood / CSA.
- **Quick start**: Beacon, Global Deauth, Probe Sniff, BLE Spam, Karma.
- **Telemetri**: heap, heap_min, stack, baterai, suhu, uptime, channel.
- **EAPOL panel**: lihat M1-M4.
- **Event Log panel**: lihat riwayat kejadian.
- **Config attack**: ubah burst deauth/auth, interval hop/beacon, reason code, WiFi & URL sync.
- **Export CSV**: Probe, Karma, Password.

### Token Web (opsional)
Jika `NISKALA_WEB_PASS` diisi di `src/secrets.h`, Web Remote akan minta token (dikirim sebagai header `X-NISKALA-TOKEN`). Jika kosong, autentikasi dilewati.

---

## Struktur Project

```text
NISKALA/
├── src/
│   ├── main.cpp            # State machine, UI, tombol, loop utama
│   ├── attack_engine.cpp/h # Frame builder deauth/beacon/CSA + sniffer + attack task
│   ├── karma.cpp/h         # Logika Karma
│   ├── bt_spam.cpp/h       # BLE spam
│   ├── tx_count.cpp/h      # Counter TX + mutex helper
│   ├── ble_scan.cpp/h      # BLE scan + tracking
│   ├── eapol.cpp/h         # EAPOL handshake capture
│   ├── client_monitor.cpp/h# Client monitor
│   ├── cloud.cpp/h         # Cloud sync & fetch template
│   ├── recon.cpp/h         # Network recon
│   ├── macro.cpp/h         # Macro attack sequence
│   ├── ota.cpp/h           # OTA update
│   ├── smart_attack.cpp/h  # Profiling + deteksi PMF
│   ├── telemetry.cpp/h     # Telemetri + event log
│   ├── radio_manager.cpp/h # Manajemen mode radio
│   ├── wifi_state.h        # Restore MAC / reset WiFi state
│   ├── pins.h              # Definisi pin tombol & baterai
│   ├── cli.cpp/h           # Serial CLI
│   ├── allowlist.cpp/h     # Allowlist perangkat sendiri
│   ├── ui_screens.cpp/h    # Layar & UI TFT
│   ├── ui_helpers.h        # Helper gambar
│   ├── ui_icons.h          # Ikon
│   ├── web_remote.h        # Endpoint API web
│   ├── web_ui.h            # HTML/JS Web Remote
│   ├── web_utils.h         # JSON escape / CSV helper
│   ├── state_machine.h     # Enum state
│   ├── templates.h         # Template captive portal
│   ├── disclaimer.h        # Layar disclaimer
│   ├── wifi_bypass.c       # Kompatibilitas raw frame
│   └── secrets.example.h   # Template kredensial
├── server/
│   ├── sync.php            # Endpoint Cloud Sync
│   ├── view.php            # Dashboard viewer
│   ├── template.html       # Template portal hosting
│   └── data/.htaccess      # Proteksi akses folder data
├── platformio.ini
├── .gitignore
├── LICENSE
└── README.md
```

---

## Konfigurasi & Secrets

### Konstanta runtime (disimpan di NVS, bisa diubah via Web Remote)
| Konstanta | Keterangan |
|---|---|
| `cfg_deauth_burst` | Jumlah frame deauth per burst (1-100) |
| `cfg_auth_burst` | Jumlah frame auth per interval (1-200) |
| `cfg_hop_interval_ms` | Interval ganti channel (20-2000 ms) |
| `cfg_beacon_hop_ms` | Interval ganti beacon (20-2000 ms) |
| `cfg_deauth_reason` | Reason code deauth (0 = rotasi) |

### Kredensial (jangan commit)
| File | Status |
|---|---|
| `src/secrets.h` | Tidak di-commit (lokal). Berisi `NISKALA_WIFI_SSID/PASS`, `NISKALA_SYNC_API_KEY`, `NISKALA_WEB_PASS`. |
| `src/secrets.example.h` | Template (di-commit). Salin jadi `secrets.h`. |
| `server/config.php` | Tidak di-commit. Berisi `API_KEY`. |
| `server/config.example.php` | Template (di-commit). |

**Cara membuat secrets.h:**
1. Salin `src/secrets.example.h` → `src/secrets.h`.
2. Isi nilai sesuai kebutuhan (boleh dikosongkan jika tidak pakai).
3. Pastikan `secrets.h` dan `config.php` tidak ter-commit (sudah di-ignore).

---

## Glosarium (Istilah Awam)

| Istilah | Arti sederhana |
|---|---|
| **AP (Access Point)** | Perangkat pemancar WiFi (router). |
| **SSID** | Nama WiFi yang terlihat (mis. "Office PT.PPKT"). |
| **BSSID** | Alamat MAC unik dari AP. |
| **MAC address** | "Nomor identitas" perangkat (contoh: `AA:BB:CC:DD:EE:FF`). |
| **Deauth** | Frame yang memaksa perangkat putus dari WiFi. |
| **Probe request** | "Panggilan" perangkat saat mencari WiFi yang pernah disimpan. |
| **Beacon** | "Iklan" yang dipancarkan AP agar namanya terlihat. |
| **Evil Twin** | AP palsu dengan nama sama seperti target. |
| **Captive portal** | Halaman login palsu yang muncul saat terhubung. |
| **RSSI** | Kekuatan sinyal (semakin dekat 0 semakin kuat). |
| **Channel** | "Jalur" frekuensi WiFi (1-14 untuk 2.4 GHz). |
| **PMF (802.11w)** | Fitur keamanan yang memblokir deauth. |
| **WPA2/WPA3** | Jenis enkripsi WiFi. |
| **EAPOL** | Frame proses "jabat tangan" keamanan WPA2. |
| **Handshake** | Proses tukar kunci saat perangkat terhubung. |
| **BLE** | Bluetooth Low Energy (untuk perangkat pintar). |
| **NVS** | Penyimpanan permanen kecil di ESP32. |

---

## Troubleshooting / FAQ

### 1. Board tidak terdeteksi di laptop?
- Pastikan pakai **kabel data** (bukan kabel charge-only).
- Install driver USB CH340/CP210x.
- Cek di Device Manager port `COMx` (Windows) atau `/dev/ttyUSB*` (Linux).

### 2. Gagal build / upload?
- Pastikan PlatformIO + platform ESP32 sudah terpasang.
- Coba `pio run -t clean` lalu `pio run` lagi.
- Pastikan pilih environment `ttgo-t-display`.

### 3. Layar kosong / menyala tapi tidak ada teks?
- Pastikan board-nya memang TTGO T-Display (atau sesuaikan pin TFT di `platformio.ini` / `User_Setup.h` TFT_eSPI).

### 4. Deauth tidak memutus target?
- **Kemungkinan besar target di 5 GHz.** ESP32 hanya bisa 2.4 GHz.
- Kemungkinan lain: target memakai **PMF/WPA3** (deauth diblokir).
- Pastikan target tersambung ke jaringan **2.4 GHz**.

### 5. HP tidak terhubung ke AP `NISKALA_CTRL`?
- Pastikan menu `WEB REMOTE` aktif.
- Password AP: `niskala123`.
- Buka `http://192.168.4.1/rc`.

### 6. Web Remote minta token?
- Token diambil dari `NISKALA_WEB_PASS` di `secrets.h`. Kalau kosong, tidak perlu token.

### 7. Board terasa panas?
- Normal untuk operasi berat (deauth/beacon terus-menerus). Beri jeda, atau pastikan ventilasi cukup.

### 8. Bagaimana melihat apakah board sehat?
- Serial CLI ketik `status` dan `log`.
- Pantau `Heap` (harus stabil) dan `Min` (heap watermark, tidak terus turun).

---

## Batasan Teknis

- **ESP32 hanya 2.4 GHz.** Tidak bisa melihat/menyerang WiFi 5 GHz.
- **PMF/WPA3** dapat memblokir management frame (deauth/beacon).
- **WiFi & BLE berbagi satu radio** — tidak bisa aktif bersamaan.
- Web server, DNS, TFT, dan radio aktif menambah beban heap.
- Hasil pengujian bergantung versi ESP32 core, board, dan driver.
- Jangan simpan kredensial nyata di repository.

---

## Arsitektur

- **Core 0:** task serangan + pekerjaan radio (attackTask).
- **Core 1:** loop utama, UI TFT, tombol, WebServer, DNS, serial CLI.
- **Shared state:** counter dan status dilindungi mutex (`g_txMux`, `g_stateMux`, `g_bleScanMux`, `g_eapolMux`, dll.) untuk mencegah data race antar-task/callback.

---

## Roadmap

- [x] Dashboard lokal Web Remote (status live + grafik).
- [x] Export CSV (Probe, Karma, Password).
- [x] Template captive portal (Generic, IndiHome, Facebook, MikroTik, Google).
- [x] Telemetri heap/suhu/baterai (TFT + Web + serial).
- [x] Runtime config attack via Web Remote + NVS.
- [x] Deteksi PMF/WPA3 presisi.
- [x] BLE Scan + MAC tracking.
- [x] EAPOL capture M1-M4 + export.
- [x] Smart Attack profiling.
- [x] Central Radio State Manager.
- [x] Web Remote auth token.
- [x] Event log ring-buffer (CLI + Web UI).
- [x] Refactor engine ke `.cpp` + mutex shared state.

---

## Lisensi

**Educational Use Only.** Kode disediakan "as-is", tanpa jaminan apa pun. Pengguna bertanggung jawab atas seluruh konsekuensi hukum dan teknis.

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
