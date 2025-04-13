#!/bin/bash

docker compose up --build -d

echo "Attaching to gateway logs (press Ctrl+C to stop)..."
docker logs -f gateway
