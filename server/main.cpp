#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <map>
#include <netinet/in.h>
#include <fcntl.h>
#include "message.pb.h"
#include "client.h"
#include "question.h"


using namespace std;
void set_nonblock(int socket) {
    int flags;
    flags = fcntl(socket,F_GETFL,0);
    if(flags == -1)
        return;
    int res = fcntl(socket, F_SETFL, O_NONBLOCK, 1);
    cout << "set nonblock" << res << "\n";
    
}

void bind_address(int listen_socket, int port) {
  sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port);
  server_addr.sin_addr.s_addr = INADDR_ANY;

  if(bind(listen_socket, (sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
    cerr << "Couldn't bind the address" << endl;
    exit(-1);
  }
}

void close_client_socket(int client_socket, int epoll_fd) {
  if(Client::client_list.find(client_socket) != Client::client_list.end()){
  Client* client = Client::client_list.at(client_socket);
  Client::client_list.erase(client_socket);
  delete client;
  }
  epoll_event event_to_delete;
  event_to_delete.events = EPOLLIN;
  event_to_delete.data.fd = client_socket;
  epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client_socket, &event_to_delete);
  epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_socket, &event_to_delete);
  shutdown(client_socket, SHUT_RDWR);
  close(client_socket);
}

void epoll_loop(int epoll_fd, int listen_socket) {
  epoll_event listen_event;
  listen_event.events = EPOLLIN;
  listen_event.data.fd = listen_socket;

  epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_socket, &listen_event);



  epoll_event current_event;

  while(true) {
    epoll_wait(epoll_fd, &current_event, 1, -1);

    if (current_event.events & EPOLLIN && current_event.data.fd == listen_socket) {
      int client_socket = accept(listen_socket, NULL, NULL);
    
      if(client_socket == -1) {
        continue;
      }


      listen_event.events = EPOLLIN;
      listen_event.data.fd = client_socket;
      epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_socket, &listen_event);
      set_nonblock(client_socket);
    
      Client::client_list.insert(pair<int, Client*>(client_socket, new Client(client_socket, epoll_fd)));
    } else if (current_event.events & EPOLLIN) {
      int client_socket = current_event.data.fd;
        if(Client::client_list.find(client_socket) == Client::client_list.end())continue;
      if(Client::client_list.at(client_socket)->read_from_socket() == -1) {
        close_client_socket(client_socket, epoll_fd);
      }

    } else if (current_event.events & EPOLLERR || current_event.events & EPOLLHUP) {
        int client_socket = current_event.data.fd;
        close_client_socket(client_socket, epoll_fd);
    }
    if (current_event.events & EPOLLOUT){
        
      int client_socket = current_event.data.fd;
      if(Client::client_list.find(client_socket) == Client::client_list.end())continue;
      auto client = Client::client_list.at(client_socket);
      int res = client->send_message();
      if(res == -1){
        close_client_socket(client_socket, epoll_fd);
      }else if(res == 1){
          
        epoll_event event_to_mod;
        event_to_mod.events = EPOLLIN;
        event_to_mod.data.fd = client_socket;
        epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client_socket, &event_to_mod);
      }

    }
  }
}

int load_port_from_config() {
  ifstream file{"settings.cfg"};

  int port;
  string line;
  while(getline(file, line)) {
    stringstream ss{line};
    string setting;
    getline(ss, setting, '=');

    if(setting == "PORT") {
      ss >> port;
      return port;
    }
  }

  return -1;
}

int main() {
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  int port = load_port_from_config();
  if(port == -1) {
    cerr << "Couldn't get port from config" << endl;
    exit(-1);
  }
  int listen_socket = socket(AF_INET, SOCK_STREAM, 0);
  int flag = 1;
  setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(int));
   bind_address(listen_socket, port);
  listen(listen_socket, 1);

  int epoll_fd = epoll_create1(0);

  Question::current_question.run(epoll_fd);

  epoll_loop(epoll_fd, listen_socket);

  return 0;
}
