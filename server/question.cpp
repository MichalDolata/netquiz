#include <chrono>
#include <thread>
#include <functional>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sstream>
#include <algorithm>
#include <limits>
#include <unistd.h>
#include <string>
#include "question.h"
#include "message.pb.h"

using message::Message;

uint64_t Question::last_id = 0;
Question Question::current_question;

Question::Question() {
  readed_questions_iterator = readed_questions.begin();
}

void Question::run(int epoll_fd) {
  load_next_question();
  std::thread([this, epoll_fd]() {
      while (true)
      {        
        std::chrono::time_point<std::chrono::system_clock> tp;
        tp = std::chrono::system_clock::from_time_t(deadline_at);
        tp += std::chrono::seconds(5);
        std::this_thread::sleep_until(tp);
        
        calculate_points();
        load_next_question();

        for(auto it: Client::client_list) {
            cout << "Send ranking\n";
          if(send_to_client(it.second) == -1 || it.second->send_ranking() == -1) {
            cout << "Error\n";
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

void Question::calculate_points() {
  uint64_t best_time = 0xFFFFFFFFFFFFFFFF;

  for(auto it: Client::client_list) {
    auto player = it.second;
    if(player->current_question_id == Question::last_id 
      && player->current_answer == Question::current_question.correct_answer) {
      best_time = min(best_time, player->connected_at + player->current_answer_timestamp);
    }
  }

  for(auto it: Client::client_list) {
    auto player = it.second;
    if(player->current_question_id == Question::last_id 
      && player->current_answer == Question::current_question.correct_answer) {
      player->points += 10;
      if(player->connected_at + player->current_answer_timestamp == best_time) {
        player->points += 5;
      }
    }
  }
}

void Question::load_next_question() {
    id = ++Question::last_id;

    if(readed_questions_iterator != readed_questions.end()) {
        string line = *readed_questions_iterator;
        stringstream ss{line};
        getline(ss, question, '|');
        getline(ss, answers[0], '|');
        getline(ss, answers[1], '|');
        getline(ss, answers[2], '|');
        getline(ss, answers[3], '|');
        ss >> correct_answer;

        deadline_at = time(NULL) + 15;

        readed_questions_iterator++;
    } else {
      readed_questions.clear();

      database_mutex.lock();     
      ifstream database{"questions.db"};

      string line;
      while(!database.eof()) {
        getline(database, line);
        readed_questions.push_back(line);
      }

      random_shuffle(readed_questions.begin(), readed_questions.end());
      readed_questions_iterator = readed_questions.begin();
      database.close();
      database_mutex.unlock();

      load_next_question();
    }
}

void Question::save_question(string question, const google::protobuf::RepeatedPtrField<string> answers, uint32_t correct_answer) {
  database_mutex.lock();

  ofstream database{"questions.db", ios_base::app};
  database << endl << question << "|";
  for(auto answer: answers) {
    database << answer << "|";
  }
  database << correct_answer;
  database.close();

  database_mutex.unlock();
}

int Question::send_to_client(Client *client) {

  Message question_message;
  message::Question *question = new message::Question;
  
  question->set_id(id);
  question->set_question(this->question);

  for(auto it: answers) {
    question->add_answers(it);
  }

  question->set_deadline_at(deadline_at - client->connected_at);

  question_message.set_allocated_question(question);

  string message;
  question_message.SerializeToString(&message);

  int32_t size = message.size();
  size = ntohl(size);
  char *message_size = (char*)&size;

  client->add_message(string(message_size,4));
  client->add_message(string(message));

  return 0;
}

