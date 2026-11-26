# NISKALA

<div align="center">

### Network Intrusion & Security Kit for Analysis, Learning & Assessment

![Version](https://img.shields.io/badge/version-3.1-brightgreen)
![Platform](https://img.shields.io/badge/platform-ESP32-blue)
![Framework](https://img.shields.io/badge/framework-Arduino-00979D)
![License](https://img.shields.io/badge/license-Educational-red)

**ESP32-based WiFi & Bluetooth security learning tool for labs, education, and authorized testing.**

</div>

> [!CAUTION]
> Use only on your own devices/networks or with written permission.
> The user is fully responsible for how this tool is used.

---

## What is NISKALA?

NISKALA is firmware for an **ESP32** board (e.g. TTGO T-Display with a small color screen and 2 buttons). It is a "multi-tool" for learning WiFi & Bluetooth security:

- Scan nearby WiFi/Bluetooth devices.
- Run controlled deauthentication / flooding attacks on your own networks.
- Simulate phishing (Evil Twin + Captive Portal).
- Capture EAPOL handshakes.
- Control via buttons, Serial CLI, and a Web Remote dashboard.

> It is intended for learning and authorized security testing, not for harming others.

---

## Features (20+ modules)

| Category | Modules |
|---|---|
| Monitoring / intel | PROBE SNIFF, BLE SCAN, CLIENT MONITOR, NETWORK RECON, SMART ATTACK |
| WiFi attacks | DEAUTH MANUAL, GLOBAL DEAUTH, AUTH FLOOD, BEACON SPAM, CSA ATTACK, KARMA ATTACK |
| Social engineering | EVIL TWIN (+ Captive Portal), PASSWORD VAULT |
| Bluetooth | BLUETOOTH SPAM, BLE SCAN |
| Connectivity | WEB REMOTE, OTA UPDATE, CLOUD SYNC, AP WIFI |
| Automation | MACRO, SCRIPT (serial scripting) |
| Observability | PACKET MONITOR (deauth/disassoc detection), EAPOL CAPTURE |

---

## Requirements

- TTGO T-Display (ESP32 + ST7789 135x240) — or ESP32 generic (headless build).
- USB data cable.
- VS Code + PlatformIO.

---

## Build & Flash

```bash
# clone
git clone https://github.com/aksatria/NISKALA.git
cd NISKALA

# build & upload firmware
pio run -e ttgo-t-display -t upload

# upload the web UI filesystem (LittleFS)
pio run -e ttgo-t-display -t uploadfs
```

Other environments:
- `ota` — over-the-air update.
- `esp32-generic` — headless build (Serial CLI / Web Remote only).
- `native` — host-side unit tests.

---

## Serial CLI

Open the serial monitor at **115200 baud**, then type `help`.

Useful commands:
```text
scan            scan WiFi
status          board status
deauth <MAC>    deauth a target AP
global          global deauth
beacon          beacon spam
probe           probe sniff
karma           karma attack
blespam         BLE spam
blescan         BLE scan
web             start Web Remote
pm              packet monitor log
preset <p>      aggressive | normal | stealth
target add <name> <MAC> [ch]
target list
target deauth <idx|name>
script "beacon:5000,global:3000"
config get | config set <key> <value>
export          print all collected data as JSON
reboot          restart
```

---

## Web Remote

1. Start `WEB REMOTE` (menu or CLI `web`).
2. Connect to WiFi `NISKALA_CTRL` (password `niskala123`).
3. Open `http://192.168.4.1/rc`.

The dashboard provides scanning, attack control, telemetry, EAPOL status, event log, config, and CSV export.

> Optional token: set `NISKALA_WEB_PASS` in `src/secrets.h` to enable `X-NISKALA-TOKEN` auth.

---

## Project structure (simplified)

```text
src/
  main.cpp               state machine, menu, setup/loop
  attack_control.cpp     attack start/stop orchestration
  attack_engine.cpp      raw frames, sniffer, attack task
  packet_monitor.cpp     deauth/disassoc detector
  web_handlers.cpp       captive portal routing
  config_store.cpp       NVS config & password vault
  target_store.cpp       saved target list
  script_runner.cpp      serial attack scripting
  oui.h                  OUI vendor lookup
  board_config.h         board selection & pins
data/index.html          Web Remote UI (LittleFS)
server/sync.php          cloud sync endpoint
```

---

## Documentation

- [README.md](README.md) — full Indonesian documentation.
- [docs.html](https://niskalasys.my.id/docs.html) — online docs.

---

## License

**Educational Use Only.** Provided "as-is" without warranty. See [LICENSE](LICENSE).

**NISKALA v3.1** — *Being invisible doesn't mean being absent.*