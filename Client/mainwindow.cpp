#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->ExtentionLineEdit->setPlaceholderText("Enter extention:");
    ui->PathLineEdit->setPlaceholderText("Full path to directory:");
    ui->radioButton->setChecked(false);
    ui->PathLineEdit->setDisabled(true);

    ipAddr = "10.211.55.6";
}

MainWindow::~MainWindow()
{
    delete ui;
}

int MainWindow::SendServerRequest(QString MessageToSend){
    WSADATA wsaData;
    SOCKET ConnectSocket = INVALID_SOCKET;
    struct addrinfo *result = NULL,
        *ptr = NULL,
        hints;
    const char *sendbuf = MessageToSend.toUtf8().constData();
    char recvbuf[DEFAULT_BUFLEN];
    int iResult;
    int recvbuflen = DEFAULT_BUFLEN;

    // Validate the parameters
    // if (argc != 2) {
    //     QMessageBox::information(this, "Output", "usage: %s server-name\n", ClientName);
    //     return 1;
    // }

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) {
        QString LogMessage = "WSAStartup failed with error: " + iResult;
        QMessageBox::information(this, "Output", LogMessage);
        return 1;
    }

    ZeroMemory( &hints, sizeof(hints) );
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // Resolve the server address and port
    iResult = getaddrinfo(ipAddr.c_str(), DEFAULT_PORT, &hints, &result);
    if ( iResult != 0 ) {
        QString LogMessage = "getaddrinfo failed with error: " + iResult;
        QMessageBox::information(this, "Output", LogMessage);
        WSACleanup();
        return 1;
    }

    // Attempt to connect to an address until one succeeds
    for(ptr=result; ptr != NULL ;ptr=ptr->ai_next) {

        // Create a SOCKET for connecting to server
        ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype,
                               ptr->ai_protocol);
        if (ConnectSocket == INVALID_SOCKET) {
            QString LogMessage = "socket failed with error:" + WSAGetLastError();
            QMessageBox::information(this, "Output", LogMessage);
            WSACleanup();
            return 1;
        }

        // Connect to server.
        iResult = WSAAPI::connect( ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            closesocket(ConnectSocket);
            ConnectSocket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(result);

    if (ConnectSocket == INVALID_SOCKET) {
        QString LogMessage = "Unable to connect to server!";
        QMessageBox::information(this, "Output", LogMessage);
        WSACleanup();
        return 1;
    }

    // Send an initial buffer
    iResult = send( ConnectSocket, sendbuf, (int)strlen(sendbuf), 0 );
    if (iResult == SOCKET_ERROR) {
        QString LogMessage = "send failed with error: " + WSAGetLastError();
        QMessageBox::information(this, "Output", LogMessage);
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }

    QString Message = "Bytes Sent: " + QString::number(iResult);
    QMessageBox::information(this, "Output", Message);

    // shutdown the connection since no more data will be sent
    iResult = shutdown(ConnectSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        QString LogMessage = "shutdown failed with error: " + WSAGetLastError();
        QMessageBox::information(this, "Output", LogMessage);
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }

    // Receive until the peer closes the connection
    do {
        iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
        QString LogMessage;
        if ( iResult > 0 ){
            LogMessage += "Bytes received: " + QString::number(iResult) + "\nReceived message: " + recvbuf;
            QMessageBox::information(this, "Output", LogMessage);
        }
        else if ( iResult == 0 ){
            LogMessage = "Connection closed";
            QMessageBox::information(this, "Output", LogMessage);
        }
        else{
            LogMessage = "recv failed with error: " + WSAGetLastError();
            QMessageBox::information(this, "Output", LogMessage);
        }

    } while( iResult > 0 );

    // cleanup
    closesocket(ConnectSocket);
    WSACleanup();

    return 0;
}

void MainWindow::on_actionInstruction_triggered()
{
    QMessageBox::information(this, "Instruction", "You can either enter file extentions with dot (.) or without.\n"
                                    "\tExample: \"txt\" or \".txt\"");
}


void MainWindow::on_radioButton_clicked(bool checked)
{
    ui->PathLineEdit->setDisabled(!checked);
}

bool isInapropriateCharacter(char ch){
    return ch == '\\' || ch == '/' || ch == ':' || ch == '*' || ch == '?' || ch == '\"' || ch == '<' || ch == '>' || ch == '|';
}

void MainWindow::on_pushButton_clicked()
{
    QString ExtentionText = ui->ExtentionLineEdit->text();
    if(ExtentionText.isEmpty()){
        QMessageBox::information(this, "Assertion", "You have to enter file extention!");
        return;
    }

    if(ExtentionText[0] != '.'){
        ExtentionText.insert(0, '.');
    }

    for(int i = 1; i < ExtentionText.size(); ++i){
        if(ExtentionText[i].isDigit()){
            if(isInapropriateCharacter(ExtentionText[i].toLatin1())){
                QMessageBox::information(this, "Assertion", "You have illegal characters in file extention name!");
                return;
            }
            if(i == ExtentionText.size() - 1 && ExtentionText[i] == '.'){
                QMessageBox::information(this, "Assertion", "You cannot use dot (.) in the end of file extention name!");
                return;
            }
        }
    }

    QString PathText;
    if(!ui->radioButton->isChecked()){
        PathText = "default";
    }
    else{
        PathText = ui->PathLineEdit->text();
    }

    QString WrappedMessage = ExtentionText + '|' + PathText;
    SendServerRequest(WrappedMessage);
}

