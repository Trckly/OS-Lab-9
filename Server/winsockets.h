#ifndef WINSOCKETS_H
#define WINSOCKETS_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <stdio.h>
#include <string>
#include <map>
#include <iterator>
#include <regex>
#include <filesystem>

#define DEFAULT_PORT "53"
#define DEFAULT_BUFLEN 512

using namespace std;

class WinSockets
{
private:
    WSADATA wsaData;

    struct addrinfo *result, *ptr, hints;

    SOCKET ListenSocket;

    static CRITICAL_SECTION Cr1TiKaL;

    map<DWORD, HANDLE> threads;

    bool InitializeSocket();

    bool CreateSocket();

    bool BindSocket();

    bool ListenOnSocket();

    bool AcceptConnection();

    static DWORD WINAPI ReceiveAndSend(LPVOID ClientSocket);

    static bool Disconnect(SOCKET& ClientSocket);

    static void ProcessReceivedData(const char* client_input, string& name, string& date, string& size, string& except);

    static string ExtractFileName(const string& path);

    static string GetCreationTime(const string& path);

    static unsigned int GetSizeOfFile(const string& path);

    bool SendData(const char* name, const char* date, const char* size);

public:
    WinSockets();

    void StartServer();

};

#endif // WINSOCKETS_H
