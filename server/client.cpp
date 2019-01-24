#include <arpa/inet.h>
#include <google/protobuf/repeated_field.h>
#include "client.h"
#include "question.h"

constexpr int BUF_SIZE = 255;

map<int, Client *> Client::client_list;

int Client::read_from_socket() {
    int bytes_read;
    if (!bytes_to_read) {
        bytes_read = recv(socket, (size_buf + (4 - size_bytes_to_read)), size_bytes_to_read, 0);
        if (!bytes_read) return -1;

        size_bytes_to_read -= bytes_read;

        if (size_bytes_to_read > 0) return 0;
        else {
            bytes_to_read = htonl(*((uint32_t *) size_buf));
            size_bytes_to_read = 0;
        }
    } else {
        char buf[BUF_SIZE];
        int can_read_bytes = BUF_SIZE < bytes_to_read ? BUF_SIZE : bytes_to_read;

        bytes_read = recv(socket, buf, can_read_bytes, 0);
        if (!bytes_read) return -1;
        bytes_to_read -= bytes_to_read;
        message_buffer += string(buf, bytes_read);

        if (!bytes_to_read) {
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
    if (current_message.has_set_player_name()) {
        nick_name = current_message.set_player_name().name();
        cout << nick_name << " connected" << endl;
        if (send_ranking() < 0) return -1;

        return Question::current_question.send_to_client(this);
    } else if (current_message.has_answer()) {
        cout << "Got answer from " << nick_name << endl;
        auto answer = current_message.answer();

        current_answer_timestamp = answer.sent_at();
        current_answer = answer.selected_answer();
        current_question_id = answer.question_id();
    } else if (current_message.has_add_question()) {
        string question = current_message.add_question().question();
        const google::protobuf::RepeatedPtrField <string> answers = current_message.add_question().answers();
        uint32_t correct_answer = current_message.add_question().correct_answer();

        Question::current_question.save_question(question, answers, correct_answer);
    } else {
        cout << "Unsupported message" << endl;
    }

    return 0;
}

int Client::send_ranking() {
    cout << "Sending ranking to " << nick_name << endl;
    Message ranking_message;
    message::Ranking *ranking = new message::Ranking;

    for (auto it: Client::client_list) {
        message::Ranking_Player *player = ranking->add_players();
        player->set_name(it.second->nick_name);
        player->set_points(it.second->points);
    }

    ranking_message.set_allocated_ranking(ranking);

    string message;
    ranking_message.SerializeToString(&message);

    uint32_t size = message.size();
    size = ntohl(size);
    char *message_size = (char *) &size;

    add_message(string(message_size, 4));
    add_message(message);
    return 0;
}

void Client::add_message(string message) {

    message_queue.push(make_pair(string(message), 0));
    if (message_queue.size() == 1) {

        if (true) {
            epoll_event write_event;
            write_event.events = EPOLLIN | EPOLLOUT;
            write_event.data.fd = socket;
            epoll_ctl(epoll_fd, EPOLL_CTL_MOD, socket, &write_event);
        }

    }
}

int Client::send_message() {
    cout << "Send to client\n";
    auto message = message_queue.front().first;
    size_t bytes_already_send = message_queue.front().second;
    size_t bytes_to_send = message.size() - bytes_already_send;
    auto res = send(socket, message.data() + bytes_already_send, bytes_to_send, 0);
    if (res == -1)return -1;
    if (bytes_to_send - res == 0) {
        message_queue.pop();
        return message_queue.empty();
    }
    message_queue.front().second += res;
    return 0;

}