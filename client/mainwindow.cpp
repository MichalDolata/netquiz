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

    connect(ui->connectButton, &QPushButton::clicked, this, &MainWindow::handleConnect);
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

        std::string *encoded_message = new std::string;
        setPlayerNameMessage.SerializeToString(encoded_message);

        socket->write(intToArray((qint32) encoded_message->size()));
        socket->write(encoded_message->data());
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

    bytes_to_read -= socket->read(buf, can_read_bytes);
    message_buffer += buf;

    if(!bytes_to_read) {
      current_message.ParseFromString(message_buffer);
      message_buffer.clear();
      bytes_to_read = size_bytes_to_read = 0;
      handle_message();
    }
}

void MainWindow::handle_message() {
    if(current_message.has_ranking()) {
        for(auto it: current_message.ranking().players()) {
            QString label = QString("%1 %2").arg(it.name().data()).arg(it.points());
            ui->playerList->addItem(label);
        }
    } else {
      cout << "Unsupported message" << endl;
    }
}
