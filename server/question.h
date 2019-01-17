#ifndef __QUESTION_H__
#define __QUESTION_H__

#include <string>
#include <stdint.h>
#include <fstream>
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
  ifstream database;
  public:
  Question();
  void load_next_question();
  int send_to_client(Client *client);
  void run(int epoll_fd);
  void calculate_points();
};

#endif