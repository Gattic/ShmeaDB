#ifndef GTHREAD_H
#define GTHREAD_H

#ifdef _WIN32
    #include <windows.h>
    typedef HANDLE native_thread_t;
#else
    #include <pthread.h>
    typedef pthread_t native_thread_t;
#endif

class GThread {
public:
    GThread() : started(false) {}
    ~GThread() {}

    template <typename T>
    bool start(void* (*func)(void*), T* arg) {
#ifdef _WIN32
        handle = CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)func, arg, 0, nullptr);
        return handle != nullptr;
#else
        return pthread_create(&handle, nullptr, func, arg) == 0;
#endif
        started = true;
    }

    void join() {
#ifdef _WIN32
        if (handle) WaitForSingleObject(handle, INFINITE);
#else
        if (started) pthread_join(handle, nullptr);
#endif
    }

private:
    native_thread_t handle;
    bool started;
};

#endif // GTHREAD_H
