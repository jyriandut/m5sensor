# Arhitektuur

See dokument sisaldab süsteemi arhitektuuri jooniseid MermaidJS formaadis (soovitatav renderdada GitHubis või Mermaid Live Editoris).

## Infrastruktuuri diagramm

```mermaid
flowchart LR
  %% Styles
  classDef user fill:#FFE4C4,stroke:#333,stroke-width:1px;
  classDef network fill:#E0F7FA,stroke:#00796B,stroke-width:1px;
  classDef cloud fill:#E8EAF6,stroke:#3F51B5,stroke-width:1px;
  classDef device fill:#F1F8E9,stroke:#558B2F,stroke-width:1px;
  classDef service fill:#FFF3E0,stroke:#E65100,stroke-width:1px;
  classDef db fill:#F3E5F5,stroke:#6A1B9A,stroke-width:1px;
  classDef auth fill:#E1BEE7,stroke:#4A148C,stroke-width:1px;

  User["Kasutaja<br/>Brauser<br/>(avalik IP)"]:::user
  Internet((Internet)):::network
  Router["Kodune ruuter (Cudy)<br/>(NAT, väline IP)<br/>Port forward -> ESP32:8080"]:::network
  ESP32["ESP32 seade<br/>(VPN klient: 10.8.0.2)<br/>Web server :8080"]:::device

  subgraph DO["Digital Ocean droplet<br/>(VPN server + Keskserver + MQTT broker + InfluxDB)<br/>Public IP: 143.198.250.135<br/>VPN GW: 10.8.0.1"]
    VPN["OpenVPN server"]:::auth
    Caddy["Caddy reverse proxy<br/>TLS :443<br/>hello.narbot.ee<br/>dashboard.narbot.ee<br/>esp.narbot.ee"]:::service
    Flask["Flask Keskserver<br/>:8000 (reverse proxy kaudu)"]:::service
    MQTT["Mosquitto MQTT broker<br/>TCP :1883"]:::service
    Influx["InfluxDB<br/>HTTP API :8086"]:::db
  end
  class DO cloud;

  User -->|HTTPS :443| Internet
  Internet --> Router
  Router --> ESP32
  Internet --> DO

  ESP32 -->|MQTT publish :1883 VPN<br/>Staatuse sõnumid| MQTT
  MQTT -->|Forward| Flask
  Flask -->|Write| Influx
  User -->|Seadmete nimekiri<br/>/ UI| Caddy
  Caddy -->|reverse_proxy 127.0.0.1:8000| Flask
  Caddy -->|reverse_proxy 10.8.0.2:8080| ESP32

  %% Auth layers
  ESP32 -.->|VPN autentimine<br/>OpenVPN| VPN
  User -.->|Keskserveri login<br/>Flask username/password| Flask
  ESP32 -.->|MQTT auth<br/>hetkel puudub| MQTT
```

## Software stack diagramm

### ESP32 stack

```mermaid
flowchart TB
  classDef layer fill:#F1F8E9,stroke:#558B2F,stroke-width:1px;
  classDef mid fill:#E3F2FD,stroke:#1565C0,stroke-width:1px;
  classDef hw fill:#ECEFF1,stroke:#37474F,stroke-width:1px;

  ESPLogic["Loogika (Arduino)<br/>- Valve control (GPIO)<br/>- Pressure reading (ADC)<br/>- MQTT publishing"]:::layer
  ESPLibs["Library kihid<br/>- LittleFS (HTML template'id)<br/>- PubSubClient (MQTT klient)<br/>- NTPClient (aja sünkronisatsioon)"]:::layer
  ESPFW["Arduino/ESP-IDF framework"]:::mid
  ESPHW["ESP32 Hardware<br/>(WiFi, ADC, GPIO, Flash, NVS)"]:::hw

  ESPLogic --> ESPLibs --> ESPFW --> ESPHW

  ESPLogic -->|MQTT publish :1883| MQTTBroker[("Mosquitto MQTT Broker<br/>DO droplet")]:::mid
```

### Keskserveri stack

