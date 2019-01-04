#ifndef __QUESTION_H__
#define __QUESTION_H__

#include <string>
#include <stdint.h>

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
  public:
  Question();
  void load_next_question();
  void send_to_client(int socket);
};

#endif