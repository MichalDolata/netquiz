#include <chrono>
#include <thread>
#include <functional>
#include <sys/socket.h>
#include "question.h"
#include "message.pb.h"
#include "client.h"

using message::Message;

uint64_t Question::last_id = 0;
Question Question::current_question;

Question::Question() {
  load_next_question();

  std::thread([this]() {
      while (true)
      {        
        std::chrono::time_point<std::chrono::system_clock> tp;
        tp = std::chrono::system_clock::from_time_t(deadline_at);
        tp += std::chrono::seconds(5);
        std::this_thread::sleep_until(tp);
        
        for(auto it: Client::client_list) {
          if(it.second->current_question_id == Question::last_id 
            && it.second->current_answer == Question::current_question.correct_answer) {
            // TODO: calculate points concidering timestamp of answer
            it.second->points += 10;
          }
        }
        load_next_question();
        for(auto it: Client::client_list) {
          send_to_client(it.first);
          it.second->send_ranking();
        }
      }
  }).detach();
}

void Question::load_next_question() {
  if (Question::last_id % 2 == 0) {
    id = ++Question::last_id;
    question = "First question";
    answers[0] = "A";
    answers[1] = "B";
    answers[2] = "C";
    answers[3] = "D";
    correct_answer = 0;
  } else {
    id = ++Question::last_id;
    question = "Second question";
    answers[0] = "1";
    answers[1] = "2";
    answers[2] = "3";
    answers[3] = "4";
    correct_answer = 3;
  }
  deadline_at = time(NULL) + 30;
}

void Question::send_to_client(int socket) {
  Message question_message;
  message::Question *question = new message::Question;
  
  question->set_id(id);
  question->set_question(this->question);

  for(auto it: answers) {
    question->add_answers(it);
  }
  question->set_deadline_at(deadline_at);

  question_message.set_allocated_question(question);

  string *message = new string;
  question_message.SerializeToString(message);

  int32_t size = message->size();
  char message_size[4] = { (char)(size >> 24), (char)(size >> 16), (char)(size >> 8), (char)(size) };

  send(socket, message_size, 4, 0);
  send(socket, message->data(), message->size(), 0);
}

