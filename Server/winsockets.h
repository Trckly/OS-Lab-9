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
#include <QTimer>
#include <QDebug>
#include <ctime>
#include "cache.h"

#define DEFAULT_PORT "53"
#define DEFAULT_BUFLEN 512

#define DANYLO_SERVER
//#define ANDRII_SERVER

#define DEFAULT_ANDRII_PATH "D:\\OS\\Lab09_OS\\SearchDirectory"
#define DEFAULT_DANYLO_PATH "C:\\Users\\bossofthisgym\\Documents\\ServerMainDirectory"

using namespace std;

class WinSockets
{
private:
    WSADATA wsaData;

    struct addrinfo *result, *ptr, hints;

    SOCKET ListenSocket;

    static CRITICAL_SECTION Cr1TiKaL;

    map<DWORD, HANDLE> threads;

    static CacheClass temporary_cache;

    static HANDLE LogHandle;

    static LPWSTR LOG_PATH;

    bool InitializeSocket();

    bool CreateSocket();

    bool BindSocket();

    bool ListenOnSocket();

    bool AcceptConnection();

    static DWORD WINAPI ReceiveAndSend(LPVOID ClientSocket);

    static DWORD WINAPI TruncCacheTimer(LPVOID Param);

    static bool Disconnect(SOCKET& ClientSocket);

    static void ProcessReceivedData(const char* client_input, string& name, string& date, string& size, string& except);

    static string ExtractFileName(const string& path);

    static string GetCreationTime(const string& path);

    static unsigned int GetSizeOfFile(const string& path);

    bool SendData(const char* name, const char* date, const char* size);

    static void AddLog(string message);

    static string GetDateTime();

public:
    WinSockets();

    void StartServer();

    void ClearCache();
};

#endif // WINSOCKETS_H
