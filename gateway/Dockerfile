FROM python:3.10-slim

WORKDIR /app
COPY gateway.py .

RUN pip install paho-mqtt
RUN apt update -y && apt install mosquitto-clients -y

CMD ["python", "gateway.py"]
