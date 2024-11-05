#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <thread>
#include <vector>

class Server {
  public:
  Server(const std::string& host, int port) : host(host), port(port), server_socket(-1) {}

  ~Server() {
    closeSocket();
  }

  bool listen(int backlog = 3) {
    // Create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
      std::cerr << "Socket creation failed: " << strerror(errno) << std::endl;
      return false;
    }

    // Set socket options
    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
      std::cerr << "Failed to set socket options: " << strerror(errno) << std::endl;
      close(server_socket);
      return false;
    }

    // Configure server address
    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(host.c_str());

    // Bind the socket
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
      std::cerr << "Bind failed: " << strerror(errno) << std::endl;
      close(server_socket);
      return false;
    }

    // Start listening
    if (::listen(server_socket, backlog) < 0) {
      std::cerr << "Listen failed: " << strerror(errno) << std::endl;
      close(server_socket);
      return false;
    }

    std::cout << "Serving on " << host << ':' << port << std::endl;
    return true;
  }

  void acceptConnections() {
    while (true) {
      sockaddr_in client_addr;
      socklen_t addr_len = sizeof(client_addr);

      // Accept a new client connection
      int client_socket = accept(server_socket, (struct sockaddr*) &client_addr, &addr_len);
      if (client_socket < 0) {
        std::cerr << "Accept failed: " << strerror(errno) << std::endl;
        continue;
      }

      // std::cout << "Incoming connection from " << inet_ntoa(client_addr.sin_addr) << ':' << ntohs(client_addr.sin_port) << std::endl;

      // Launch a new thread for each client connection
      std::thread(&Server::handleClient, this, client_socket, client_addr).detach();
    }
  }

  private:
  std::string host;
  int port;
  int server_socket;

  void handleClient(int client_socket, sockaddr_in client_addr) {
    std::vector<char> buffer(1024);
    ssize_t bytes_read;
    while ((bytes_read = read(client_socket, buffer.data(), buffer.size())) > 0) {
      if (bytes_read == buffer.size()) {
        buffer.resize(buffer.size() * 2);  // Double the buffer size
      } else {
        // write(STDOUT_FILENO, buffer.data(), bytes_read);
        write(client_socket, buffer.data(), bytes_read);
      }
    }
    close(client_socket);
    // std::cout << "Connection terminated with " << inet_ntoa(client_addr.sin_addr) << ':' << ntohs(client_addr.sin_port) << std::endl;
  }

  void closeSocket() {
    if (server_socket >= 0) {
      close(server_socket);
    }
  }
};

int main() {
  Server server("0.0.0.0", 9001);

  if (!server.listen()) {
    return EXIT_FAILURE;
  }

  // Accept connections in the main thread
  server.acceptConnections();
  return EXIT_SUCCESS;
}
