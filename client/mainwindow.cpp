#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <string>

constexpr int MENU_WIDGET = 0;
constexpr int GAME_WIDGET = 1;
constexpr int BUF_SIZE = 255;

using namespace message;

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

    disable_answering();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::handleConnect() {
    ui->connectButton->setEnabled(false);

    socket = new QTcpSocket(this);
    socket->connectToHost("0.0.0.0", 1337);

    if(socket->waitForConnected()) {
        ui->stackedWidget->setCurrentIndex(GAME_WIDGET);

        Message setPlayerNameMessage;
        SetPlayerName *content = new SetPlayerName;
        content->set_name(ui->nickNameEdit->text().toUtf8().constData());
        setPlayerNameMessage.set_allocated_set_player_name(content);

        send_message(setPlayerNameMessage);
    }
    connect(socket, &QTcpSocket::readyRead, this, &MainWindow::handleRead);

    ui->statusBar->clearMessage();
    ui->connectButton->setEnabled(true);
}

// MOVE TO DIFFRENT CLASS
void MainWindow::handleRead() {
    if(!bytes_to_read) {
      size_bytes_to_read -= socket->read((size_buf + (4 - size_bytes_to_read)), size_bytes_to_read);

      if (size_bytes_to_read > 0) return;
      else {
        bytes_to_read = (size_buf[0] << 24) | (size_buf[1] << 16) | (size_buf[2] << 8) | (size_buf[3]);
        size_bytes_to_read = 0;
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
        for(auto it: current_message.ranking().players()) {
            QString label = QString("%1 %2").arg(it.name().data()).arg(it.points());
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
      deadline_at = question.deadline_at();
      timer->start();
      enable_answering();
    } else {
      cout << "Unsupported message" << endl;
    }
}

void MainWindow::handleTick() {
    qint64 timeLeft = (qint64)deadline_at - time(nullptr);
    if (timeLeft >= 0) ui->timerLabel->setText(QString("%1 seconds left").arg(timeLeft));
    else {
        ui->timerLabel->setText("Waiting for a next question");
        disable_answering();
        timer->stop();
    }
}

void MainWindow::disable_answering() {
    ui->answer_0->setEnabled(false);
    ui->answer_1->setEnabled(false);
    ui->answer_2->setEnabled(false);
    ui->answer_3->setEnabled(false);
}

void MainWindow::enable_answering() {
    ui->answer_0->setEnabled(true);
    ui->answer_1->setEnabled(true);
    ui->answer_2->setEnabled(true);
    ui->answer_3->setEnabled(true);
}

void MainWindow::handle_answer(int seleced_answer) {
    disable_answering();
    Message answer_message;
    Answer *answer = new Answer;
    answer->set_question_id(question_id);
    answer->set_selected_answer(seleced_answer);
    answer->set_sent_at(time(nullptr));

    answer_message.set_allocated_answer(answer);

    send_message(answer_message);
}

void MainWindow::send_message(Message &message) {
    std::string *encoded_message = new std::string;
    message.SerializeToString(encoded_message);

    socket->write(intToArray((qint32) encoded_message->size()));
    socket->write(encoded_message->data());
}
