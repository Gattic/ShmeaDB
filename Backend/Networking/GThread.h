#pragma once

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <windows.h>
#else
    #include <pthread.h>
#endif

class GThread {
public:
    GThread();
    ~GThread();

    bool start(void* (*func)(void*), void* arg);
    void join();
private:
#ifdef _WIN32
    HANDLE handle;
#else
    pthread_t handle;
#endif
    bool started;
};
