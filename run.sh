#!/bin/bash

docker compose up --build -d

echo "[INFO] Subscribing to all MQTT topics:"
docker exec -it gateway mosquitto_sub -h mqtt -p 1883 -t '#' -v
