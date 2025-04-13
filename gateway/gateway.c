#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define DEFAULT_GATEWAY_PORT 9010
#define MAX_VEHICLES 64
#define BUFFER_SIZE 1024
#define ID_MAX_LEN 64

typedef struct {
    int socket;
    char id[ID_MAX_LEN];
    struct sockaddr_in addr;
    int connected;
} VehicleEntry;

VehicleEntry vehicle_table[MAX_VEHICLES];
pthread_mutex_t table_lock = PTHREAD_MUTEX_INITIALIZER;

int get_gateway_port() {
    const char* env = getenv("GATEWAY_PORT");
    return env ? atoi(env) : DEFAULT_GATEWAY_PORT;
}

void print_vehicle_table() {
    printf("\n[INFO] Vehicle table:\n");
    pthread_mutex_lock(&table_lock);
    for (int i = 0; i < MAX_VEHICLES; i++) {
        if (vehicle_table[i].connected) {
            printf("  - %s : Connected (%s)\n",
                   vehicle_table[i].id,
                   inet_ntoa(vehicle_table[i].addr.sin_addr));
        }
    }
    pthread_mutex_unlock(&table_lock);
    printf("\n");
}

int register_vehicle(int client_fd, struct sockaddr_in client_addr, const char* id) {
    pthread_mutex_lock(&table_lock);
    for (int i = 0; i < MAX_VEHICLES; i++) {
        if (!vehicle_table[i].connected) {
            vehicle_table[i].socket = client_fd;
            strncpy(vehicle_table[i].id, id, ID_MAX_LEN - 1);
            vehicle_table[i].addr = client_addr;
            vehicle_table[i].connected = 1;
            pthread_mutex_unlock(&table_lock);
            return i;
        }
    }
    pthread_mutex_unlock(&table_lock);
    return -1;
}

void unregister_vehicle(int index) {
    pthread_mutex_lock(&table_lock);
    if (vehicle_table[index].connected) {
        close(vehicle_table[index].socket);
        vehicle_table[index].connected = 0;
        printf("[INFO] Disconnected: %s\n", vehicle_table[index].id);
    }
    pthread_mutex_unlock(&table_lock);
    print_vehicle_table();
}

void* handle_vehicle(void* arg) {
    int client_fd = *(int*)arg;
    free(arg);
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    getpeername(client_fd, (struct sockaddr*)&client_addr, &addr_len);

    char buffer[BUFFER_SIZE];
    int index = -1;
    char vehicle_id[ID_MAX_LEN] = "UNKNOWN";

    int bytes = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
    if (bytes <= 0) {
        close(client_fd);
        return NULL;
    }
    buffer[bytes] = '\0';

    if (strncmp(buffer, "vehicle_id:", 11) == 0) {
        strncpy(vehicle_id, buffer + 12, ID_MAX_LEN - 1);
        vehicle_id[strcspn(vehicle_id, "\r\n")] = 0;  // strip newline
        index = register_vehicle(client_fd, client_addr, vehicle_id);
        printf("[INFO] Connected: %s (%s)\n", vehicle_id, inet_ntoa(client_addr.sin_addr));
        print_vehicle_table();
    } else {
        printf("[WARN] No vehicle_id received from %s\n", inet_ntoa(client_addr.sin_addr));
        close(client_fd);
        return NULL;
    }

    while (1) {
        bytes = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
        if (bytes <= 0) break;
        buffer[bytes] = '\0';
        printf("[%s] %s", vehicle_id, buffer);
        fflush(stdout);
    }

    if (index >= 0) {
        unregister_vehicle(index);
    } else {
        close(client_fd);
    }

    return NULL;
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addrlen = sizeof(client_addr);

    int port = get_gateway_port();

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { perror("socket failed"); exit(1); }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        exit(1);
    }

    listen(server_fd, 5);
    printf("[GATEWAY] Listening for vehicles on port %d...\n", port);

    while (1) {
        client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addrlen);
        if (client_fd < 0) {
            perror("accept failed");
            continue;
        }

        int* pclient = malloc(sizeof(int));
        *pclient = client_fd;
        pthread_t tid;
        pthread_create(&tid, NULL, handle_vehicle, pclient);
        pthread_detach(tid);
    }

    close(server_fd);
    return 0;
}
