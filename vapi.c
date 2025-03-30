#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/select.h>

#define SERIAL_LINE_BUF 128

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

void print_field(const VehicleData *d, const char *field) {
    char out[128];

    if (strcmp(field, "oil_temp") == 0)
        snprintf(out, sizeof(out), "%d\n", d->oil_temp_f);
    else if (strcmp(field, "maf_raw") == 0)
        snprintf(out, sizeof(out), "%u\n", d->maf_raw);
    else if (strcmp(field, "maf_cfm") == 0)
        snprintf(out, sizeof(out), "%.2f\n", d->maf_raw * (2500.0 / 2047.0));
    else if (strcmp(field, "battery_voltage") == 0)
        snprintf(out, sizeof(out), "%u\n", d->battery_voltage);
    else if (strcmp(field, "tire_pressure_raw") == 0)
        snprintf(out, sizeof(out), "%u\n", d->tire_pressure_raw);
    else if (strcmp(field, "tire_pressure_psi") == 0)
        snprintf(out, sizeof(out), "%.2f\n", d->tire_pressure_raw * (100.0 / 2047.0));
    else if (strcmp(field, "fuel_level_liters") == 0)
        snprintf(out, sizeof(out), "%u\n", d->fuel_level_liters);
    else if (strcmp(field, "fuel_consumption_rate") == 0)
        snprintf(out, sizeof(out), "%u\n", d->fuel_consumption_rate);
    else if (strcmp(field, "error_codes") == 0)
        snprintf(out, sizeof(out), "0x%08X\n", d->error_codes);
    else
        snprintf(out, sizeof(out), "ERROR: Unknown field '%s'\n", field);

    serial_print(out);
}

int main() {
    srand(time(NULL));
    char line[SERIAL_LINE_BUF];
    int pushing = 0;
    VehicleData latest = generate_data();

    while (serial_read_line(line, sizeof(line))) {
        if (strncmp(line, "GET", 3) == 0) {
            char *arg = line + 4;
            if (strcmp(arg, "ALL") == 0) {
                latest = generate_data();
                print_json(&latest);
            } else {
                print_field(&latest, arg);
            }

        } else if (strncmp(line, "START PUSH", 10) == 0) {
            pushing = 1;
            while (pushing) {
                latest = generate_data();
                print_json(&latest);
                sleep(2);

                if (serial_input_available()) {
                    if (serial_read_line(line, sizeof(line))) {
                        if (strncmp(line, "STOP", 4) == 0)
                            pushing = 0;
                    }
                }
            }

        } else if (strncmp(line, "STOP", 4) == 0) {
            pushing = 0;
        } else {
            serial_print_line("UNKNOWN COMMAND");
        }
    }
    return 0;
}