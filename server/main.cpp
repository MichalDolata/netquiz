#include <iostream>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <map>
#include <netinet/in.h>
#include "message.pb.h"
#include "client.h"

using namespace std;
using namespace message;

int main() {
  GOOGLE_PROTOBUF_VERIFY_VERSION;

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
      int client_socket = accept(listen_socket, NULL, NULL);

      listen_event.events = EPOLLIN;
      listen_event.data.fd = client_socket;
      epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_socket, &listen_event);

      Client::client_list.insert(pair<int, Client*>(client_socket, new Client(client_socket)));
    } else if (current_event.events & EPOLLIN) {
      int client_socket = current_event.data.fd;
      if(Client::client_list.at(client_socket)->read_from_socket() == -1) {
        Client::client_list.erase(client_socket);

        listen_event.events = EPOLLIN;
        listen_event.data.fd = client_socket;
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_socket, &listen_event);
        shutdown(client_socket, SHUT_RDWR);
      }
    }
  }

  return 0;
}