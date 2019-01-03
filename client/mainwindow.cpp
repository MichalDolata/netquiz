#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <string>

constexpr int MENU_WIDGET = 0;
constexpr int GAME_WIDGET = 1;

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
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    connect(ui->connectButton, &QPushButton::clicked, this, &MainWindow::handleConnect);

    ui->playerList->addItem("Test");
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::handleConnect() {
    ui->connectButton->setEnabled(false);

    QTcpSocket *socket = new QTcpSocket(this);

    socket->connectToHost("0.0.0.0", 1337);

    if(socket->waitForConnected()) {
        ui->stackedWidget->setCurrentIndex(GAME_WIDGET);

        Message setPlayerNameMessage;
        SetPlayerName *content = new SetPlayerName;
        content->set_name(ui->nickNameEdit->text().toUtf8().constData());
        setPlayerNameMessage.set_allocated_set_player_name(content);

        std::string *encoded_message = new std::string;
        setPlayerNameMessage.SerializeToString(encoded_message);

        std::cout << encoded_message->size() << std::endl;

        socket->write(intToArray((qint32) encoded_message->size()));
        socket->write(encoded_message->data());
    }

    ui->statusBar->clearMessage();
    ui->connectButton->setEnabled(true);
}
