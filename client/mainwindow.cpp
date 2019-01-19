#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <string>
#include <fstream>
#include <sstream>

constexpr int MENU_WIDGET = 0;
constexpr int GAME_WIDGET = 1;
constexpr int BUF_SIZE = 255;

using namespace message;

pair<string, quint16> load_settings_from_config() {
  ifstream file{"settings.cfg"};

  quint16 port;
  string host;

  string line;
  while(getline(file, line)) {
    stringstream ss{line};
    string setting;
    getline(ss, setting, '=');

    if(setting == "PORT") {
      ss >> port;
    } else if(setting == "HOST") {
      ss >> host;
    }

  }
  return make_pair(host, port);
}

QByteArray intToArray(qint32 source)
{
    QByteArray temp;
    QDataStream data(&temp, QIODevice::ReadWrite);
    data << source;
    return temp;
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    size_bytes_to_read{4}, bytes_to_read{0},
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    timer = new QTimer();
    timer->setInterval(1000);

    connect(timer, &QTimer::timeout, this, &MainWindow::handleTick);
    connect(ui->connectButton, &QPushButton::clicked, this, &MainWindow::handleConnect);
    connect(ui->answer_0, &QPushButton::clicked, this, [this]() { handle_answer(0); });
    connect(ui->answer_1, &QPushButton::clicked, this, [this]() { handle_answer(1); });
    connect(ui->answer_2, &QPushButton::clicked, this, [this]() { handle_answer(2); });
    connect(ui->answer_3, &QPushButton::clicked, this, [this]() { handle_answer(3); });

    enable_answering(false);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::handleConnect() {
    ui->connectButton->setEnabled(false);

    auto settings = load_settings_from_config();
    if(settings.first == "" || settings.second == 0) {
        ui->statusBar->showMessage("Couldn't get setting from config", 5000);
        ui->connectButton->setEnabled(true);
        return;
    }

    socket = new QTcpSocket(this);
    socket->connectToHost(settings.first.c_str(), settings.second);

    if(socket->waitForConnected()) {
        connected_at = time(NULL);
        ui->stackedWidget->setCurrentIndex(GAME_WIDGET);

        Message setPlayerNameMessage;
        SetPlayerName *content = new SetPlayerName;
        content->set_name(ui->nickNameEdit->text().toUtf8().constData());
        setPlayerNameMessage.set_allocated_set_player_name(content);

        send_message(setPlayerNameMessage);

        connect(socket, &QTcpSocket::readyRead, this, &MainWindow::handleRead);
        connect(socket, &QTcpSocket::disconnected, this, &MainWindow::handleDisconnect);
        ui->statusBar->clearMessage();
    } else {
        ui->statusBar->showMessage("Can't connect to the server", 5000);
    }
    ui->connectButton->setEnabled(true);
}

// MOVE TO DIFFRENT CLASS
void MainWindow::handleRead() {
    if(!bytes_to_read) {
      size_bytes_to_read -= socket->read((size_buf + (4 - size_bytes_to_read)), size_bytes_to_read);

      if (size_bytes_to_read > 0) return;
      else {
        bytes_to_read = (size_buf[0] << 24) | (size_buf[1] << 16) | (size_buf[2] << 8) | (size_buf[3]);
      }
    }

    char buf[BUF_SIZE];
    int can_read_bytes = BUF_SIZE < bytes_to_read ? BUF_SIZE : bytes_to_read;

    qint64 bytes_read;
    bytes_read = socket->read(buf, can_read_bytes);
    bytes_to_read -= bytes_read;
    message_buffer += string(buf, bytes_read);

    if(!bytes_to_read) {
      current_message.ParseFromString(message_buffer);
      message_buffer.clear();
      bytes_to_read = 0;
      size_bytes_to_read = 4;
      handle_message();
    }

    if(socket->bytesAvailable() > 0) {
        handleRead();
    }
}

void MainWindow::handle_message() {
    if(current_message.has_ranking()) {
        ui->playerList->clear();
        auto ranking  = vector<pair<string,int>>();
        for(auto it: current_message.ranking().players())
            ranking.push_back(make_pair(string(it.name().data()), it.points()));
        sort(ranking.begin(), ranking.end(),
                                    [](const pair<string,int> &a, const pair<string,int> &b)
                                    {
                                        return a.second > b.second;
                                    });
        for(auto it: ranking) {

            QString label = QString("%1 %2").arg(it.first.c_str()).arg(it.second);
            ui->playerList->addItem(label);
        }
    } else if(current_message.has_question()) {
      auto question = current_message.question();
      question_id = question.id();
      ui->question->setText(question.question().data());
      ui->answer_0->setText(question.answers(0).data());
      ui->answer_1->setText(question.answers(1).data());
      ui->answer_2->setText(question.answers(2).data());
      ui->answer_3->setText(question.answers(3).data());
      deadline_at = connected_at + question.deadline_at();
      handleTick();
      timer->start();
      enable_answering(true);
    } else {
      cout << "Unsupported message" << endl;
    }
}

void MainWindow::handleTick() {
    qint64 timeLeft = (qint64)deadline_at - time(nullptr);
    if (timeLeft >= 0) ui->timerLabel->setText(QString("%1 seconds left").arg(timeLeft));
    else {
        ui->timerLabel->setText("Waiting for a next question");
        enable_answering(false);
        timer->stop();
    }
}

void MainWindow::handleDisconnect() {
    ui->stackedWidget->setCurrentIndex(MENU_WIDGET);
    ui->statusBar->showMessage("Disconnected", 5000);
}

void MainWindow::enable_answering(bool enabled) {
    ui->answer_0->setEnabled(enabled);
    ui->answer_1->setEnabled(enabled);
    ui->answer_2->setEnabled(enabled);
    ui->answer_3->setEnabled(enabled);
}

void MainWindow::handle_answer(int seleced_answer) {
    enable_answering(false);
    Message answer_message;
    Answer *answer = new Answer;
    answer->set_question_id(question_id);
    answer->set_selected_answer(seleced_answer);
    answer->set_sent_at(time(nullptr) - connected_at);

    answer_message.set_allocated_answer(answer);

    send_message(answer_message);
}

void MainWindow::send_message(Message &message) {
    std::string encoded_message;
    message.SerializeToString(&encoded_message);

    socket->write(intToArray((qint32) encoded_message.size()));
    socket->write(encoded_message.data());
}
