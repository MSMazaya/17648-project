#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>

#define SERIAL_LINE_BUF 128
#define TCP_BUF_SIZE 512
#define DEFAULT_GATEWAY_HOST "localhost"
#define DEFAULT_GATEWAY_PORT 9010
#define TCP_BUF_SIZE 512

int gateway_fd = -1;

// ================== Utility Functions  =====================
const char* get_env_or_default(const char* var, const char* def) {
    const char* val = getenv(var);
    return val ? val : def;
}

// ================== Serial TCP/IP Layer  ===================
// Utility functions for TCP/IP communication using socket API

int connect_to_gateway(const char* host, int port) {
    struct sockaddr_in server_addr;
    struct hostent* gateway_host;

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) { perror("socket failed"); exit(1); }

    gateway_host = gethostbyname(host);
    if (!gateway_host) {
        fprintf(stderr, "Failed to resolve gateway host: %s\n", host);
        exit(1);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    memcpy(&server_addr.sin_addr, gateway_host->h_addr, gateway_host->h_length);
    server_addr.sin_port = htons(port);

    while (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection to gateway failed");
        sleep(2);
        /* exit(1); */
    }

    return sockfd;
}

void send_vehicle_id(int sockfd, const char* id) {
    char msg[128];
    snprintf(msg, sizeof(msg), "vehicle_id: %s\n", id);
    send(sockfd, msg, strlen(msg), 0);
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

void send_json_data(int sockfd, const VehicleData* d) {
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
    send(sockfd, out, strlen(out), 0);
}

int main() {
    srand(time(NULL));

    const char* vehicle_id = get_env_or_default("VEHICLE_ID", "default_vehicle");
    const char* host = get_env_or_default("GATEWAY_HOST", DEFAULT_GATEWAY_HOST);
    int port = atoi(get_env_or_default("GATEWAY_PORT", "9010"));

    
    int sockfd = connect_to_gateway(host, port);
    send_vehicle_id(sockfd, vehicle_id);

    while (1) {
        VehicleData d = generate_data();
        send_json_data(sockfd, &d);
        sleep(2);
    }

    close(sockfd);
    return 0;
}
