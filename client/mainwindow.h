#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtNetwork>
#include <string>
#include "message.pb.h"

using namespace message;
using namespace std;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    QTcpSocket *socket;
    char size_buf[4];
    int size_bytes_to_read;
    qint32 bytes_to_read;
    string message_buffer;
    Message current_message;
    Ui::MainWindow *ui;
    QTimer *timer;
    quint64 deadline_at;
    quint64 question_id;

    void handle_message();
    void disable_answering();
    void enable_answering();
    void handle_answer(int selected_answer);
    void send_message(Message &message);

private slots:
    void handleConnect();
    void handleRead();
    void handleTick();
};

#endif // MAINWINDOW_H
