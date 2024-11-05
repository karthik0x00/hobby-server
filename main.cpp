#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <arpa/inet.h>

int main() {
  int sock = socket(AF_INET, SOCK_STREAM, 0);
  int new_socket;
  if (sock < 0) {
    std::cerr << "Socket Creation Failed...";
    return -1;
  }
  const int PORT = 9001;
  int opt = 1;
  struct sockaddr_in serv_addr;
  int addrSize = sizeof(serv_addr);
  if (setsockopt(sock, SOL_SOCKET, SO_DEBUG, &opt, sizeof(opt))) {
    std::cerr << "Something went wrong" << std::endl;
    return -1;
  }
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(PORT);
  if (bind(sock, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) > 0) {
    std::cerr << "Bind failed";
    return -1;
  }

  if (listen(sock, 3) < 0) {
    std::cerr << "Listen failed" << std::endl;
    return -1;
  }

  std::cout << "Serving on " << inet_ntoa(serv_addr.sin_addr) << ':' << ntohs(serv_addr.sin_port) << std::endl;
  if ((new_socket = accept(sock, (struct sockaddr*) &serv_addr, (socklen_t*) &addrSize)) < 0) {
    std:: cerr << "Accept failed" << std::endl;
  }

  struct sockaddr_in remoteAddr;
  int peerNameStatus = getpeername(new_socket, (struct sockaddr*) &remoteAddr, (socklen_t*) (&addrSize));
  if (peerNameStatus == 0) {
    std::cout << "Incoming connection from " << inet_ntoa(remoteAddr.sin_addr) << ':' << ntohs(remoteAddr.sin_port) << std::endl;
  }
  while (true) {
    char buffer[1024] = {0};
    int valread = read(new_socket, buffer, 1024);
    printf("%s", buffer);
    write(new_socket, buffer, sizeof(buffer));
  }
  return 0;
}
