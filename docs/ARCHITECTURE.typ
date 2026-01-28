#set page(margin: 2.2cm)
#set text(font: "Helvetica", size: 11pt)

= Arhitektuur

See dokument koondab süsteemi arhitektuuri diagrammid. Mermaid-diagrammid on asendatud renderdatud PNG-piltidega.

== Infrastruktuuri diagramm

#figure(
  image("diagrams/infra.png", width: 100%),
  caption: [Infrastruktuuri diagramm (võrk, IP-d, teenused, autentimine)]
)

== Software stack diagrammid

=== ESP32 stack

#figure(
  image("diagrams/stack_esp32.png", width: 100%),
  caption: [ESP32 tehnoloogiakihtide diagramm]
)

=== Keskserveri stack

#figure(
  image("diagrams/stack_server.png", width: 100%),
  caption: [Keskserveri tehnoloogiakihtide diagramm]
)

== Suhtlus (sequence diagrammid)

=== Stsenaarium 1: ESP32 saadab staatust

#figure(
  image("diagrams/seq_esp32_status.png", width: 100%),
  caption: [ESP32 staatussõnumite edastus ja võrgu katkestuse käitumine]
)

=== Stsenaarium 2: Kasutaja vaatab seadmete nimekirja

#figure(
  image("diagrams/seq_device_list.png", width: 100%),
  caption: [Seadmete nimekirja päring ja vastus]
)

=== Stsenaarium 3: Kasutaja juhib ESP32 klappi

#figure(
  image("diagrams/seq_valve_control.png", width: 100%),
  caption: [Klapi juhtimise sündmusvoog]
)

== Tehnoloogiate legend

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

== Miks need diagrammid on olulised

Need diagrammid dokumenteerivad süsteemi arhitektuuri viisil, mida on võimalik:

- Näidata, et sa mõistad kogu süsteemi terviklikult
- Kasutada ise tulevikus kui pead süsteemi uuesti üles seadma
- Jagada teiste tudengitega, et nad saaksid sinu lahendusest õppida
- Kasutada alusena kui tahad süsteemi laiendada (nt lisada uusi sensoreid)

Diagrammid peaksid olema piisavalt detailsed, et keegi teine saaks nende põhjal süsteemi üles ehitada, kuid mitte nii detailsed, et muutuvad loetamatuks.
