FROM ubuntu:22.04

RUN apt-get update && apt-get install -y build-essential

WORKDIR /app
COPY gateway.c .

RUN gcc -o gateway gateway.c

CMD ["./gateway"]
