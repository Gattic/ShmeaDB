#include "GCondVar.h"

namespace shmea {

GCondVar::GCondVar()
{
#ifdef _WIN32
    InitializeConditionVariable(&cond);
#else
    pthread_cond_init(&cond, NULL);
#endif
}

GCondVar::~GCondVar()
{
#ifndef _WIN32
    pthread_cond_destroy(&cond);
#endif
}

void GCondVar::wait(GMutex& m)
{
#ifdef _WIN32
    SleepConditionVariableCS(&cond, m.nativeHandle(), INFINITE);
#else
    pthread_cond_wait(&cond, m.nativeHandle());
#endif
}

bool GCondVar::timedWait(GMutex& m, unsigned long ms)
{
#ifdef _WIN32
    BOOL res = SleepConditionVariableCS(&cond, m.nativeHandle(), (DWORD)ms);
    return res != 0;
#else
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec  += (long)(ms / 1000);
    ts.tv_nsec += (long)(ms % 1000) * 1000000L;
    if (ts.tv_nsec >= 1000000000L) { ts.tv_sec++; ts.tv_nsec -= 1000000000L; }
    int rc = pthread_cond_timedwait(&cond, m.nativeHandle(), &ts);
    return (rc == 0);
#endif
}

void GCondVar::signal()
{
#ifdef _WIN32
    WakeConditionVariable(&cond);
#else
    pthread_cond_signal(&cond);
#endif
}

void GCondVar::broadcast()
{
#ifdef _WIN32
    WakeAllConditionVariable(&cond);
#else
    pthread_cond_broadcast(&cond);
#endif
}

} // namespace shmea
