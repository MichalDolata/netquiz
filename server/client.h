#ifndef __CLIENT_H__
#define __CLIENT_H__

#include <stdint.h>
#include <string>
#include <sys/socket.h>
#include <queue>
#include "message.pb.h"
#include <sys/epoll.h>

using namespace std;
using message::Message;

class Client {
  public:
  static map<int, Client*> client_list;
  queue<pair<string,int>> message_queue;
  private:
  char size_buf[4];
  int size_bytes_to_read;
  uint32_t bytes_to_read;
  string message_buffer;
  Message current_message;
  string nick_name;

  public:
  Client(int socket, int epoll_fd) :
    size_bytes_to_read{4}, bytes_to_read{0}, socket{socket}, epoll_fd{epoll_fd}, current_answer{5},
    current_answer_timestamp{0}, current_question_id{0}, points{0}, connected_at{(uint64_t) time(NULL)} {};
  int read_from_socket();
  int handle_message();
  int send_ranking();
  int send_message();
  void add_message(string message);
  int socket;
  int epoll_fd;

  volatile uint32_t current_answer;
  volatile uint64_t current_answer_timestamp;
  volatile uint64_t current_question_id;
  float points;
  uint64_t connected_at;
};

#endif