#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>

int main() {
  int listen_socket = socket(AF_INET, SOCK_STREAM, 0);
  sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(1337);
  server_addr.sin_addr.s_addr = INADDR_ANY; 
  bind(listen_socket, (sockaddr *)&server_addr, sizeof(server_addr));
  listen(listen_socket, 1);

  int epoll_fd = epoll_create1(0);

  epoll_event listen_event;
  listen_event.events = EPOLLIN;
  listen_event.data.fd = listen_socket;

  epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_socket, &listen_event);

  epoll_event current_event;
  while(true) {
    epoll_wait(epoll_fd, &current_event, 1, -1);

    if (current_event.events & EPOLLIN && current_event.data.fd == listen_socket) {
      std::cout << "Trying to connect" << std::endl;
      accept(listen_socket, NULL, NULL);
    }
  }

  return 0;
}