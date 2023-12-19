#include "mainwindow.h"
#include "ui_mainwindow.h"

void ClearLists();

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->ExtentionLineEdit->setPlaceholderText("Enter extention:");
    ui->PathLineEdit->setPlaceholderText("Full path to directory:");
    ui->radioButton->setChecked(false);
    ui->PathLineEdit->setDisabled(true);

    ui->BytesSumText->setReadOnly(true);
#ifdef ANDRII_SERVER
    ipAddr = "10.0.1.15";
#elif defined(DANYLO_SERVER)
    ipAddr = "10.211.55.6";
#else
    QMessageBox::critical(this, "Connection", "Please define local ipv4 adress!");
#endif
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
        QMessageBox::critical(this, "Output", LogMessage);
        WSACleanup();
        return 1;
    }

    // Send an initial buffer
    iResult = send( ConnectSocket, sendbuf, (int)strlen(sendbuf), 0 );
    if (iResult == SOCKET_ERROR) {
        QString LogMessage = "send failed with error: " + WSAGetLastError();
        QMessageBox::critical(this, "Output", LogMessage);
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }

    qDebug() << "Bytes Sent: " + iResult;

    // shutdown the connection since no more data will be sent
    iResult = shutdown(ConnectSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        QString LogMessage = "shutdown failed with error: " + WSAGetLastError();
        QMessageBox::critical(this, "Output", LogMessage);
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }

    // Receive until the peer closes the connection
    do {
        iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
        if ( iResult > 0 ){
            DivideReceivedMessage(recvbuf);
            qDebug() << "Bytes received: " << iResult << "\nReceived message: " << recvbuf;
        }
        else if ( iResult == 0 ){
            qDebug() << "Connection closed";
        }
        else{
            qDebug() << "recv failed with error: " << WSAGetLastError();
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
        QMessageBox::critical(this, "Assertion", "You have to enter file extention!");
        return;
    }

    if(ExtentionText[0] != '.'){
        ExtentionText.insert(0, '.');
    }

    for(int i = 1; i < ExtentionText.size(); ++i){
        if(!ExtentionText[i].isLetter()){
            QMessageBox::critical(this, "Assertion", "You have illegal characters in file extention name!");
            return;
        }
    }

    QString PathText;
    if(!ui->radioButton->isChecked()){
#ifdef ANDRII_SERVER
        PathText = DEFAULT_ANDRII_PATH;
#elif defined(DANYLO_SERVER)
        PathText = DEFAULT_DANYLO_PATH;
#endif
    }
    else{
        PathText = ui->PathLineEdit->text();
        CastToRelevantPath(PathText);
    }

    QString WrappedMessage = PathText + '|' + ExtentionText;
    SendServerRequest(WrappedMessage);
}

void MainWindow::DivideReceivedMessage(char* RecvMessage)
{
    std::string Data(RecvMessage);

    if(Data[0] == '<'){
        ClearLists();
        Data.erase(Data.begin());
        Data.erase(Data.end() - 1);
        QMessageBox::critical(this, "Server Error Message", QString::fromStdString(Data));
        return;
    }

    std::string Name, DateTime, Bytes;
    QVector<std::string> NameUnits;
    QVector<std::string> DateUnits;

    std::regex regular("[^\\?]+");
    auto words_begin = std::sregex_iterator(Data.begin(), Data.end(), regular);
    auto words_end = std::sregex_iterator();

    if(words_begin != words_end)
    {
        std::smatch match_result = *words_begin;
        Name = match_result.str();
        words_begin++;
    }

    if(words_begin != words_end)
    {
        std::smatch match_result = *words_begin;
        DateTime = match_result.str();
        words_begin++;
    }

    if(words_begin != words_end)
    {
        std::smatch match_result = *words_begin;
        Bytes = match_result.str();
    }

    std::regex regular_name("[^\\|]+");
    auto words_begin_name = std::sregex_iterator(Name.begin(), Name.end(), regular_name);
    auto words_end_name = std::sregex_iterator();
    for(auto i = words_begin_name; i != words_end_name; ++i){
        std::smatch match_result = *i;
        NameUnits.append(match_result.str());
    }

    std::regex regular_date("[^\\|]+");
    auto words_begin_date = std::sregex_iterator(DateTime.begin(), DateTime.end(), regular_date);
    auto words_end_date = std::sregex_iterator();
    for(auto i = words_begin_date; i != words_end_date; ++i){
        std::smatch match_result = *i;
        DateUnits.append(match_result.str());
    }

   OutputReceivedData(NameUnits, DateUnits, Bytes);
}

void MainWindow::ClearLists(){
    ui->NameList->clear();
    ui->DateList->clear();
    ui->BytesSumText->clear();
}

void MainWindow::OutputReceivedData(QVector<std::string> FileNames, QVector<std::string> DateTime, std::string Bytes)
{
    ClearLists();
    ui->BytesSumText->setText(QString::fromStdString(Bytes));

    for(int i = 0; i < FileNames.size(); ++i)
    {
        // Create QListWidgetItem objects and add them to the QLists
        QListWidgetItem *nameItem = new QListWidgetItem(QString::fromStdString(FileNames[i]));
        QListWidgetItem *dateItem = new QListWidgetItem(QString::fromStdString(DateTime[i]));

        ui->NameList->addItem(nameItem);
        ui->DateList->addItem(dateItem);
    }
}

void MainWindow::CastToRelevantPath(QString& InitialPath){
    for (int i = 0; i < InitialPath.size(); ++i){
        if(InitialPath[i] == '/'){
            InitialPath.erase(InitialPath.begin() + i);
            InitialPath.insert(i, "\\\\");
        }
    }
}
