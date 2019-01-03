#include "client.h"

constexpr int BUF_SIZE = 5;

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
    cout << nick_name << endl;
  } else {
    cout << "Unsupported message" << endl;
  }
}