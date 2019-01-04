#include "client.h"

constexpr int BUF_SIZE = 255;

map<int, Client*> Client::client_list;

void Client::read_from_socket() {
  if(!bytes_to_read) {
    size_bytes_to_read -= recv(socket, (size_buf + (4 - size_bytes_to_read)), size_bytes_to_read, 0); 

    if (size_bytes_to_read > 0) return;
    else {
      bytes_to_read = (size_buf[0] << 24) | (size_buf[1] << 16) | (size_buf[2] << 8) | (size_buf[3]);
      size_bytes_to_read = 0;
    }
  }

  char buf[BUF_SIZE];
  int can_read_bytes = BUF_SIZE < bytes_to_read ? BUF_SIZE : bytes_to_read;

  bytes_to_read -= recv(socket, buf, can_read_bytes, 0);
  message_buffer += buf;

  if(!bytes_to_read) {
    current_message.ParseFromString(message_buffer);

    message_buffer.clear();
    bytes_to_read = size_bytes_to_read = 0;
    handle_message();
  }
}

void Client::handle_message() {
  if(current_message.has_set_player_name()) {
    nick_name = current_message.set_player_name().name();
    send_ranking();
  } else {
    cout << "Unsupported message" << endl;
  }
}

void Client::send_ranking() {
  Message ranking_message;
  Ranking *ranking = new Ranking;
  
  for(auto it: client_list) {
    Ranking_Player *player = ranking->add_players();
    player->set_name(it.second->nick_name);
    player->set_points(it.second->points);
  }

  ranking_message.set_allocated_ranking(ranking);

  string *message = new string;
  ranking_message.SerializeToString(message);

  int32_t size = message->size();
  char message_size[4] = { (char)(size >> 24), (char)(size >> 16), (char)(size >> 8), (char)(size) };

  send(socket, message_size, 4, 0);
  send(socket, message->data(), message->size(), 0);
}