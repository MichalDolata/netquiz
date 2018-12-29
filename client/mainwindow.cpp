#include "mainwindow.h"
#include "ui_mainwindow.h"

constexpr int MENU_WIDGET = 0;
constexpr int GAME_WIDGET = 1;

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
    ui->statusBar->showMessage("Connecting...");

    for(int i = 0; i < 1000000; i++) {
        for(int j = 0; j < 1000; j++) {
        }
    }

    ui->stackedWidget->setCurrentIndex(GAME_WIDGET);

    ui->statusBar->clearMessage();
    ui->connectButton->setEnabled(true);
}
