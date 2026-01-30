#pragma once

// Platform-specific includes and socket type defs
// In Windows, sock_t is SOCKET (unsigned-ish UINT_PTR)
// In Linux sock_t is int and invalid is -1
#ifdef _WIN32
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif

    // winsock2 must come before any windows.h includes elsewhere
    #include <winsock2.h>
    #include <ws2tcpip.h>

    #ifdef _MSC_VER
        #pragma comment(lib, "ws2_32.lib")
    #endif

    typedef SOCKET sock_t;
    #define SHMEA_INVALID_SOCKET INVALID_SOCKET
    #define SHMEA_SOCKERR        SOCKET_ERROR
    #define SHMEA_SOCKLEN        int

    #define CLOSESOCK(s)         closesocket(s)

    #define SOCKOPT_CAST  (const char*)
    #define SOCKOPT_GCAST  (char*)

    #define SHMEA_SHUT_RDWR SD_BOTH
    #define SHMEA_ETIMEDOUT WSAETIMEDOUT

    inline int SHMEA_RECV(sock_t s, char* buf, int len) { return ::recv(s, buf, len, 0); }
    inline int SHMEA_SEND(sock_t s, const char* buf, int len) { return ::send(s, buf, len, 0); }
    inline int SHMEA_LAST_SOCK_ERR() { return WSAGetLastError(); }

    inline void SHMEA_WINSOCK_INIT()
    {
        static bool inited = []() {
            WSADATA wsa{};
            return WSAStartup(MAKEWORD(2,2), &wsa) == 0;
        }();
        (void)inited;
    }

#else
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <netinet/tcp.h>
    #include <unistd.h>
    #include <errno.h>

    typedef int sock_t;
    #define SHMEA_INVALID_SOCKET (-1)
    #define SHMEA_SOCKERR        (-1)
    #define SHMEA_SOCKLEN        socklen_t

    #define CLOSESOCK(s)         ::close(s)

    // Linux/POSIX setsockopt takes (const void*) / (void*)
    #define SOCKOPT_CAST
    #define SOCKOPT_GCAST

    #define SHMEA_SHUT_RDWR SHUT_RDWR
    #define SHMEA_ETIMEDOUT ETIMEDOUT

    inline int SHMEA_RECV(sock_t s, char* buf, int len) { return (int)::read(s, buf, (size_t)len); }
    inline int SHMEA_SEND(sock_t s, const char* buf, int len) { return (int)::write(s, buf, (size_t)len); }
    inline int SHMEA_LAST_SOCK_ERR() { return errno; }

    inline void SHMEA_WINSOCK_INIT() {} // no-op
#endif

// Optional helper
inline bool SHMEA_SOCKET_VALID(sock_t s) { return s != SHMEA_INVALID_SOCKET; }
