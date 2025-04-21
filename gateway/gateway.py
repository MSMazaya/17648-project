import socket
import threading
import os
import json
import time
from datetime import datetime
import paho.mqtt.client as mqtt

DEFAULT_TCP_PORT = 9010
DEFAULT_MQTT_HOST = "mqtt"
DEFAULT_MQTT_PORT = 1883
BUFFER_SIZE = 1024

vehicle_table = {}
table_lock = threading.Lock()


def log(msg):
    now = datetime.now().strftime("%H:%M:%S")
    print(f"[{now}] {msg}", flush=True)


def print_vehicle_table():
    log("Vehicle table:")
    with table_lock:
        for vehicle_id, info in vehicle_table.items():
            status = "Connected" if info["connected"] else "Disconnected"
            ip = info["ip"]
            log(f"  - {vehicle_id}: {status} ({ip})")


mqtt_host = os.environ.get("MQTT_BROKER_HOST", DEFAULT_MQTT_HOST)
mqtt_port = int(os.environ.get("MQTT_BROKER_PORT", DEFAULT_MQTT_PORT))

mqtt_client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)


def handle_client(conn, addr):
    vehicle_id = None
    try:
        data = conn.recv(BUFFER_SIZE).decode().strip()
        if not data.startswith("vehicle_id:"):
            log(f"Rejected connection from {addr[0]}: no vehicle_id")
            conn.close()
            return

        vehicle_id = data.split("\n", 1)[0].strip()
        with table_lock:
            vehicle_table[vehicle_id] = {
                "ip": addr[0],
                "connected": True,
                "socket": conn,
            }

        log(f"Connected: {vehicle_id} ({addr[0]})")
        print_vehicle_table()

        while True:
            data = conn.recv(BUFFER_SIZE)
            if not data:
                break
            payload = data.decode().strip()
            log(f"[{vehicle_id}] {payload}")

            try:
                parsed = json.loads(payload)
                for key, value in parsed.items():
                    topic = f"vehicles/{vehicle_id}/sensor/{key}"
                    mqtt_client.publish(topic, str(value))
            except json.JSONDecodeError:
                log(f"[WARN] Invalid JSON from {vehicle_id}")

    except Exception as e:
        log(f"[ERROR] Client {addr[0]}: {e}")
    finally:
        if vehicle_id:
            with table_lock:
                if vehicle_id in vehicle_table:
                    vehicle_table[vehicle_id]["connected"] = False
            log(f"Disconnected: {vehicle_id}")
            print_vehicle_table()
        conn.close()


def start_server(port):
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.bind(("0.0.0.0", port))
    server_socket.listen()
    log(f"Gateway listening on TCP port {port}...")

    while True:
        conn, addr = server_socket.accept()
        threading.Thread(target=handle_client, args=(
            conn, addr), daemon=True).start()


if __name__ == "__main__":
    mqtt_client.connect(mqtt_host, mqtt_port)
    mqtt_client.loop_start()
    tcp_port = int(os.environ.get("GATEWAY_PORT", DEFAULT_TCP_PORT))
    start_server(tcp_port)
