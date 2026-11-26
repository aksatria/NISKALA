# Changelog

Semua perubahan penting pada NISKALA.

## v3.2 (unreleased)
- Refactor besar `main.cpp` menjadi modul: `attack_control`, `web_handlers`, `config_store`, `target_store`, `script_runner`, `packet_monitor`, `oui`.
- UI Web Remote dipindah ke LittleFS (`data/index.html`) + fallback firmware.
- Packet Monitor: deteksi deauth/disassociate (menu TFT, CLI `pm`, API `/api/pm`).
- OUI vendor lookup terpusat.
- Save/load daftar target (CLI + API + UI Web Remote).
- Preset serangan (CLI + API + UI Web Remote).
- Scripting serangan via CLI (`script`).
- i18n Web Remote EN/ID lengkap (label, toast, hasil scan/status/PMF/telemetri/BLE).
- CLI bilingual (`lang id|en`).
- Dokumentasi bahasa Inggris (`README.en.md`).
- Hardening: rate-limit auth web, CSRF/origin check + `collectHeaders`, sync.php rate-limit & payload-limit.
- Dukungan board generic headless (`esp32-generic`), board HAL untuk tombol.
- CI GitHub Actions + unit test host-side (native).
- Perbaikan bug: strip tanda kutip pada `script`, fix MAC/channel deauth, UI freeze akibat MutationObserver.

## v3.1
- 20 modul awal, Web Remote dashboard, telemetri, cloud sync, EAPOL capture, Smart Attack, BLE scan/spam.

## v3.0
- Rilis awal berbasis TTGO T-Display.