```mermaid
flowchart TB
  classDef ui fill:#FFF3E0,stroke:#E65100,stroke-width:1px;
  classDef service fill:#E8EAF6,stroke:#3F51B5,stroke-width:1px;
  classDef db fill:#F3E5F5,stroke:#6A1B9A,stroke-width:1px;
  classDef mqtt fill:#E0F7FA,stroke:#00796B,stroke-width:1px;

  Browser["Kasutaja brauser<br/>(HTML, CSS, Chart.js)"]:::ui
  Caddy["Caddy reverse proxy<br/>TLS :443"]:::service
  Flask["Flask veebirakendus<br/>Routes:<br/>- /<br/>- /device/&lt;id&gt;/history<br/>- /export"]:::service
  Jinja["Jinja2 template rendering"]:::service
  InfluxClient["InfluxDB client library<br/>(päringud, lugemine)"]:::service
  Influx["InfluxDB :8086<br/>Bucket: syringe_bucket<br/>Measurement: device_status"]:::db
  Listener["MQTT Listener<br/>(Python script/thread)"]:::mqtt
  Broker["Mosquitto MQTT Broker :1883"]:::mqtt
  ESP32["ESP32 seadmed<br/>(PubSubClient)"]:::mqtt

  Browser -->|HTTPS :443| Caddy
  Caddy -->|reverse_proxy 127.0.0.1:8000| Flask
  Caddy -->|reverse_proxy 10.8.0.2:8080| ESP32
  Flask --> Jinja --> InfluxClient --> Influx
  Listener --> Influx
  Broker --> Listener
  ESP32 -->|MQTT publish| Broker
  class Caddy service;
```

## Suhtlus (sequence diagrammid)

### Stsenaarium 1: ESP32 saadab staatust

```mermaid
sequenceDiagram
  participant ESP32 as ESP32
  participant MQTT as MQTT Broker
  participant Listener as MQTT Listener
  participant Influx as InfluxDB

  ESP32->>ESP32: mõõtmine → buffer
  ESP32->>MQTT: publish buffer[0] (VPN)
  MQTT-->>ESP32: OK
  ESP32->>ESP32: eemalda buffer[0]
  MQTT->>Listener: forward
  Listener->>Influx: write

  ESP32->>ESP32: mõõtmine → buffer
  ESP32->>MQTT: publish buffer[0]
  MQTT--x ESP32: WiFi katkeb, FAIL

  ESP32->>ESP32: mõõtmine → buffer
  ESP32->>ESP32: mõõtmine → buffer

  Note over ESP32: WiFi taastub
  ESP32->>MQTT: publish buffer[0]
  MQTT-->>ESP32: OK
  ESP32->>ESP32: eemalda buffer[0]
  ESP32->>MQTT: publish buffer[1]
  MQTT-->>ESP32: OK
  ESP32->>ESP32: eemalda buffer[1]
```

### Stsenaarium 2: Kasutaja vaatab seadmete nimekirja

```mermaid
sequenceDiagram
  participant Browser as Browser
  participant Flask as Flask Server
  participant Influx as InfluxDB

  Browser->>Caddy: GET /
  Caddy->>Flask: reverse_proxy 127.0.0.1:8000
  Flask->>Influx: query last status
  Influx-->>Flask: result
  Flask-->>Browser: HTML + data (Jinja2)
```

### Stsenaarium 3: Kasutaja juhib ESP32 klappi

```mermaid
sequenceDiagram
  participant Button as Nupp
  participant ESP32 as ESP32 Loogika
  participant Caddy as Caddy
  participant MQTT as MQTT Broker
  participant Listener as MQTT Listener
  participant Influx as InfluxDB

  Button->>ESP32: press (GPIO interrupt)
  ESP32->>ESP32: toggle valve state (GPIO)
  ESP32->>MQTT: publish topic sensors/{uid}/status\n{"valve_state":"open","pressure_30ms_ago":2.1,"pressure_now":4.8}
  MQTT->>Listener: forward
  Listener->>Influx: write Point(...)
```

## Tehnoloogiate legend

- OpenVPN: VPN protokoll (UDP/TCP)
- MQTT: Message broker protokoll (TCP, port 1883)
- MQTT autentimine: hetkel puudub
- Mosquitto: MQTT broker tarkvara
- InfluxDB: Time-series andmebaas (HTTP API, port 8086)
- Caddy: Reverse proxy + TLS terminatsioon (HTTPS :443)
- Flask: Python web framework
- Jinja2: Template rendering (Flask sees)
- NTP: Network Time Protocol (aja sünkronisatsioon)
- LittleFS: ESP32 failisüsteem
- NVS: ESP32 non-volatile storage (WiFi kredentsiaalid)
- PubSubClient: Arduino MQTT kliendi library
- Chart.js: JavaScript graafikute teek

## Miks need diagrammid on olulised

Need diagrammid dokumenteerivad süsteemi arhitektuuri viisil, mida on võimalik:

- Näidata, et sa mõistad kogu süsteemi terviklikult
- Kasutada ise tulevikus kui pead süsteemi uuesti üles seadma
- Jagada teiste tudengitega, et nad saaksid sinu lahendusest õppida
- Kasutada alusena kui tahad süsteemi laiendada (nt lisada uusi sensoreid)

Diagrammid peaksid olema piisavalt detailsed, et keegi teine saaks nende põhjal süsteemi üles ehitada, kuid mitte nii detailsed, et muutuvad loetamatuks. Kasuta värve, et eristada erinevaid komponente ja andmevooge.
