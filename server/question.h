#ifndef __QUESTION_H__
#define __QUESTION_H__

#include <string>
#include <stdint.h>
#include <fstream>
#include <vector>
#include <mutex>
#include <google/protobuf/repeated_field.h>
#include "client.h"

using namespace std;

class Question {
  public:
  static uint64_t last_id;
  static Question current_question;
  private:
  uint64_t id;
  string question;
  string answers[4];
  ushort correct_answer;
  uint64_t deadline_at;
  mutex database_mutex;
  vector<string> readed_questions;
  vector<string>::iterator readed_questions_iterator;
  public:
  Question();
  void load_next_question();
  int send_to_client(Client *client);
  void run(int epoll_fd);
  void calculate_points();
  void save_question(string question, const google::protobuf::RepeatedPtrField<string>, uint32_t correct_answer);
};

#endif