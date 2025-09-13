# Changelog

All notable changes to this project will be documented in this file.  
This format follows the [Keep a Changelog](https://keepachangelog.com/en/1.0.0/) convention.

## [Unreleased]
* STA mode (connect to a Wi-Fi router) with AP fallback.
* Token-based authentication for the `/set` endpoint.
* Basic **HTML client page** stub (`/web/index.html`) — planned to display current LED color, color picker, and Wi-Fi settings form.

## 2025-09-13
### Added
* **Wi-Fi access-point (AP) firmware:** implemented AP mode for M5Stack Atom Lite.  
  New sketch (`firmware/src/main.ino`) starts a soft-AP, serves an HTML page with `<input type="color">`, and provides REST endpoints:
  * `GET /get` — returns current LED color (`#RRGGBB`)
  * `GET /set?value=#RRGGBB` — sets LED color:contentReference[oaicite:0]{index=0}.
* **QR code generator:** added notebook `tools/qr_generator.ipynb` to generate two QR codes:
  * Wi-Fi connect code (SSID + password)
  * Info code (SSID, password, and device URL).  
  Output files: `wifi_connect_qr.png`, `atom_info_qr.png`:contentReference[oaicite:1]{index=1}.
* **Smoke tests:** added `tests/api_smoke.md` with cURL commands and expected responses for `/get`, `/set`, and error scenarios.
* **Documentation:** added `README.md` and this `CHANGELOG.md`.

### Changed
* **Project structure:** reorganized into `LEDandWiFi/` with subfolders:
  * `firmware`, `web`, `tools`, `tests`  
  Old `LEDandWiFi.cpp` removed; new main file in `firmware/src/main.ino`.  
  Added VS Code configuration files (`.vscode/…`):contentReference[oaicite:2]{index=2}.
* **Resources:** replaced old `qrCode.ipynb` with new generator notebook under `tools/qr_generator.ipynb`:contentReference[oaicite:3]{index=3}.