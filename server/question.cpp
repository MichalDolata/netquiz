#include <chrono>
#include <thread>
#include <functional>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sstream>
#include <unistd.h>
#include <string>
#include "question.h"
#include "message.pb.h"
#include "client.h"

using message::Message;

uint64_t Question::last_id = 0;
Question *Question::current_question = NULL;

Question::Question(int epoll_fd) {
  database = ifstream{"questions.db"};

  load_next_question();
  std::thread([this, epoll_fd]() {
      while (true)
      {        
        std::chrono::time_point<std::chrono::system_clock> tp;
        tp = std::chrono::system_clock::from_time_t(deadline_at);
        tp += std::chrono::seconds(5);
        std::this_thread::sleep_until(tp);
        
        for(auto it: Client::client_list) {
          if(it.second->current_question_id == Question::last_id 
            && it.second->current_answer == Question::current_question->correct_answer) {
            // TODO: calculate points concidering timestamp of answer
            it.second->points += 10;
          }
        }
        load_next_question();
        for(auto it: Client::client_list) {
          if(send_to_client(it.first) == -1 || it.second->send_ranking() == -1) {
            int client_socket = it.first;
            Client* client = Client::client_list.at(client_socket);
            Client::client_list.erase(client_socket);
            delete client;

            epoll_event event_to_delete;
            event_to_delete.events = EPOLLIN;
            event_to_delete.data.fd = client_socket;
            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_socket, &event_to_delete);
            shutdown(client_socket, SHUT_RDWR);
            close(client_socket);
            std::terminate();
          }
        }
      }
  }).detach();
}

void Question::load_next_question() {
    id = ++Question::last_id;
    
    if(database.eof()) {
      database.seekg(0);
    }

    string line;
    getline(database, line);
    stringstream ss{line};
    getline(ss, question, '|');
    getline(ss, answers[0], '|');
    getline(ss, answers[1], '|');
    getline(ss, answers[2], '|');
    getline(ss, answers[3], '|');
    ss >> correct_answer;

    deadline_at = time(NULL) + 30;
}

int Question::send_to_client(int socket) {
  Message question_message;
  message::Question *question = new message::Question;
  
  question->set_id(id);
  question->set_question(this->question);

  for(auto it: answers) {
    question->add_answers(it);
  }
  question->set_deadline_at(deadline_at);

  question_message.set_allocated_question(question);

  string message;
  question_message.SerializeToString(&message);

  int32_t size = message.size();
  size = ntohl(size);
  char *message_size = (char*)&size;

  if(send(socket, message_size, 4, 0) < 0) return -1;
  if(send(socket, message.data(), message.size(), 0) < 0) return -1;

  return 0;
}

