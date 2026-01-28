MQTT_HOST ?= 127.0.0.1
MQTT_PORT ?= 1883

activate:
	source .venv/bin/activate
install:
	pip install -r requirements.txt

dev_server:
	MQTT_HOST=$(MQTT_HOST) MQTT_PORT=$(MQTT_PORT) MQTT_ENABLED=$(MQTT_ENABLED) fastapi dev server.py

dev_server_no_mqtt:
	MQTT_ENABLED=0 fastapi dev server.py
MQTT_ENABLED ?= 1
