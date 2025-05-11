#ifndef GMUTEX_H
#define GMUTEX_H

#ifdef _WIN32
    #include <windows.h>
#else
    #include <pthread.h>
#endif

class GMutex {
public:
    GMutex();
    ~GMutex();

    void lock();
    void unlock();

#ifdef _WIN32
    CRITICAL_SECTION* nativeHandle() { return &mutex; }
#else
    pthread_mutex_t* nativeHandle() { return &mutex; }
#endif

private:
#ifdef _WIN32
    CRITICAL_SECTION mutex;
#else
    pthread_mutex_t mutex;
#endif
};

#endif // GMUTEX_H
