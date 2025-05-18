#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUFFER_SIZE 1024
#define PORT 8080

int main() {
  int server_fd, client_fd;
  struct sockaddr_in server_addr, client_addr;
  socklen_t addr_len = sizeof(client_addr);
  char buffer[BUFFER_SIZE] = {0};

  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
    perror("Creation socker failed");
    exit(EXIT_FAILURE);
  }

  printf("Socket created successfully\n");

  /*int opt = 1;*/
  /*if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
   * {*/
  /*  perror("setsockopt failed");*/
  /*  exit(EXIT_FAILURE);*/
  /*}*/

  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(PORT);

  if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) <
      0) {
    perror("Bind failed");
    exit(EXIT_FAILURE);
  }
  printf("Socket bound to port %d\n", PORT);

  if (listen(server_fd, 3) < 0) {
    perror("Listen failed");
    exit(EXIT_FAILURE);
  }
  printf("Listening on port %d\n", PORT);

  printf("Waiting for connections...\n");
  client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
  if (client_fd < 0) {
    perror("Accept failed");
    exit(EXIT_FAILURE);
  }

  printf("Connection accepted from %s:%d\n", inet_ntoa(client_addr.sin_addr),
         ntohs(client_addr.sin_port));

  size_t bytes_read = read(client_fd, buffer, BUFFER_SIZE - 1);
  if (bytes_read < 0) {
    perror("Read failed");
    exit(EXIT_FAILURE);
  } else {
    buffer[bytes_read] = '\0'; // Null-terminate the string
    printf("Received message from client: %s\n", buffer);

    const char *response = "Hello from server!";
    write(client_fd, response, strlen(response));
    printf("Sent response to client: %s\n", response);
  }

  close(client_fd);
  close(server_fd);
  printf("Connection closed\n");

  return 0;
}
