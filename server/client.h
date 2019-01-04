#include <stdint.h>
#include <string>
#include <sys/socket.h>
#include "message.pb.h"

using namespace std;
using namespace message;

class Client {
  public:
  static map<int, Client*> client_list;

  private:
  int socket;
  char size_buf[4];
  int size_bytes_to_read;
  int32_t bytes_to_read;
  string message_buffer;
  Message current_message;
  string nick_name;
  float points;

  public:
  Client(int socket) : socket{socket}, size_bytes_to_read{4}, bytes_to_read{0}, points{0} {};
  int read_from_socket();
  void handle_message();
  void send_ranking();
};