#include <arpa/inet.h>
#include "client.h"
#include "question.h"

constexpr int BUF_SIZE = 255;

map<int, Client*> Client::client_list;

int Client::read_from_socket() {
  int bytes_read;
  if(!bytes_to_read) {
    bytes_read = recv(socket, (size_buf + (4 - size_bytes_to_read)), size_bytes_to_read, 0); 
    if (!bytes_read) return -1;

    size_bytes_to_read -= bytes_read;

    if (size_bytes_to_read > 0) return 0;
    else {
      bytes_to_read = htonl(*((uint32_t*)size_buf));
      size_bytes_to_read = 0;
    }
  } else {
    char buf[BUF_SIZE];
    int can_read_bytes = BUF_SIZE < bytes_to_read ? BUF_SIZE : bytes_to_read;

    bytes_read = recv(socket, buf, can_read_bytes, 0);
    if (!bytes_read) return -1;
    bytes_to_read -= bytes_to_read;
    message_buffer += string(buf, bytes_read);

    if(!bytes_to_read) {
      current_message.ParseFromString(message_buffer);

      message_buffer.clear();
      bytes_to_read = 0;
      size_bytes_to_read = 4;
      return handle_message();
    }
  }

  return 0;
}

int Client::handle_message() {
  if(current_message.has_set_player_name()) {
    nick_name = current_message.set_player_name().name();
    cout << nick_name << " connected" << endl;
    if(send_ranking() < 0) return -1;
    return Question::current_question.send_to_client(this);
  } else if(current_message.has_answer()) {
    cout << "Got answer from " << nick_name << endl;
    auto answer = current_message.answer();

    current_answer_timestamp = answer.sent_at();
    current_answer = answer.selected_answer();
    current_question_id = answer.question_id();
  } else {
    cout << "Unsupported message" << endl;
  }

  return 0;
}

int Client::send_ranking() {
  cout << "Sending ranking to " << nick_name << endl;
  Message ranking_message;
  message::Ranking *ranking = new message::Ranking;
  
  for(auto it: Client::client_list) {
    message::Ranking_Player *player = ranking->add_players();
    player->set_name(it.second->nick_name);
    player->set_points(it.second->points);
  }

  ranking_message.set_allocated_ranking(ranking);

  string message;
  ranking_message.SerializeToString(&message);

  uint32_t size = message.size();
  size = ntohl(size);
  char *message_size = (char*)&size;

  if(send(socket, message_size, 4, 0) < 0) return -1;
  if(send(socket, message.data(), message.size(), 0) < 0) return -1;

  return 0;
}