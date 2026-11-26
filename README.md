# NISKALA

<div align="center">

### Network Intrusion & Security Kit for Analysis, Learning & Assessment

![Version](https://img.shields.io/badge/version-3.1-brightgreen)
![Platform](https://img.shields.io/badge/platform-ESP32-blue)
![Framework](https://img.shields.io/badge/framework-Arduino-00979D)
![License](https://img.shields.io/badge/license-Educational-red)

**Alat belajar keamanan WiFi & Bluetooth berbasis ESP32 untuk lab, edukasi, dan pengujian berizin.**

</div>

> English documentation: [README.en.md](README.en.md)

> [!CAUTION]
> **Gunakan hanya pada perangkat dan jaringan milik sendiri atau dengan izin tertulis.**
> Jangan mengganggu jaringan publik, mencuri data, atau menguji target tanpa izin. Pengguna bertanggung jawab penuh atas penggunaannya.

---

# Daftar Isi

1. [Apa itu NISKALA? (Bahasa Sederhana)](#1-apa-itu-niskala-bahasa-sederhana)
2. [Konsep Dasar dalam 1 Menit](#2-konsep-dasar-dalam-1-menit)
3. [Checklist: Apa yang Kamu Butuhkan](#3-checklist-apa-yang-kamu-butuhkan)
4. [Instalasi dari Nol (Langkah demi Langkah)](#4-instalasi-dari-nol-langkah-demi-langkah)
5. [Penggunaan Pertama Kali di Board](#5-penggunaan-pertama-kali-di-board)
6. [Peta Menu (20 Modul)](#6-peta-menu-20-modul)
7. [Detail Lengkap Setiap Modul](#7-detail-lengkap-setiap-modul)
8. [Contoh Kasus Nyata](#8-contoh-kasus-nyata)
9. [Serial CLI (Lengkap)](#9-serial-cli-lengkap)
10. [Web Remote (Lengkap)](#10-web-remote-lengkap)
11. [Cheat Sheet](#11-cheat-sheet)
12. [Struktur Project](#12-struktur-project)
13. [Konfigurasi & Secrets](#13-konfigurasi--secrets)
14. [Troubleshooting / FAQ](#14-troubleshooting--faq)
15. [Batasan Teknis](#15-batasan-teknis)
16. [Arsitektur](#16-arsitektur)
17. [Roadmap](#17-roadmap)
18. [Lisensi](#18-lisensi)

---

# 1. Apa itu NISKALA? (Bahasa Sederhana)

**NISKALA** adalah program (firmware) yang kamu masukkan ke sebuah papan elektronik kecil bernama **ESP32** (contohnya TTGO T-Display) yang punya **layar kecil** dan **2 tombol**.

Ibaratnya ini **"pisau serbaguna"** untuk belajar keamanan jaringan WiFi dan Bluetooth:

- **Melihat** WiFi/Bluetooth di sekitar (seperti radar).
- **Memutus koneksi** perangkat WiFi milikmu sendiri (untuk menguji keamanan).
- **Membuat WiFi palsu** untuk simulasi penipuan (agar orang belajar waspada).
- **Merekam** data jaringan untuk dianalisis.

> Tujuannya **belajar & menguji keamanan milik sendiri**, bukan merugikan orang lain.

---

# 2. Konsep Dasar dalam 1 Menit

Sebelum lanjut, pahami istilah-istilah ini dengan analogi sederhana:

| Istilah | Arti seperti... |
|---|---|
| **WiFi** | Jaringan internet tanpa kabel. |
| **SSID** | "Nama" WiFi yang kamu lihat di daftar (mis. "WiFi Rumah"). |
| **Password WiFi** | Kunci untuk masuk ke WiFi. |
| **AP (Access Point)** | Alat pemancar WiFi (router). |
| **MAC Address** | "Nomor identitas" unik tiap perangkat (seperti KTP). |
| **Deauth** | Sinyal untuk "mengusir" perangkat dari WiFi. |
| **Beacon** | "Iklan" yang dipancarkan router agar namanya terlihat. |
| **Probe Request** | "Panggilan" perangkat mencari WiFi yang pernah disimpan. |
| **Evil Twin** | WiFi palsu yang meniru nama WiFi asli. |
| **Captive Portal** | Halaman login palsu yang muncul saat terhubung. |
| **BLE (Bluetooth)** | Bluetooth hemat daya untuk perangkat pintar (earphone, smartwatch). |
| **Channel** | "Jalur" frekuensi WiFi (seperti jalur jalan). |
| **2.4 GHz vs 5 GHz** | Dua jenis pita frekuensi WiFi. **ESP32 hanya bisa 2.4 GHz.** |
| **PMF** | Fitur keamanan WiFi yang memblokir deauth. |
| **WPA2 / WPA3** | Jenis kunci/pengaman WiFi. |

> **Kunci penting:** ESP32 NISKALA **hanya bekerja di WiFi 2.4 GHz**, tidak bisa menyentuh WiFi 5 GHz.

---

# 3. Checklist: Apa yang Kamu Butuhkan

### Perangkat keras
- [ ] 1 buah board **TTGO T-Display** (ESP32) — atau ESP32 + layar ST7789 135x240.
- [ ] 1 kabel **USB data** (bisa transfer data, bukan cuma isi daya).
- [ ] (Opsional) Baterai Li-Po 3.7V.

### Perangkat lunak (di laptop/PC)
- [ ] **VS Code** (editor teks gratis dari Microsoft).
- [ ] Ekstensi **PlatformIO** (untuk menulis & meng-upload program ke ESP32).
- [ ] **Driver USB** (CH340 atau CP210x) — biasanya otomatis terpasang.
- [ ] File project **NISKALA** (di-download dari GitHub).

### Pengetahuan minimum
- [ ] Bisa menyalakan komputer, install aplikasi, dan buka folder.
- [ ] Itu saja. Sisanya dijelaskan di bawah.

---

# 4. Instalasi dari Nol (Langkah demi Langkah)

Ini panduan **paling detail** untuk orang yang belum pernah pakai ESP32 sama sekali.

### 4.1 Install VS Code
1. Buka browser, kunjungi `https://code.visualstudio.com`.
2. Klik **Download**, pilih sesuai OS (Windows/Mac/Linux).
3. Buka file hasil download, ikuti petunjuk instalasi (Next → Install → Finish).
4. Buka VS Code setelah selesai.

### 4.2 Install PlatformIO
1. Di VS Code, klik ikon **Extensions** (kotak-kotak di sisi kiri).
2. Ketik `PlatformIO` di kotak pencarian.
3. Klik **Install** pada hasil "PlatformIO IDE".
4. Tunggu sampai selesai (beberapa menit). VS Code akan restart/memuat ulang.

### 4.3 Download Project NISKALA
1. Buka `https://github.com/aksatria/NISKALA`.
2. Klik tombol hijau **Code** → **Download ZIP**.
3. Ekstrak (unzip) file tersebut ke folder yang mudah diingat (mis. `C:\NISKALA`).

> Alternatif (kalau sudah terbiasa): `git clone https://github.com/aksatria/NISKALA.git`

### 4.4 Buka Project di VS Code
1. Buka VS Code.
2. Klik **File → Open Folder**.
3. Pilih folder NISKALA tadi (folder yang berisi `platformio.ini`).
4. Tunggu PlatformIO memuat project (muncul tulisan "Loading..." di bawah).

### 4.5 Sambungkan Board
1. Colokkan kabel USB dari board ke laptop.
2. **Layar board akan menyala** menampilkan logo NISKALA.
3. Kalau layar tidak menyala, cek kabel (harus kabel data) atau cek FAQ.

### 4.6 Build (Kompilasi)
1. Di VS Code, klik ikon **PlatformIO** (logo alien/kepala) di sisi kiri.
2. Pada bagian **PROJECT TASKS** → klik `ttgo-t-display` → **General → Build**.
3. Tunggu sampai muncul tulisan **SUCCESS** di bawah.

> Atau lewat terminal: `pio run`

### 4.7 Upload (Masukkan program ke board)
1. Masih di PlatformIO, klik **General → Upload** (atau `pio run -t upload`).
2. Tunggu sampai muncul **SUCCESS** dan board restart sendiri.
3. Board akan menampilkan logo NISKALA lalu layar **Disclaimer**.

### 4.8 Buka Serial Monitor (untuk melihat log & perintah)
1. Klik **General → Monitor** (atau `pio device monitor -p COM4 -b 115200`).
2. Pilih baudrate **115200** jika diminta.
3. Kamu akan melihat teks dari board di terminal.

> **Apa itu COM4?** Itu "alamat" kabel USB di laptop. Di Windows, cek di **Device Manager → Ports (COM & LPT)**. Di Linux biasanya `/dev/ttyUSB0`.

---

# 5. Penggunaan Pertama Kali di Board

Board punya **2 tombol**:

```
  [Tombol ATAS]  = GPIO 0  -> pilih ke ATAS / ganti pilihan
  [Tombol BAWAH] = GPIO 35 -> KONFIRMASI / MULAI / BERHENTI
```

### Urutan saat pertama dinyalakan
1. **Layar splash** → logo NISKALA muncul sebentar.
2. **Layar Disclaimer** → peringatan hukum.
3. Tekan **tombol bawah** → muncul layar konfirmasi.
4. Tekan **tombol bawah** lagi → masuk **Menu Utama**.

### Cara navigasi menu
- Tekan **tombol atas/bawah** untuk naik-turun memilih menu.
- Tekan **tombol bawah** (lama) pada item yang dipilih untuk **menjalankan** modul itu.
- Saat modul berjalan, tekan **tombol bawah** untuk **berhenti**.

> **Tes pertama yang aman:** pilih `BLE SCAN` lalu tekan tombol bawah. Layar akan menampilkan perangkat Bluetooth di sekitar. Tekan tombol bawah lagi untuk berhenti.

---

# 6. Peta Menu (20 Modul)

Menu utama NISKALA berisi 20 modul. Berikut pengelompokannya agar mudah dipahami:

### 🔍 Pemantauan & Intelijen (aman, hanya melihat)
| Modul | Fungsi singkat |
|---|---|
| PROBE SNIFF | Melihat "panggilan" perangkat mencari WiFi. |
| BLE SCAN | Melihat perangkat Bluetooth di sekitar. |
| CLIENT MONITOR | Melihat siapa saja yang terhubung ke WiFi tertentu. |
| NETWORK RECON | Memetakan perangkat di jaringan (setelah terhubung). |
| SMART ATTACK | Menganalisis target & memberi saran serangan. |

### ⚔️ Serangan WiFi (uji keamanan jaringan sendiri)
| Modul | Fungsi singkat |
|---|---|
| DEAUTH MANUAL | Memutus satu WiFi target. |
| GLOBAL DEAUTH | Memutus semua WiFi di semua channel. |
| AUTH FLOOD | Membanjiri router dengan permintaan login. |
| BEACON SPAM | Menyiarkan banyak nama WiFi palsu. |
| CSA ATTACK | Memaksa perangkat pindah channel. |
| KARMA ATTACK | Menjawab panggilan perangkat agar terkoneksi. |

### 🎭 Social Engineering (simulasi penipuan)
| Modul | Fungsi singkat |
|---|---|
| EVIL TWIN | Membuat WiFi palsu + halaman login palsu. |
| PASSWORD VAULT | Menyimpan hasil login yang tertangkap. |

### 📡 Bluetooth
| Modul | Fungsi singkat |
|---|---|
| BLUETOOTH SPAM | Mengirim iklan Bluetooth palsu. |

### 🔗 Koneksi & Sinkronisasi
| Modul | Fungsi singkat |
|---|---|
| WEB REMOTE | Kendali dari browser HP. |
| OTA UPDATE | Update program lewat WiFi. |
| CLOUD SYNC | Kirim hasil ke internet (hosting). |
| AP WIFI | Menyimpan daftar password WiFi. |

### 🤖 Otomasi
| Modul | Fungsi singkat |
|---|---|
| MACRO | Menjalankan rangkaian serangan otomatis. |

---

# 7. Detail Lengkap Setiap Modul

<details>
<summary><strong>Klik untuk membuka penjelasan semua modul</strong></summary>

### 7.1 DEAUTH MANUAL
**Apa fungsinya :** Memaksa perangkat putus dari WiFi target.

**Contoh kasus:** Kamu ingin tahu apakah router-mu sendiri rentan diserang, atau memaksa satu HP putus lalu melihatnya reconnect.

**Cara pakai:**
1. Pilih `DEAUTH MANUAL` → otomatis scan WiFi.
2. Pilih SSID target dengan tombol.
3. Tekan tombol bawah untuk mulai.
4. Lihat `P:` (paket terkirim) dan `E:` (error) di layar.
5. Tekan tombol bawah untuk berhenti.

**Yang akan kamu lihat:** HP/laptop target terputus dari WiFi, lalu (biasanya) otomatis tersambung lagi.

**Catatan:** Target 5 GHz atau PMF/WPA3 tidak bisa dideauth oleh ESP32.

### 7.2 EVIL TWIN
**Apa fungsinya :** Membuat WiFi palsu dengan nama sama seperti target, lalu menampilkan halaman login palsu.

**Contoh kasus:** Melatih karyawan/keluarga agar tidak asal memasukkan password di halaman login mencurigakan.

**Cara pakai:**
1. Pilih `EVIL TWIN` → scan → pilih AP target.
2. Pilih **mode** (lihat tabel di bawah): `1 BOARD` atau `2 BOARD`.
3. Pilih template login (Generic, IndiHome, Facebook, dll.).
4. Korban (dengan izin) menghubungkan ke WiFi palsu.
5. Saat korban memasukkan password, NISKALA memverifikasi ke WiFi asli.
6. Hasil tersimpan di `PASSWORD`.

**Yang akan kamu lihat:** Korban melihat halaman login; hasil (email/password, benar/salah) muncul di `PASSWORD`.

#### Mode Evil Twin

| Mode | Deauth sendiri | Clone MAC AP asli | Kapan dipakai |
|---|---|---|---|
| **1 BOARD** | Ya | Ya | Pakai 1 perangkat saja |
| **2 BOARD** | Tidak | Tidak | Dua perangkat dibagi tugas |

#### 7.2.1 Mode 2 BOARD (Jammer + Evil Twin)

**Kenapa butuh 2 board?** ESP32 hanya punya **1 radio**. Dalam mode 1 board, radio harus bergantian tiga tugas: (1) jadi AP palsu, (2) deauth AP asli, (3) verifikasi password. Ketiganya saling berebut — begitu board jadi AP (layani korban), deauth berhenti; begitu verifikasi password, AP palsu sempat drop. Akibatnya deauth putus-putus dan AP palsu tidak stabil.

**Solusi:** pisahkan tugas ke 2 board.

| Board | Menu | Tugas |
|---|---|---|
| **A (Jammer)** | `DEAUTH MANUAL` | Deauth AP asli + kliennya terus-menerus tanpa berhenti |
| **B (Evil Twin)** | `EVIL TWIN` → mode `2 BOARD` | AP palsu + portal + tangkap & verifikasi password |

**Kenapa mode 2 BOARD tidak clone MAC?** Kalau Board B memakai MAC sama dengan AP asli, deauth dari Board A (yang menyasar MAC AP asli) akan ikut memutus korban yang sudah terhubung ke AP palsu Board B. Kedua board jadi "perang" sendiri. Dengan MAC sendiri, deauth Board A tidak menyentuh korban di AP palsu.

**Kenapa mode 2 BOARD tidak deauth sendiri?** Deauth sudah jadi tugas Board A. Kalau Board B ikut deauth di channel yang sama, frame dobel/tabrakan, dan jadwal radio Board B jadi tidak stabil (AP palsu drop).

**Hasil akhir:** deauth kuat & kontinu (Board A) + AP palsu stabil & verifikasi akurat (Board B).

**Cara pakai 2 board:**
1. Board A → `DEAUTH MANUAL` → pilih AP asli → biarkan menyala.
2. Board B → `EVIL TWIN` → pilih SSID yang sama → mode `2 BOARD` → pilih template → mulai.

### 7.3 BEACON SPAM
**Apa fungsinya :** Menyiarkan banyak nama WiFi palsu sekaligus.

**Contoh kasus:** Menguji ketahanan driver WiFi atau mengisi daftar WiFi dengan nama palsu di lab.

**Cara pakai:** Pilih `BEACON SPAM` → tombol bawah untuk berhenti.

### 7.4 PROBE SNIFF
**Apa fungsinya :** Melihat perangkat yang sedang "mencari" WiFi (termasuk WiFi tersembunyi yang pernah disimpan).

**Contoh kasus:** Mengungkap SSID tersembunyi yang tersimpan di HP — HP otomatis mem-probe SSID tersimpan.

**Cara pakai:** Pilih `PROBE SNIFF`, biarkan berjalan. SSID kosong tampil sebagai `(hidden)`.

### 7.5 GLOBAL DEAUTH
**Apa fungsinya :** Memutus semua WiFi di semua channel sekaligus.

**Contoh kasus:** Uji ketahanan jaringan di lab **terisolasi**.

**Cara pakai:** Pilih `GLOBAL DEAUTH` → tombol bawah berhenti. **Hanya di lingkungan terisolasi.**

### 7.6 AUTH FLOOD
**Apa fungsinya :** Membanjiri router dengan banyak permintaan login palsu.

**Contoh kasus:** Menguji apakah router tahan terhadap banjir permintaan login.

**Cara pakai:** Pilih `AUTH FLOOD` → scan → pilih AP target.

### 7.7 BLUETOOTH SPAM
**Apa fungsinya :** Mengirim iklan Bluetooth palsu (Apple, Google, Samsung, dll.) untuk memunculkan popup pairing.

**Contoh kasus:** Memunculkan popup "Fast Pair"/"Swift Pair" di HP target.

**Cara pakai:** Pilih `BLUETOOTH SPAM`. WiFi mati sementara (radio dipakai Bluetooth).

### 7.8 PASSWORD VAULT
**Apa fungsinya :** Tempat menyimpan hasil login yang tertangkap dari Evil Twin.

**Contoh kasus:** Review hasil simulasi phishing (email, password, status benar/salah).

**Cara pakai:** Pilih `PASSWORD` → navigasi → hapus entri/hapus semua.

### 7.9 KARMA ATTACK
**Apa fungsinya :** Menjawab "panggilan" perangkat agar mengira WiFi yang dicarinya tersedia.

**Contoh kasus:** Memancing klien terhubung otomatis ke AP yang dikontrol.

**Cara pakai:** Pilih `KARMA ATTACK` → tunggu → pilih SSID → lanjut ke Evil Twin.

### 7.10 CSA ATTACK
**Apa fungsinya :** Memaksa perangkat pindah channel WiFi.

**Contoh kasus:** Menguji ketahanan klien terhadap perpindahan channel, termasuk target PMF/WPA3 yang kebal deauth.

**Cara pakai:** Pilih `CSA ATTACK` → scan → pilih AP target.

### 7.11 WEB REMOTE
**Apa fungsinya :** Mengendalikan NISKALA dari browser HP/laptop lewat WiFi.

**Cara pakai:**
1. Pilih `WEB REMOTE`.
2. Hubungkan HP ke WiFi `NISKALA_CTRL` (password `niskala123`).
3. Buka `http://192.168.4.1/rc`.

**Yang bisa dilakukan:** scan WiFi, mulai serangan, lihat telemetri, lihat event log, ubah konfigurasi, export CSV.

### 7.12 OTA UPDATE
**Apa fungsinya :** Update program NISKALA lewat WiFi, tanpa cabut kabel USB.

**Cara pakai:**
1. Pilih `OTA UPDATE`.
2. Hubungkan laptop ke WiFi `NISKALA_OTA` (password `niskala123`).
3. Jalankan `pio run -e ota -t upload`.

### 7.13 BLE SCAN
**Apa fungsinya :** Melihat & melacak perangkat Bluetooth di sekitar.

**Contoh kasus:** Memetakan AirTag/smartwatch/earphone di area lab.

**Cara pakai:** Pilih `BLE SCAN` → tombol bawah berhenti.

### 7.14 CLOUD SYNC
**Apa fungsinya :** Mengirim hasil pengujian ke internet (hosting) sebagai pengganti SD card.

**Cara pakai:**
1. Isi WiFi & Server URL di `WEB REMOTE > Config`.
2. Upload `server/sync.php` + `server/config.php` ke hosting.
3. Jalankan `CLOUD SYNC`.

### 7.15 MACRO
**Apa fungsinya :** Menjalankan beberapa serangan secara otomatis berurutan.

- **RECON:** Probe Sniff → Karma (intelijen pasif).
- **ASSAULT:** Beacon Spam → Global Deauth (tekanan aktif).
- **HANDSHAKE:** EAPOL capture + deauth.

**Cara pakai:** Pilih `MACRO` → pilih jenis → tombol bawah jalankan.

### 7.16 CLIENT MONITOR
**Apa fungsinya :** Melihat siapa saja yang sedang terhubung ke WiFi target.

**Contoh kasus:** Mengetahui perangkat online di WiFi target, lalu deauth presisi ke satu MAC.

**Cara pakai:** Pilih `CLIENT MONITOR` → scan → pilih AP → tombol atas pilih klien → tombol bawah deauth klien/kembali.

### 7.17 EAPOL CAPTURE
**Apa fungsinya :** Merekam "jabat tangan" keamanan WPA2 saat perangkat reconnect.

**Contoh kasus:** Audit handshake WPA2 di lab.

**Cara pakai:** Pilih `EAPOL CAPTURE` → scan → pilih AP. Hasil di layar, Web Remote `/api/eapol`, atau Cloud Sync.

### 7.18 SMART ATTACK
**Apa fungsinya :** Menganalisis target otomatis (enkripsi, PMF, vendor) lalu menyarankan serangan terbaik.

**Cara pakai:** Pilih `SMART ATTACK` → scan → pilih target → lihat rekomendasi → tombol bawah jalankan.

### 7.19 NETWORK RECON
**Apa fungsinya :** Setelah terhubung ke jaringan, memetakan perangkat & layanan di dalamnya (ping sweep, port scan, banner).

**Cara pakai:** Pilih `NETWORK RECON` → scan → pilih AP. Set password dulu via `config set ssid=<SSID> pass=<PASS>` atau `AP WIFI`.

### 7.20 AP WIFI
**Apa fungsinya :** Menyimpan daftar password WiFi untuk dipakai modul lain.

**Cara pakai:** Pilih `AP WIFI` → tambah/hapus/list password.

</details>

---

# 8. Contoh Kasus Nyata

### Kasus A — Menguji deauth pada router sendiri
1. Pastikan router memancarkan WiFi **2.4 GHz**.
2. Hubungkan HP/laptop ke WiFi router tersebut.
3. Pilih `DEAUTH MANUAL` → scan → pilih SSID router.
4. Amati HP/laptop: koneksi putus lalu reconnect.
5. Kalau tidak putus → target kemungkinan 5 GHz atau PMF aktif (lihat FAQ).

### Kasus B — Simulasi Evil Twin (phishing awareness)
1. Pilih `EVIL TWIN` → scan → pilih SSID target.
2. Pilih template login (mis. `Generic`).
3. Minta "korban" (dengan izin) hubung ke WiFi palsu.
4. Saat korban masukkan password, NISKALA verifikasi ke WiFi asli.
5. Hasil tersimpan di `PASSWORD`.

### Kasus C — Memetakan perangkat Bluetooth di area
1. Pilih `BLE SCAN`.
2. Layar menampilkan MAC, nama, RSSI, jumlah kemunculan.

### Kasus D — Audit handshake WPA2 (EAPOL)
1. Pilih `EAPOL CAPTURE` → scan → pilih AP.
2. NISKALA otomatis kirim deauth agar klien reconnect.
3. Amati M1-M4 di layar. Jika `FULL` bertambah, handshake tertangkap.

---

# 9. Serial CLI (Lengkap)

> Dokumentasi web lengkap tersedia di [docs.html](https://niskalasys.my.id/docs.html), termasuk tabel menu, troubleshooting, API, dan glosarium.


**Apa itu Serial CLI?** Cara mengendalikan board lewat teks (terminal), tanpa layar/tombol.

**Cara membuka:** colok USB, buka Serial Monitor 115200, ketik perintah lalu Enter.

### Semua perintah

| Perintah | Fungsi |
|---|---|
| `help` | Daftar perintah |
| `status` | Info lengkap (state, mode, paket, error, heap, min heap, baterai, suhu, stack, channel, uptime) |
| `log` | Lihat event log (riwayat kejadian) |
| `scan` | Scan WiFi + tampilkan SSID/BSSID/channel/RSSI |
| `stop` | Hentikan semua modul |
| `web` | Mulai Web Remote tanpa tombol fisik |
| `web stop` | Tutup Web Remote dan kembali ke menu |
| `beacon` | Mulai Beacon Spam |
| `probe` | Mulai Probe Sniff |
| `karma` | Mulai Karma Attack |
| `global` | Mulai Global Deauth |
| `blespam` | Mulai Bluetooth Spam |
| `blescan` | Mulai BLE Scan |
| `deauth AA:BB:CC:DD:EE:FF` | Deauth target AP (MAC) |
| `client AA:BB:CC:DD:EE:FF` | Lihat klien terhubung ke AP |
| `ble-raw 020106...` | Replay iklan BLE mentah (hex) |
| `ble-ibeacon [major minor]` | Spoof iBeacon |
| `ble-eddy` | Spoof Eddystone-UID |
| `config get` | Lihat konfigurasi |
| `config set ssid <Nilai>` | Ubah konfigurasi |
| `apwifi add <SSID> <PASS>` | Simpan password AP |
| `apwifi list` | Lihat daftar AP tersimpan |
| `apwifi del <index>` | Hapus AP tersimpan |
| `sync` / `cloudsync` | Mulai Cloud Sync |
| `export` | Cetak data hasil (JSON) |
| `wardriving [stop]` | Mulai/hentikan Wardriving Web Remote (port 8080) |
| `wigle <file.csv>` | Upload CSV tersimpan ke WiGLE.net |
| `files` | Daftar file CSV tersimpan di LittleFS |
| `reboot` | Restart board |

### Contoh output `status`
```
State=0 Mode=0 PKT=0 ERR=0 Heap=138000 Min=136000 Bat=100 Temp=90 Stack=3060 Ch=6 Up=120s
```
Artinya: idle, 0 paket, heap 138KB, baterai 100%, suhu 90C, stack 3060 byte, channel 6, nyala 120 detik.

---

# 10. Web Remote (Lengkap)

> [!NOTE]
> Sejak v3.2, UI Web Remote disimpan di filesystem LittleFS (`data/index.html`).
> Setelah upload firmware, jalankan juga `pio run -e ttgo-t-display -t uploadfs`
> agar antarmuka web tersedia. Tanpa itu, halaman akan menampilkan fallback.

1. Pilih menu `WEB REMOTE`.
2. Hubungkan HP/laptop ke WiFi **`NISKALA_CTRL`** (password `niskala123`).
3. Buka browser ke **`http://192.168.4.1/rc`**.

### Fitur Web Remote
- **SCAN WIFI** — tampilkan daftar WiFi, klik untuk serang.
- **MODE ATTACK** — Deauth / Evil Twin / Auth Flood / CSA.
- **QUICK START** — Beacon, Global Deauth, Probe Sniff, BLE Spam, Karma.
- **TELEMETRI** — heap, heap_min, stack, baterai, suhu, uptime, channel.
- **EAPOL CAPTURE** — lihat M1-M4.
- **EVENT LOG** — lihat riwayat kejadian.
- **CONFIG ATTACK** — ubah burst/interval/reason + WiFi & URL sync.
- **EXPORT** — download CSV Probe/Karma/Password.

### Token Web (opsional)
Jika `NISKALA_WEB_PASS` diisi di `secrets.h`, Web Remote minta token. Kalau kosong, autentikasi dilewati.

### Wardriving via Web Remote (port 8080)

Wardriving adalah fitur pemetaan jaringan WiFi + GPS. ESP32 menjadi "sensor radio"
yang mengirim data AP (BSSID, SSID, channel, RSSI, auth, vendor) ke browser lewat
WebSocket. Browser (HP) mengambil lokasi GPS via Geolocation API lalu menggabungkannya
menjadi CSV standar WiGLE.

1. Wardriving otomatis aktif saat boot — board jadi AP `NISKALA_CTRL` / `niskala123`.
2. Hubungkan HP ke WiFi `NISKALA_CTRL`.
3. Buka `http://192.168.4.1:8080/`.
4. Izinkan akses lokasi (GPS) di browser.
5. Tunggu jaringan ter-scan, lalu tekan **Download WiGLE CSV**.

Fitur UI:
- Status WebSocket, GPS lock, jumlah jaringan.
- Statistik Open / Encrypted / rata-rata RSSI.
- Tabel 5 jaringan terakhir (real-time) + dedup otomatis (AP hilang > 60 detik dihapus).
- Auto-reconnect WebSocket (exponential backoff).
- Simpan CSV ke LittleFS board (`/api/save`), daftar & hapus file di UI.

Upload CSV ke WiGLE.net lewat Serial CLI:

```
wigle niskala_<timestamp>.csv
```

Board akan menyambung ke WiFi `NISKALA_WIFI_SSID` lalu upload. Kredensial WiGLE
diisi di `secrets.h` (`NISKALA_WIGLE_API_NAME` dan `NISKALA_WIGLE_API_TOKEN`).

---

# 11. Cheat Sheet

### Tombol board
| Aksi | Tombol |
|---|---|
| Pilih ke atas | Tombol atas |
| Pilih ke bawah / konfirmasi | Tombol bawah |
| Mulai modul | Tombol bawah (pada item terpilih) |
| Berhenti | Tombol bawah (saat modul jalan) |

### Perintah CLI penting
| Tujuan | Perintah |
|---|---|
| Lihat status | `status` |
| Lihat log | `log` |
| Scan WiFi | `scan` |
| Berhenti semua | `stop` |
| Deauth AP | `deauth <MAC>` |
| Lihat klien AP | `client <MAC>` |
| Restart | `reboot` |

---

# 12. Struktur Project

```text
NISKALA/
├── src/
│   ├── main.cpp              # Menu, tombol, state machine, loop utama
│   ├── attack_engine.cpp/h   # Frame deauth/beacon/CSA + sniffer + attack task
│   ├── karma.cpp/h           # Logika Karma
│   ├── bt_spam.cpp/h         # BLE spam
│   ├── tx_count.cpp/h        # Counter TX + mutex
│   ├── ble_scan.cpp/h        # BLE scan + tracking
│   ├── eapol.cpp/h           # EAPOL capture
│   ├── client_monitor.cpp/h  # Client monitor
│   ├── cloud.cpp/h           # Cloud sync & template
│   ├── recon.cpp/h           # Network recon
│   ├── macro.cpp/h           # Macro sequence
│   ├── ota.cpp/h             # OTA update
│   ├── smart_attack.cpp/h    # Profiling + deteksi PMF
│   ├── telemetry.cpp/h       # Telemetri + event log
│   ├── radio_manager.cpp/h   # Manajemen mode radio
│   ├── wifi_state.h          # Restore MAC / reset WiFi
│   ├── pins.h                # Pin tombol & baterai
│   ├── cli.cpp/h             # Serial CLI
│   ├── allowlist.cpp/h       # Allowlist perangkat sendiri
│   ├── ui_screens.cpp/h      # Layar & UI TFT
│   ├── ui_helpers.h          # Helper gambar
│   ├── ui_icons.h            # Ikon
│   ├── web_remote.h          # Endpoint API web
│   ├── web_ui.h              # HTML/JS Web Remote
│   ├── web_utils.h           # JSON/CSV helper
│   ├── state_machine.h       # Enum state
│   ├── templates.h           # Template captive portal
│   ├── disclaimer.h          # Layar disclaimer
│   ├── wifi_bypass.c         # Kompatibilitas raw frame
│   ├── wardriving.cpp/h      # Wardriving backend (WebSocket + scan pasif)
│   ├── wigle_upload.cpp/h    # Upload CSV ke WiGLE.net
│   └── secrets.example.h     # Template kredensial
├── server/
│   ├── sync.php              # Endpoint Cloud Sync
│   ├── view.php              # Dashboard viewer
│   ├── template.html         # Template portal hosting
│   └── data/.htaccess        # Proteksi folder data
├── data/
│   ├── index.html            # UI Web Remote (LittleFS)
│   └── wardriving.html       # UI Wardriving (Vanilla JS + GPS)
├── platformio.ini
├── .gitignore
├── LICENSE
└── README.md
```

---

# 13. Konfigurasi & Secrets

### Konstanta runtime (diubah via Web Remote, tersimpan di NVS)
| Konstanta | Keterangan |
|---|---|
| `cfg_deauth_burst` | Jumlah frame deauth per burst (1-100) |
| `cfg_auth_burst` | Jumlah frame auth per interval (1-200) |
| `cfg_hop_interval_ms` | Interval ganti channel (20-2000 ms) |
| `cfg_beacon_hop_ms` | Interval ganti beacon (20-2000 ms) |
| `cfg_deauth_reason` | Reason code deauth (0 = rotasi) |

### Kredensial rahasia (JANGAN di-commit ke GitHub)
| File | Status |
|---|---|
| `src/secrets.h` | Tidak di-commit (lokal). Isi: `NISKALA_WIFI_SSID/PASS`, `NISKALA_SYNC_API_KEY`, `NISKALA_WEB_PASS`, `NISKALA_WIGLE_API_NAME/TOKEN`. |
| `src/secrets.example.h` | Template (di-commit). Salin jadi `secrets.h`. |
| `server/config.php` | Tidak di-commit. Isi: `API_KEY`. |

**Cara membuat secrets.h:**
1. Salin `src/secrets.example.h` → `src/secrets.h`.
2. Isi nilai sesuai kebutuhan (boleh dikosongkan).
3. Pastikan `secrets.h` dan `config.php` tidak ter-commit (sudah di-ignore otomatis).

---

# 14. Troubleshooting / FAQ

### 1. Board tidak terdeteksi laptop?
- Pakai **kabel data** (bukan kabel charge-only).
- Install driver USB CH340/CP210x.
- Cek Device Manager → Ports (COM & LPT).

### 2. Gagal build / upload?
- Pastikan PlatformIO + platform ESP32 terpasang.
- Coba `pio run -t clean` lalu `pio run`.
- Pilih environment `ttgo-t-display`.

### 3. Layar kosong / tidak menyala?
- Pastikan board TTGO T-Display (atau sesuaikan pin TFT).
- Cek kabel USB.

### 4. Deauth tidak memutus target?
- **Paling sering: target di 5 GHz.** ESP32 cuma 2.4 GHz.
- Kemungkinan lain: target pakai PMF/WPA3.
- Pastikan target tersambung ke WiFi 2.4 GHz.

### 5. HP tidak terhubung ke `NISKALA_CTRL`?
- Pastikan menu `WEB REMOTE` aktif.
- Password `niskala123`.
- Buka `http://192.168.4.1/rc`.

### 6. Web Remote minta token?
- Token dari `NISKALA_WEB_PASS` di `secrets.h`. Kalau kosong, tidak perlu.

### 7. Board terasa panas?
- Normal untuk operasi berat. Beri jeda / pastikan ventilasi.

### 8. Bagaimana cek kesehatan board?
- Serial CLI: `status` dan `log`.
- Pantau `Heap` (harus stabil) dan `Min` (heap watermark, tidak terus turun).

### 9. Apa bedanya `P:` dan `E:` di layar deauth?
- `P:` = paket terkirim. `E:` = error. Error kecil (beberapa persen) itu normal.

### 10. Kenapa BLE Spam mematikan WiFi?
- ESP32 hanya punya satu radio untuk WiFi & Bluetooth. Saat BLE aktif, WiFi mati sementara.

---

# 15. Batasan Teknis

- **ESP32 hanya 2.4 GHz** — tidak bisa melihat/menyerang WiFi 5 GHz.
- **PMF/WPA3** bisa memblokir management frame (deauth/beacon).
- **WiFi & BLE berbagi satu radio** — tidak bisa aktif bersamaan.
- **Headless**: environment `esp32-generic` berjalan tanpa layar; kode TFT otomatis menjadi stub no-op via `tft_compat.h`.
- Web server, DNS, TFT, radio aktif menambah beban heap.
- Hasil pengujian bergantung versi ESP32 core, board, dan driver.
- Jangan simpan kredensial nyata di repository.

---

# 16. Arsitektur

- **Core 0:** task serangan + pekerjaan radio (attackTask).
- **Core 1:** loop utama, UI TFT, tombol, WebServer, DNS, serial CLI.
- **Shared state:** counter/status dilindungi mutex (`g_txMux`, `g_stateMux`, `g_bleScanMux`, `g_eapolMux`) untuk mencegah data race.

---

# 17. Roadmap

- [x] Dashboard lokal Web Remote (status live + grafik).
- [x] Export CSV (Probe, Karma, Password).
- [x] Template captive portal (Generic, IndiHome, Facebook, MikroTik, Google).
- [x] Telemetri heap/suhu/baterai.
- [x] Runtime config attack via Web Remote + NVS.
- [x] Deteksi PMF/WPA3 presisi.
- [x] BLE Scan + MAC tracking.
- [x] EAPOL capture M1-M4 + export.
- [x] Smart Attack profiling.
- [x] Central Radio State Manager.
- [x] Web Remote auth token.
- [x] Event log ring-buffer (CLI + Web UI).
- [x] Refactor engine ke `.cpp` + mutex shared state.
- [x] Wardriving via Web Remote (WebSocket + GPS + CSV WiGLE).
- [x] Upload CSV ke WiGLE.net (CLI `wigle`).
- [x] Headless 100% (env `esp32-generic` tanpa library TFT_eSPI via `tft_compat.h`).

---

# 18. Lisensi

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
