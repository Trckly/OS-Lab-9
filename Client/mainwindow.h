#pragma once

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <regex>
#include <string>
#include <QMainWindow>
#include <QMessageBox>

//#define DANYLO_SERVER
#define ANDRII_SERVER

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "53"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    int SendServerRequest(QString MessageToSend);

    void DivideReceivedMessage(char* RecvMessage);

    void OutputReceivedData(QVector<std::string> FileNames, QVector<std::string> DateTime, std::string Bytes);

    void ClearLists();

    void CastToRelevantPath(QString& InitialPath);

private slots:
    void on_actionInstruction_triggered();

    void on_radioButton_clicked(bool checked);

    void on_pushButton_clicked();

private:
    Ui::MainWindow *ui;

    std::string ipAddr;
};
