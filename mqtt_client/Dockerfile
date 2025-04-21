FROM python:3.11-slim

WORKDIR /app

RUN pip install paho-mqtt

COPY mqtt_client.py .

CMD ["python3", "mqtt_client.py"]
