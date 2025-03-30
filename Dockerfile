FROM ubuntu:22.04

RUN apt-get update && apt-get install -y build-essential

WORKDIR /app
COPY vapi.c .

RUN gcc -o vapi vapi.c

CMD ["./vapi"]
