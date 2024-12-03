#include <iostream>
#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
#include <vector>
#include <memory>
#include <unordered_map>
#include <cstring>
#include <arpa/inet.h>

const int MAX_EVENTS = 1000;
const int BUFFER_SIZE = 4096;
const int MAX_CONNECTIONS = 10000;

struct Connection {
  int fd;
  char readBuffer[BUFFER_SIZE];
  char writeBuffer[BUFFER_SIZE];
  ssize_t writeOffset = 0;
  ssize_t readOffset = 0;
  bool writePending = false;

  Connection(int fd) : fd(fd) {}
};

// Utility function to set a socket to non-blocking mode
void setNonBlocking(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

class EchoServer {
  public:
  EchoServer(int port);
  void run();

  private:
  int server_fd;
  int epoll_fd;
  std::unordered_map<int, std::unique_ptr<Connection>> connections;

  void acceptConnection();
  void handleRead(Connection& conn);
  void handleWrite(Connection& conn);
  void closeConnection(int fd);
};

EchoServer::EchoServer(int port) {
  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
    perror("Socket creation failed");
  }
  int opt = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    perror("Set socket options failed");
  }
  setNonBlocking(server_fd);

  sockaddr_in server_addr{};
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(port);

  if (bind(server_fd, (struct sockaddr*) &server_addr, sizeof(server_addr)) < 0) {
    perror("Bind failed");
  }

  if (listen(server_fd, 3) < 0) {
    perror("Listen failed");
  }

  std::cout << "Server listening on port " << port << std::endl;

  // bind(server_fd, (sockaddr*)&server_addr, sizeof(server_addr));
  // listen(server_fd, SOMAXCONN);

  epoll_fd = epoll_create1(0);
  epoll_event ev{};
  ev.events = EPOLLIN;
  ev.data.fd = server_fd;
  epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev);
}

void EchoServer::run() {
  epoll_event events[MAX_EVENTS];

  while (true) {
    int n = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);

    for (int i = 0; i < n; ++i) {
      int fd = events[i].data.fd;

      if (fd == server_fd) {
        acceptConnection();
      } else {
        auto& conn = *connections[fd];

        if (events[i].events & EPOLLIN) {
          handleRead(conn);
        }
        if (events[i].events & EPOLLOUT) {
          handleWrite(conn);
        }
      }
    }
  }
}

void EchoServer::acceptConnection() {
  sockaddr_in client_addr;
  socklen_t client_len = sizeof(client_addr);
  int client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_len);

  if (client_fd >= 0) {
    setNonBlocking(client_fd);
    epoll_event ev{};
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = client_fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev);

    connections[client_fd] = std::make_unique<Connection>(client_fd);
  }
}

void EchoServer::handleRead(Connection& conn) {
  ssize_t bytesRead = read(conn.fd, conn.readBuffer, BUFFER_SIZE);
  if (bytesRead > 0) {
    // Store data in write buffer to echo back
    std::memcpy(conn.writeBuffer, conn.readBuffer, bytesRead);
    conn.writeOffset = bytesRead;
    conn.writePending = true;

    // Register socket for writing
    epoll_event ev{};
    ev.events = EPOLLIN | EPOLLOUT | EPOLLET;
    ev.data.fd = conn.fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_MOD, conn.fd, &ev);
  } else if (bytesRead == 0 || (bytesRead == -1 && errno != EAGAIN)) {
    closeConnection(conn.fd);
  }
}

void EchoServer::handleWrite(Connection& conn) {
  if (conn.writePending) {
    ssize_t bytesWritten = write(conn.fd, conn.writeBuffer, conn.writeOffset);
    if (bytesWritten > 0) {
      conn.writeOffset -= bytesWritten;

      // If all data written, unregister EPOLLOUT
      if (conn.writeOffset == 0) {
        conn.writePending = false;
        epoll_event ev{};
        ev.events = EPOLLIN | EPOLLET;
        ev.data.fd = conn.fd;
        epoll_ctl(epoll_fd, EPOLL_CTL_MOD, conn.fd, &ev);
      }
    } else if (bytesWritten == -1 && errno != EAGAIN) {
      closeConnection(conn.fd);
    }
  }
}

void EchoServer::closeConnection(int fd) {
  epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
  close(fd);
  connections.erase(fd);
}

int main() {
  EchoServer server(9001);
  server.run();
  return 0;
}
