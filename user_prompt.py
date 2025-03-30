#!/usr/bin/env python3

import subprocess
import threading
import time

proc = subprocess.Popen(
    ["docker", "run", "--rm", "-i", "vapi"],
    stdin=subprocess.PIPE,
    stdout=subprocess.PIPE,
    text=True,
    bufsize=1
)


def reader_thread():
    for line in proc.stdout:
        print("VAPI:", line.strip())


threading.Thread(target=reader_thread, daemon=True).start()

print("Connected to Vehicle API. Commands: GET, START PUSH, STOP, EXIT")
while True:
    cmd = input("> ").strip()
    if cmd.upper() == "EXIT":
        break
    proc.stdin.write(cmd + "\n")
    proc.stdin.flush()

proc.terminate()
