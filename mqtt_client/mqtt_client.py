import os
import paho.mqtt.client as mqtt
import threading
import time

BROKER_HOST = os.environ.get("MQTT_BROKER_HOST", "localhost")
BROKER_PORT = int(os.environ.get("MQTT_BROKER_PORT", "1883"))

subscribed = set()
seen_topics = set()
lock = threading.Lock()


def on_connect(client, userdata, flags, rc):
    print(f"Connected to MQTT broker at {BROKER_HOST}:{BROKER_PORT}")
    client.subscribe("vehicles/+/sensor/#")


def on_message(client, userdata, msg):
    topic = msg.topic
    with lock:
        seen_topics.add(topic)
    if topic in subscribed:
        print(f"[{topic}] {msg.payload.decode()}")


def input_loop(client):
    while True:
        try:
            cmd = input("> ").strip()
            if cmd == "exit":
                break
            elif cmd == "list":
                print("Known topics:")
                with lock:
                    for t in sorted(seen_topics):
                        marker = "*" if t in subscribed else " "
                        print(f"{marker} {t}")
            elif cmd.startswith("sub "):
                topic = cmd[4:].strip()
                with lock:
                    subscribed.add(topic)
                print(f"Subscribed to {topic}")
            elif cmd.startswith("unsub "):
                topic = cmd[6:].strip()
                with lock:
                    if topic in subscribed:
                        subscribed.remove(topic)
                print(f"Unsubscribed from {topic}")
            else:
                print("Commands: list, sub <topic>, unsub <topic>, exit")
        except (EOFError, KeyboardInterrupt):
            break


def main():
    time.sleep(2)
    client = mqtt.Client()
    client.on_connect = on_connect
    client.on_message = on_message
    client.connect(BROKER_HOST, BROKER_PORT)
    threading.Thread(target=client.loop_forever, daemon=True).start()
    input_loop(client)


if __name__ == "__main__":
    main()
