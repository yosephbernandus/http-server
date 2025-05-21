#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUFFER_SIZE 1024
#define PORT 8080

// Maximum size of the request line eg. "GET / HTTP/1.1"
#define REQUEST_LINE 256

typedef struct {
  char method[8];
  char path[256];
  char version[16];
  char host[256];
  char user_agent[512];
} HttpRequest;

void parse_http_request(const char *request_buffer, HttpRequest *request) {
  memset(request, 0, sizeof(HttpRequest));

  char request_line[REQUEST_LINE];
  sscanf(request_buffer, "%255[^\r\n]", request_line);

  // Parse the request line to get method, path, and version
  sscanf(request_line, "%15s %255s %15s", request->method, request->path,
         request->version);

  const char *host_ptr = strstr(request_buffer, "Host: ");
  if (host_ptr) {
    sscanf(host_ptr, "Host: %255[^\r\n]", request->host);
  }

  const char *ua_ptr = strstr(request_buffer, "User-Agent: ");
  if (ua_ptr) {
    sscanf(ua_ptr, "User-Agent: %511[^\r\n]", request->user_agent);
  }
}

void print_http_request(const HttpRequest *request) {
  printf("\n----- HTTP Request Details -----\n");
  printf("Method:     %s\n", request->method);
  printf("Path:       %s\n", request->path);
  printf("Version:    %s\n", request->version);
  printf("Host:       %s\n", request->host);
  printf("User-Agent: %.100s...\n",
         request->user_agent); // Only trim to 100 first chars for display
  printf("-------------------------------\n\n");
}

int main() {
  int server_fd, client_fd;
  struct sockaddr_in server_addr, client_addr;
  socklen_t addr_len = sizeof(client_addr);
  char buffer[BUFFER_SIZE] = {0};
  HttpRequest request;

  // Create socket file descriptor
  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
    perror("Creation socker failed");
    exit(EXIT_FAILURE);
  }

  printf("Socket created successfully\n");

  // Set socket options (Optional but recommended)
  // This allows reusing the address without waiting for TIME_WAIT to expire
  int opt = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    perror("setsockopt failed");
    exit(EXIT_FAILURE);
  }

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

  // Read HTTP request from client
  ssize_t bytes_read = read(client_fd, buffer, BUFFER_SIZE - 1);
  if (bytes_read < 0) {
    perror("Read failed");
  } else {
    // Ensure null termination
    buffer[bytes_read] = '\0';

    // Print raw request
    printf("Raw HTTP Request:\n%s\n", buffer);

    // Parse the HTTP request
    parse_http_request(buffer, &request);

    // Print structured request information
    print_http_request(&request);

    // Send a simple response
    const char *response = "HTTP/1.1 200 OK\r\n"
                           "Content-Type: text/plain\r\n"
                           "Content-Length: 13\r\n"
                           "\r\n"
                           "Hello, HTTP!";

    write(client_fd, response, strlen(response));
    printf("HTTP response sent\n");
  }

  close(client_fd);
  close(server_fd);
  printf("Connection closed\n");

  return 0;
}
