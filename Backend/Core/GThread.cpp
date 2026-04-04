#include "GThread.h"
#include <stddef.h>

namespace shmea {

#ifdef _WIN32
struct GThreadThunkArgs
{
    GThread::ThreadFunc func;
    void* arg;
};

static DWORD WINAPI gthread_thunk(LPVOID p)
{
    GThreadThunkArgs* args = static_cast<GThreadThunkArgs*>(p);
    GThread::ThreadFunc f = args->func;
    void* a = args->arg;
    delete args;
    f(a);
    return 0;
}
#endif

GThread::GThread() : started(false)
{
#ifdef _WIN32
    handle = NULL;
#else
    handle = 0;
#endif
}

GThread::~GThread()
{
#ifdef _WIN32
    if (started && handle != NULL)
        CloseHandle(handle);
#endif
}

bool GThread::start(ThreadFunc func, void* arg)
{
    if (started)
        return false;

#ifdef _WIN32
    GThreadThunkArgs* thunkArgs = new GThreadThunkArgs();
    thunkArgs->func = func;
    thunkArgs->arg  = arg;
    handle = CreateThread(NULL, 0, gthread_thunk, thunkArgs, 0, NULL);
    if (handle == NULL)
    {
        delete thunkArgs;
        return false;
    }
#else
    if (pthread_create(&handle, NULL, func, arg) != 0)
        return false;
#endif
    started = true;
    return true;
}

bool GThread::join()
{
    if (!started)
        return false;
#ifdef _WIN32
    DWORD res = WaitForSingleObject(handle, INFINITE);
    CloseHandle(handle);
    handle  = NULL;
    started = false;
    return (res == WAIT_OBJECT_0);
#else
    int res = pthread_join(handle, NULL);
    started = false;
    return (res == 0);
#endif
}

bool GThread::detach()
{
    if (!started)
        return false;
#ifdef _WIN32
    CloseHandle(handle);
    handle  = NULL;
    started = false;
    return true;
#else
    int res = pthread_detach(handle);
    started = false;
    return (res == 0);
#endif
}

bool GThread::isStarted() const
{
    return started;
}

} // namespace shmea
