# m5sensor

This repo contains the ESP32 firmware, dashboard server, infrastructure playbooks, and architecture documentation for the M5 pressure sensor system.

## Repo layout

- `PressureSensor/` – ESP32 firmware (MQTT publish, HTTP server, pressure read)
- `dashboard-server/` – FastAPI dashboard + MQTT listener
- `infra/` – Ansible playbooks for Mosquitto, InfluxDB, Caddy, and deploy
- `docs/` – architecture docs + diagrams (`ARCHITECTURE.md`, `ARCHITECTURE.typ`)
- `infra/conf-files/` – example config files

## Prerequisites

- Python 3.10+
- Typst (for PDF output)
- PlatformIO / Arduino toolchain for ESP32 firmware
- Ansible (for infrastructure automation)

## Dashboard server (local)

From repo root:

```bash
cd dashboard-server
make dev_server
```

Default MQTT is `127.0.0.1:1883`. Override if needed:

```bash
make dev_server MQTT_HOST=143.198.250.135 MQTT_PORT=1883
```

Run without MQTT (useful when offline):

```bash
make dev_server_no_mqtt
```

Or run directly:

```bash
cd dashboard-server
MQTT_HOST=127.0.0.1 MQTT_PORT=1883 MQTT_ENABLED=1 \
uvicorn server:app --host 0.0.0.0 --port 8000
```

Optional env vars:

```bash
INFLUX_URL=http://localhost:8086 INFLUX_TOKEN=supersecrettoken \
MQTT_HOST=mqtt.narbot.ee MQTT_PORT=1883 MQTT_ENABLED=1 \
uvicorn server:app --host 0.0.0.0 --port 8000
```

## ESP32 firmware

The firmware lives under `PressureSensor/` and uses MQTT + HTTP. The typical flow is:

1. Open `PressureSensor/` in PlatformIO.
2. Build and flash to the M5 device.
3. Provision WiFi using the device AP mode if credentials are missing.

Key MQTT settings live in `PressureSensor/src/main.cpp`:

- `mqttHost`: `mqtt.narbot.ee`
- `mqttPort`: `1883`
- `pressureTopic`: `pressure/testing`

## Infra (Ansible)

Inventory:

- `infra/inventory.ini` – DigitalOcean droplet IP

Playbooks:

- `infra/install_mosquitto.yml` – install Mosquitto (anonymous MQTT enabled)
- `infra/install_influxdb.yml` – install InfluxDB 2.x
- `infra/deploy_dashboard.yml` – deploy dashboard server as systemd service
- `infra/install_caddy.yml` – Caddy reverse proxy + TLS

Run example:

```bash
ansible-playbook -i infra/inventory.ini infra/install_mosquitto.yml
```

## Architecture docs

- Source: `docs/ARCHITECTURE.md`
- PNGs: `docs/diagrams/*.png`
- Typst: `docs/ARCHITECTURE.typ`

Rebuild Typst PDF:

```bash
typst compile docs/ARCHITECTURE.typ docs/ARCHITECTURE.pdf
```

## Notes

- MQTT auth is disabled in the Mosquitto playbook (`allow_anonymous true`).
- Dashboard server currently reads the latest value from MQTT in memory; InfluxDB writes/queries are not wired yet.
