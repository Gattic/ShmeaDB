#include "GThread.h"

struct ThreadParams {
    void* (*func)(void*);
    void* arg;
};

#ifdef _WIN32
DWORD WINAPI threadFuncWrapper(LPVOID lpParam) {
    ThreadParams* params = static_cast<ThreadParams*>(lpParam);
    params->func(params->arg);
    delete params;
    return 0;
}
#else
void* threadFuncWrapper(void* lpParam) {
    ThreadParams* params = static_cast<ThreadParams*>(lpParam);
    params->func(params->arg);
    delete params;
    return NULL;
}
#endif

GThread::GThread() : handle(0), started(false) {}
GThread::~GThread() {}

bool GThread::start(void* (*func)(void*), void* arg) {
    started = true;
    ThreadParams* params = new ThreadParams{func, arg};
#ifdef _WIN32
    handle = CreateThread(NULL, 0, threadFuncWrapper, params, 0, NULL);
    return handle != NULL;
#else
    return pthread_create(&handle, NULL, threadFuncWrapper, params) == 0;
#endif
}

void GThread::join() {
#ifdef _WIN32
    if (handle) WaitForSingleObject(handle, INFINITE);
#else
    if (started) pthread_join(handle, NULL);
#endif
}
