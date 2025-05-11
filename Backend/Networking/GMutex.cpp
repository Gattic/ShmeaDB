#include "GMutex.h"

GMutex::GMutex() {
#ifdef _WIN32
    mutex = CreateMutex(NULL, FALSE, NULL);
#else
    pthread_mutex_init(&mutex, NULL);
#endif
}

GMutex::~GMutex() {
#ifdef _WIN32
    CloseHandle(mutex);
#else
    pthread_mutex_destroy(&mutex);
#endif
}

void GMutex::lock() {
#ifdef _WIN32
    WaitForSingleObject(mutex, INFINITE);
#else
    pthread_mutex_lock(&mutex);
#endif
}

void GMutex::unlock() {
#ifdef _WIN32
    ReleaseMutex(mutex);
#else
    pthread_mutex_unlock(&mutex);
#endif
}
