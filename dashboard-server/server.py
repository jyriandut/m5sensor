from contextlib import asynccontextmanager
from datetime import datetime, datetime
import os
from fastapi import FastAPI, Request
from fastapi.responses import HTMLResponse
from fastapi.staticfiles import StaticFiles
from fastapi.templating import Jinja2Templates
from influxdb_client import InfluxDBClient, Point, WritePrecision
from gmqtt import Client as MQTTClient
from fastapi_mqtt import FastMQTT, MQTTConfig
import logging
from typing import Any

# --------------------
# InfluxDB config
# --------------------
INFLUX_URL = os.getenv("INFLUX_URL", "http://localhost:8086")
INFLUX_TOKEN = os.getenv("INFLUX_TOKEN", "supersecrettoken")
INFLUX_ORG = os.getenv("INFLUX_ORG", "narbot")
INFLUX_BUCKET = os.getenv("INFLUX_BUCKET", "narbot-bucket")

# MQTT
MQTT_HOST = os.getenv("MQTT_HOST", "mqtt.narbot.ee")
MQTT_PORT = int(os.getenv("MQTT_PORT", "1883"))
MQTT_USERNAME = os.getenv("MQTT_USERNAME", "")
MQTT_PASSWORD = os.getenv("MQTT_PASSWORD", "")

client = InfluxDBClient(
    url=INFLUX_URL,
    token=INFLUX_TOKEN,
    org=INFLUX_ORG,
)

write_api = client.write_api()
query_api = client.query_api()

mqtt_config = MQTTConfig(
    host=MQTT_HOST,
    port=MQTT_PORT,
    username=MQTT_USERNAME or None,
    password=MQTT_PASSWORD or None,
)

fast_mqtt = FastMQTT(config=mqtt_config)

#fast_mqtt.init_app(app)


@asynccontextmanager
async def _lifespan(_app: FastAPI):
    await fast_mqtt.mqtt_startup()
    yield
    await fast_mqtt.mqtt_shutdown()

logger = logging.getLogger
app = FastAPI(lifespan=_lifespan)
templates = Jinja2Templates(directory="templates")



@fast_mqtt.on_connect()
def handle_connect(client, flags, rc, properties):
    pprint("Connected to MQTT with code:", rc)

@fast_mqtt.on_disconnect()
def handle_disconnect(client, packet, exc=None):
    print("Disconnected from MQTT", exc)

@fast_mqtt.on_subscribe()
def handle_subscribe(client, mid, qos, properties):
    print("Subscribed, mid =", mid, "qos =", qos)

@fast_mqtt.subscribe("devices/+/init")
async def device_status_handler(client: MQTTClient, topic: str, payload: bytes, qos: int, properties: Any):
    print("device/init: ", topic, payload.decode(), qos, properties)


@app.get("/", response_class=HTMLResponse)
async def index(request: Request):
    return templates.TemplateResponse(request=request, name="index.html")
