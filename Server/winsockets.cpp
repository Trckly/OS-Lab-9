#include "winsockets.h"

CRITICAL_SECTION WinSockets::Cr1TiKaL;
CacheClass WinSockets::temporary_cache;
HANDLE WinSockets::LogHandle;
LPWSTR WinSockets::LOG_PATH = L"C:\\Users\\Akmitliviy\\Documents\\Log_OS\\Log.txt";

WinSockets::WinSockets() {
    result = NULL;
    ptr = NULL;
    ListenSocket = INVALID_SOCKET;

    CreateThread(0, 0, TruncCacheTimer, 0, 0, 0);
}

bool WinSockets::InitializeSocket(){

    int iResult;
    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) {
        AddLog("WSAStartup failed: " + to_string(iResult));
        return false;
    }

    AddLog("Socket initialization succeed");
    return true;
}

bool WinSockets::CreateSocket(){

    int iResult;
    ZeroMemory(&hints, sizeof (hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the local address and port to be used by the server
    iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
    if (iResult != 0) {
        AddLog("getaddrinfo failed: " + to_string(iResult));
        WSACleanup();
        return false;
    }

    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

    if (ListenSocket == INVALID_SOCKET) {
        AddLog("Error at socket(): " + to_string(WSAGetLastError()));
        freeaddrinfo(result);
        WSACleanup();
        return false;
    }

    AddLog("Socket creation succeed\n");
    return true;
}

bool WinSockets::BindSocket(){
    int iResult;

    // Setup the TCP listening socket
    iResult = bind( ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        AddLog("bind failed with error: " + to_string(WSAGetLastError()));
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        return false;
    }

    freeaddrinfo(result);

    AddLog("bind succeed");
    return true;
}

bool WinSockets::ListenOnSocket(){

    if (listen( ListenSocket, SOMAXCONN ) == SOCKET_ERROR ) {
        AddLog("Listen failed with error: " + to_string(WSAGetLastError()));
        closesocket(ListenSocket);
        WSACleanup();
        return false;
    }
    AddLog("Listen succeed");
    return true;
}

bool WinSockets::AcceptConnection(){
    SOCKET ClientSocket;
    sockaddr_in client_adress;
    int client_adress_size = sizeof(client_adress);

    InitializeCriticalSection(&Cr1TiKaL);
    while(true){
        ClientSocket = accept(ListenSocket, reinterpret_cast<sockaddr*> (&client_adress), &client_adress_size);
        if (ClientSocket == INVALID_SOCKET) {

            AddLog("accept failed with client " + string(inet_ntoa(client_adress.sin_addr)));
            closesocket(ListenSocket);
            WSACleanup();
            return false;
        }

        AddLog("Acception succeed with client " + string(inet_ntoa(client_adress.sin_addr)));

        DWORD ThreadID;
        HANDLE ThreadHandle = CreateThread(NULL, NULL, WinSockets::ReceiveAndSend, &ClientSocket, NULL, &ThreadID);
        threads.insert(make_pair(ThreadID, ThreadHandle));
    }
    return true;
}

DWORD WINAPI WinSockets::ReceiveAndSend(LPVOID ClientSocketInput){

    EnterCriticalSection(&Cr1TiKaL);
    SOCKET ClientSocket = (reinterpret_cast<SOCKET*> (ClientSocketInput))[0];

    char recvbuf[DEFAULT_BUFLEN];
    int iResult, iSendResult;
    int recvbuflen = DEFAULT_BUFLEN;
    string names, dates, size, except, result = "Nothing";

    // Receive until the peer shuts down the connection
    do {

        iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
        if (iResult > 0) {

            AddLog("Bytes received: " + to_string(iResult));
            std::string recvstr(recvbuf);
            if((result = temporary_cache.GetCachedData(recvstr)) == "empty")
            {
                ProcessReceivedData(recvbuf, names, dates, size, except);

                if(except.empty()){
                    result = names + "?" + dates + "?" + size + "?";
                }
                else{
                    result = "<" + except + ">";
                }
                temporary_cache.CacheData(recvstr, result);
                qDebug() << "cache was not used!\n";
            }
            // Sending names
            iSendResult = send(ClientSocket, result.c_str(), result.size(), 0);
            if (iSendResult == SOCKET_ERROR) {
                AddLog("send failed: " + to_string(WSAGetLastError()));
                closesocket(ClientSocket);
                WSACleanup();
                return 1;
            }
            AddLog("Bytes sent: " + to_string(iSendResult));

        } else if (iResult == 0)
            AddLog("Connection closing...");
        else {
            AddLog("Receiving failed: " + to_string(WSAGetLastError()));
            closesocket(ClientSocket);
            WSACleanup();
            return 2;
        }

    } while (iResult > 0);

    AddLog("Receive&Send succeed");

    Disconnect(ClientSocket);
    LeaveCriticalSection(&Cr1TiKaL);
    return 0;
}

DWORD WinSockets::TruncCacheTimer(LPVOID Param)
{
    while(true){
        Sleep(20000);
        temporary_cache.ClearCache();
        qDebug() << "Cache cleared\n";
    }
}

bool WinSockets::Disconnect(SOCKET& ClientSocket){

    int iResult;

    // shutdown the send half of the connection since no more data will be sent
    iResult = shutdown(ClientSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        AddLog("shutdown failed: " + to_string(WSAGetLastError()));
        closesocket(ClientSocket);
        WSACleanup();
        return false;
    }

    // cleanup
    closesocket(ClientSocket);

    AddLog("Disconnection succeed");
    return true;
}

void WinSockets::StartServer(){
    if(!InitializeSocket()){
        return;
    }

    if(!CreateSocket()){
        return;
    }

    if(!BindSocket()){
        return;
    }

    if(!ListenOnSocket()){
        return;
    }

    if(!AcceptConnection()){
        return;
    }

    WSACleanup();

}

void WinSockets::ClearCache()
{
    temporary_cache.ClearCache();
    qDebug() << "Cache is cleared!\n";
}

void WinSockets::ProcessReceivedData(const char* client_input, string& names, string& dates, string& size, string& except){
    string input_string(client_input);

    string directory;
    string extention;

#ifdef ANDRII_SERVER
    directory = DEFAULT_ANDRII_PATH;
#elif defined(DANYLO_SERVER)
    directory = DEFAULT_DANYLO_PATH;
#else
    AddLog("Defined wrong server owner (add #define ANDRII_SERVER of #define DANYLO_SERVER)");
#endif

    regex global_regular("[^\\|]+");
    auto words_begin = sregex_iterator(input_string.begin(), input_string.end(), global_regular);
    auto words_end = sregex_iterator();

    if(words_begin != words_end)
    {
        smatch match_result = *words_begin;
        if(match_result.str() != " ")
            directory += "\\" + match_result.str();
        words_begin++;
    }

    if(words_begin != words_end)
    {
        smatch match_result = *words_begin;
        extention = match_result.str();
    }

    unsigned int i_size(0);

    bool bExist(false);
    try{
        for(const auto& entry : std::filesystem::directory_iterator{directory}){

            string dir_member = entry.path().generic_string();
            auto position = dir_member.rfind(extention);
            if(position != string::npos && extention.size() == dir_member.size() - position){
                bExist = true;

                names += ExtractFileName(dir_member) + "|";
                dates += GetCreationTime(dir_member) + "|";
                i_size += GetSizeOfFile(dir_member);
            }
        }
        size = to_string(i_size);
    }
    catch(exception& ex){
        except = "There is no such directory, try another path";
        return;
    }

    if(!bExist){
        except = "There are no files with provided extention";
        return;
    }

    return;
}

string WinSockets::ExtractFileName(const string& path){
    regex regular("[^\\/]+");

    auto word_ranner = sregex_iterator(path.begin(), path.end(), regular);
    auto word_end = sregex_iterator();
    auto target = sregex_iterator();

    while(word_ranner != word_end){
        target = word_ranner;
        word_ranner++;
    }

    smatch match = *target;
    return match.str();
}

string WinSockets::GetCreationTime(const string& path){
    string ResultCreationTime;
    HANDLE file_handle;

    FILETIME CreationTime, LastAccessTime, LastWriteTime;
    SYSTEMTIME SysCreationTime;

    file_handle = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    bool bSucceed(false);
    if(file_handle){
        bSucceed = GetFileTime(file_handle, &CreationTime, &LastAccessTime, &LastWriteTime);
    }

    if(bSucceed){
        bSucceed = FileTimeToSystemTime(&CreationTime, &SysCreationTime);
    }

    if(bSucceed){
        ResultCreationTime += to_string(SysCreationTime.wDay) + "."
                            + to_string(SysCreationTime.wMonth) + "."
                            + to_string(SysCreationTime.wYear)+ " "
                            + to_string(SysCreationTime.wHour)+ ":"
                              + to_string(SysCreationTime.wMinute);
    }

    CloseHandle(file_handle);
    return ResultCreationTime;
}

unsigned int WinSockets::GetSizeOfFile(const string& path){
    HANDLE file_handle;

    DWORD FileSize;

    file_handle = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if(file_handle){
        FileSize = GetFileSize(file_handle, NULL);
    }

    return FileSize;
}

void WinSockets::AddLog(string message){

    if((LogHandle = CreateFile(LOG_PATH, GENERIC_WRITE, 0, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0)) == INVALID_HANDLE_VALUE){
        return;
    }

    string time = "[" + GetDateTime() + "]:\t\t";

    string output(time + message + "\n");
    DWORD BytesWritten;
    SetFilePointer(LogHandle, 0, NULL, FILE_END);
    WriteFile(LogHandle, output.c_str(), output.size(), &BytesWritten, 0);

    CloseHandle(LogHandle);
}

string WinSockets::GetDateTime(){
    time_t CurrentTime = time(nullptr);
    string ret_time = ctime(&CurrentTime);
    ret_time.replace(ret_time.size() - 1, 1, "\0");
    return ret_time;
}

