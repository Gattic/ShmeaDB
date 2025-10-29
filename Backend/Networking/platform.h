#pragma once

// Platform-specific includes and socket type defs
#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <signal.h>
    #ifdef _MSC_VER
        #pragma comment(lib, "ws2_32.lib")
    #endif

    typedef SOCKET sock_t;  // Windows sockets are unsigned (SOCKET is UINT_PTR)
    #define CLOSESOCK(s) closesocket(s)
    #define SHMEA_INVALID_SOCKET INVALID_SOCKET  // Typically ((SOCKET)(~0))
    #define INVALID_SOCK_VALUE INVALID_SOCKET    // Alias for consistency
    #define SOCKOPT_CAST (const char*)
    #define SOCKOPT_GCAST (char*)
    #define SHMEA_SHUT_RDWR SD_BOTH
    #define SHMEA_ETIMEDOUT WSAETIMEDOUT

#else
    #include <sys/socket.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <sys/signal.h>

    typedef int sock_t;   // UNIX sockets are signed int
    #define CLOSESOCK(s) close(s)
    #define SHMEA_INVALID_SOCKET -1
    #define INVALID_SOCK_VALUE -1    // Alias for consistency
    #define SOCKOPT_CAST             // Expects a const void* or void*
    #define SOCKOPT_GCAST
    #define SHMEA_SHUT_RDWR SHUT_RDWR
    #define SHMEA_ETIMEDOUT ETIMEDOUT

#endif
