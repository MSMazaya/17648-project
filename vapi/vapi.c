#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>

#define SERIAL_LINE_BUF 128
#define SERIAL_LINE_BUF 128
#define TCP_BUF_SIZE 512

int gateway_fd = -1;

// ================== Serial TCP/IP Layer  ===================
// Utility functions for TCP/IP communication using socket API

int get_vapi_port() {
    const char *env = getenv("VAPI_PORT");
    return env ? atoi(env) : 9000;
}

void init_tcp_server() {
    int server_fd;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);
    int port = get_vapi_port();

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { perror("socket failed"); exit(1); }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(1);
    }

    listen(server_fd, 1);
    printf("Vehicle API TCP server listening on port %d...\n", port);

    gateway_fd = accept(server_fd, (struct sockaddr*)&address, &addrlen);
    if (gateway_fd < 0) {
        perror("accept failed");
        exit(1);
    }

    printf("Gateway connected.\n");
}

void tcp_send(const char *msg) {
    if (gateway_fd != -1) {
        send(gateway_fd, msg, strlen(msg), 0);
    }
}

void tcp_send_line(const char *msg) {
    char buf[TCP_BUF_SIZE];
    snprintf(buf, sizeof(buf), "%s\n", msg);
    tcp_send(buf);
}

// ================== Serial I/O Layer ===================
// This is purposefully being named like a serial interface communication.
// The idea is to provide extensability so that if I need to change this
// to actually use serial communication in the future, it will be easy.

void serial_print(const char *s) {
    fputs(s, stdout);
    fflush(stdout);
}

void serial_print_line(const char *s) {
    fputs(s, stdout);
    fputs("\n", stdout);
    fflush(stdout);
}

int serial_read_line(char *buf, int maxlen) {
    if (fgets(buf, maxlen, stdin) != NULL) {
        buf[strcspn(buf, "\r\n")] = 0;
        return 1;
    }
    return 0;
}

int serial_input_available() {
    fd_set fds;
    struct timeval tv = {0, 0};
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    return select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) > 0;
}

// ================== Vehicle API Core ===================
// The required packed struct definition with randomization 
// for simulation purposes of the vehicle data.

typedef struct __attribute__((packed)) {
    int16_t oil_temp_f;
    uint16_t maf_raw;
    uint8_t battery_voltage;
    uint16_t tire_pressure_raw;
    uint16_t fuel_level_liters;
    uint8_t fuel_consumption_rate;
    uint32_t error_codes;
} VehicleData;

uint16_t random_u16(uint16_t max) { return rand() % (max + 1); }
uint8_t random_u8(uint8_t max) { return rand() % (max + 1); }
int16_t random_i16(int16_t min, int16_t max) { return min + rand() % (max - min + 1); }

VehicleData generate_data() {
    VehicleData d = {
        .oil_temp_f = random_i16(180, 250),
        .maf_raw = random_u16(2047),
        .battery_voltage = random_u8(12),
        .tire_pressure_raw = random_u16(2047),
        .fuel_level_liters = random_u16(100),
        .fuel_consumption_rate = random_u8(50),
        .error_codes = (random_u8(255) << 24) |
                       (random_u8(255) << 16) |
                       (random_u8(255) << 8)  |
                       random_u8(255)
    };
    return d;
}

// ================== User Interaction ===================

void print_json(const VehicleData *d) {
    char out[512];
    snprintf(out, sizeof(out),
        "{\"oil_temp_f\": %d, \"maf_raw\": %u, \"maf_cfm\": %.2f, "
        "\"battery_voltage\": %u, \"tire_pressure_raw\": %u, \"tire_pressure_psi\": %.2f, "
        "\"fuel_level_liters\": %u, \"fuel_consumption_rate\": %u, \"error_codes\": \"0x%08X\"}\n",
        d->oil_temp_f,
        d->maf_raw, d->maf_raw * (2500.0 / 2047.0),
        d->battery_voltage,
        d->tire_pressure_raw, d->tire_pressure_raw * (100.0 / 2047.0),
        d->fuel_level_liters,
        d->fuel_consumption_rate,
        d->error_codes
    );
    serial_print(out);
}

void send_json(const VehicleData *d) {
    char out[TCP_BUF_SIZE];
    snprintf(out, sizeof(out),
        "{\"oil_temp_f\": %d, \"maf_raw\": %u, \"maf_cfm\": %.2f, "
        "\"battery_voltage\": %u, \"tire_pressure_raw\": %u, \"tire_pressure_psi\": %.2f, "
        "\"fuel_level_liters\": %u, \"fuel_consumption_rate\": %u, \"error_codes\": \"0x%08X\"}\n",
        d->oil_temp_f,
        d->maf_raw, d->maf_raw * (2500.0 / 2047.0),
        d->battery_voltage,
        d->tire_pressure_raw, d->tire_pressure_raw * (100.0 / 2047.0),
        d->fuel_level_liters,
        d->fuel_consumption_rate,
        d->error_codes
    );
    tcp_send(out);
}

int main() {
    srand(time(NULL));
    init_tcp_server();

    while (1) {
        VehicleData data = generate_data();
        send_json(&data);
        sleep(2);
    }

    close(gateway_fd);
    return 0;
}
