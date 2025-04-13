import socket
import threading
import os
from datetime import datetime

DEFAULT_PORT = 9010
MAX_VEHICLES = 64
BUFFER_SIZE = 1024

vehicle_table = {}
table_lock = threading.Lock()


def get_gateway_port():
    return int(os.environ.get("GATEWAY_PORT", DEFAULT_PORT))


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


def handle_client(conn, addr):
    try:
        # Expect first message to be vehicle_id
        data = conn.recv(BUFFER_SIZE).decode().strip()
        if not data.startswith("vehicle_id:"):
            log(f"Rejected connection from {addr[0]}: no vehicle_id")
            conn.close()
            return

        vehicle_id = data.split(":", 1)[1].strip()

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
            log(f"{vehicle_id}: {data.decode().strip()}")

    except Exception as e:
        log(f"Error with {addr[0]}: {e}")
    finally:
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
    log(f"Gateway listening on port {port}...")

    while True:
        conn, addr = server_socket.accept()
        threading.Thread(target=handle_client, args=(
            conn, addr), daemon=True).start()


if __name__ == "__main__":
    start_server(get_gateway_port())
