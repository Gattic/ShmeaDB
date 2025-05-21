// platform.h
#pragma once

// Use these macros for socket options to handle Windows/Linux pointer type differences.
#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <signal.h>
    #pragma comment(lib, "ws2_32.lib")
    typedef SOCKET sock_t;
    #define CLOSESOCK(s) closesocket(s)
    #define SHMEA_INVALID_SOCKET INVALID_SOCKET
    #define SOCKOPT_CAST (const char*)
    #define SOCKOPT_GCAST (char*)
    #define SHMEA_SHUT_RDWR SD_BOTH
    #define SHMEA_ETIMEDOUT WSAETIMEDOUT
#else
    #include <sys/socket.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <sys/signal.h>
    typedef int sock_t;
    #define CLOSESOCK(s) close(s)
    #define SHMEA_INVALID_SOCKET -1
    #define SOCKOPT_CAST //Expects a const void* or void*
    #define SOCKOPT_GCAST
    #define SHMEA_SHUT_RDWR SHUT_RDWR
    #define SHMEA_ETIMEDOUT ETIMEDOUT
#endif