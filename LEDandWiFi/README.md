# M5Sensor — Atom Lite Wi-Fi LED Controller

Control the built-in RGB LED on an [M5Stack Atom Lite](https://shop.m5stack.com/products/atom-lite-esp32-development-kit) over Wi-Fi.
The device starts in **AP mode**, serves a simple web page, and exposes REST endpoints for reading and setting the LED color.

---

## Features

* **Wi-Fi AP mode** (default SSID: `M5Stack_Ap`, password: `66666666`)
* **Web UI** with color picker (WIP: `/web/index.html`)
* **REST API**:

  * `GET /get` → returns current LED color in `#RRGGBB` format
  * `GET /set?value=#RRGGBB` → sets LED color
* **QR codes** for quick Wi-Fi connect and device info (`tools/qr_generator.ipynb`)
* **Smoke tests** with cURL examples (`tests/api_smoke.md`)

---

## Repository Layout

```
/firmware
  /src
    main.ino          ← Arduino sketch (AP + API + HTML)
/web
  index.html          ← client page (currently a stub)
/tools
  qr_generator.ipynb  ← generates Wi-Fi + info QR codes
/tests
  api_smoke.md        ← manual API tests
README.md
CHANGELOG.md
```

---

## Getting Started

### Hardware

* M5Stack Atom Lite (ESP32-PICO)
* USB-C cable for flashing

### Software

* [Arduino IDE](https://docs.m5stack.com/en/arduino/arduino_ide) or PlatformIO
* Install board package: [See the Arduino Board Management tutorial.](https://docs.m5stack.com/en/arduino/arduino_board)
* Install library: [See the Arduino Library Management tutorial.](https://docs.m5stack.com/en/arduino/arduino_library)

### Flash

1. Open `firmware/src/main.ino` in Arduino IDE.
2. Select **M5Stack-ATOM** board.
3. Upload to device.

### Connect

1. On your phone or PC, connect to Wi-Fi:
   **SSID:** `M5Stack_Ap`
   **Password:** `66666666`
2. Open [http://192.168.4.1](http://192.168.4.1) in your browser.
3. Use the color picker to change the LED.

---

## API Examples

Read current color:

```bash
curl http://192.168.4.1/get
# → "#00FF00"
```

Set LED to red:

```bash
curl "http://192.168.4.1/set?value=%23FF0000"
```

Invalid input:

```bash
curl "http://192.168.4.1/set?value=red"
# → HTTP 400 Bad format
```

---

## Tests

* Manual smoke tests: see [`/tests/api_smoke.md`](./tests/api_smoke.md)
* Automated scripts (optional): Python requests-based checks under `/tests`.

---

## Tools

Generate QR codes for Wi-Fi and device info:

```bash
jupyter notebook tools/qr_generator.ipynb
```

Outputs:

* `wifi_connect_qr.png` — auto Wi-Fi connect
* `atom_info_qr.png` — SSID + PASS + URL

---

## Roadmap

* [ ] Finish HTML client page (color + Wi-Fi settings form)
* [ ] STA mode with AP fallback
* [ ] Token authentication for `/set`
* [ ] Serve assets from SPIFFS/LittleFS
* [ ] GitHub Actions: lint + build + API tests

---

## License

MIT
