#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define DEFAULT_HOST "vapi"
#define DEFAULT_PORT 9000
#define BUFFER_SIZE 1024

const char *get_vapi_host() {
  const char *env = getenv("VAPI_HOST");
  return env ? env : DEFAULT_HOST;
}

int get_vapi_port() {
  const char *env = getenv("VAPI_PORT");
  return env ? atoi(env) : DEFAULT_PORT;
}

int main() {
  int sockfd;
  struct sockaddr_in server_addr;
  char buffer[BUFFER_SIZE];

  const char *server_ip = get_vapi_host();
  int server_port = get_vapi_port();

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    perror("socket failed");
    exit(1);
  }

  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(server_port);

  if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
    struct hostent *host = gethostbyname(server_ip);
    if (!host) {
      fprintf(stderr, "Failed to resolve host: %s\n", server_ip);
      exit(1);
    }
    memcpy(&server_addr.sin_addr, host->h_addr, host->h_length);
  }

  if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) <
      0) {
    perror("Connection failed");
    exit(1);
  }

  printf("Connected to Vehicle API at %s:%d\n", server_ip, server_port);

  send(sockfd, "START PUSH\n", strlen("START PUSH\n"), 0);

  while (1) {
    int bytes = recv(sockfd, buffer, BUFFER_SIZE - 1, 0);
    if (bytes <= 0)
      break;
    buffer[bytes] = '\0';
    printf("GATEWAY LOG: %s", buffer);
  }

  close(sockfd);
  return 0;
}
