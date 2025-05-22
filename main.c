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

void handle_http_request(int client_fd, const HttpRequest *request) {
  char response[BUFFER_SIZE];
  const char *html_content;
  int status_code = 200; // Default: OK

  printf("Handling request for path: %s\n", request->path);

  // Basic routing based on the path
  if (strcmp(request->path, "/") == 0 ||
      strcmp(request->path, "/index.html") == 0) {
    html_content = "<!DOCTYPE html>\n"
                   "<html>\n"
                   "<head>\n"
                   "    <title>My HTTP Server</title>\n"
                   "    <style>\n"
                   "        body { font-family: Arial, sans-serif; margin: "
                   "40px; line-height: 1.6; }\n"
                   "        h1 { color: #333; }\n"
                   "    </style>\n"
                   "</head>\n"
                   "<body>\n"
                   "    <h1>Welcome to My HTTP Server</h1>\n"
                   "    <p>This is the home page served by our custom HTTP "
                   "server written in C.</p>\n"
                   "    <ul>\n"
                   "        <li><a href=\"/about\">About</a></li>\n"
                   "        <li><a href=\"/contact\">Contact</a></li>\n"
                   "    </ul>\n"
                   "</body>\n"
                   "</html>";
  } else if (strcmp(request->path, "/about") == 0) {
    html_content =
        "<!DOCTYPE html>\n"
        "<html>\n"
        "<head>\n"
        "    <title>About - My HTTP Server</title>\n"
        "    <style>\n"
        "        body { font-family: Arial, sans-serif; margin: 40px; "
        "line-height: 1.6; }\n"
        "        h1 { color: #333; }\n"
        "    </style>\n"
        "</head>\n"
        "<body>\n"
        "    <h1>About This Server</h1>\n"
        "    <p>This is a simple HTTP server built from scratch in C.</p>\n"
        "    <p>It demonstrates the basics of the HTTP protocol:</p>\n"
        "    <ul>\n"
        "        <li>Socket programming</li>\n"
        "        <li>HTTP request parsing</li>\n"
        "        <li>Response generation</li>\n"
        "        <li>Basic routing</li>\n"
        "    </ul>\n"
        "    <p><a href=\"/\">Back to Home</a></p>\n"
        "</body>\n"
        "</html>";
  } else if (strcmp(request->path, "/contact") == 0) {
    html_content = "<!DOCTYPE html>\n"
                   "<html>\n"
                   "<head>\n"
                   "    <title>Contact - My HTTP Server</title>\n"
                   "    <style>\n"
                   "        body { font-family: Arial, sans-serif; margin: "
                   "40px; line-height: 1.6; }\n"
                   "        h1 { color: #333; }\n"
                   "    </style>\n"
                   "</head>\n"
                   "<body>\n"
                   "    <h1>Contact</h1>\n"
                   "    <p>This is a demo contact page.</p>\n"
                   "    <p><a href=\"/\">Back to Home</a></p>\n"
                   "</body>\n"
                   "</html>";
  } else {
    // 404 Not Found for any other path
    status_code = 404;
    html_content =
        "<!DOCTYPE html>\n"
        "<html>\n"
        "<head>\n"
        "    <title>404 Not Found</title>\n"
        "    <style>\n"
        "        body { font-family: Arial, sans-serif; margin: 40px; "
        "line-height: 1.6; }\n"
        "        h1 { color: #CC0000; }\n"
        "    </style>\n"
        "</head>\n"
        "<body>\n"
        "    <h1>404 Not Found</h1>\n"
        "    <p>The requested resource was not found on this server.</p>\n"
        "    <p><a href=\"/\">Back to Home</a></p>\n"
        "</body>\n"
        "</html>";
  }

  // Create the HTTP response with appropriate status code
  const char *status_text = (status_code == 200) ? "OK" : "Not Found";

  // Format the HTTP response
  int response_len =
      snprintf(response, BUFFER_SIZE,
               "HTTP/1.1 %d %s\r\n"
               "Content-Type: text/html\r\n"
               "Content-Length: %ld\r\n"
               "Connection: close\r\n"
               "\r\n"
               "%s",
               status_code, status_text, strlen(html_content), html_content);

  // Send the response
  write(client_fd, response, response_len);
  printf("Response sent with status code %d\n", status_code);
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
