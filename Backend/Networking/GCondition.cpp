#include "GCondition.h"
#include "GMutex.h" // Required for native mutex access

GCondition::GCondition()
{
#ifdef _WIN32
    InitializeConditionVariable(&condVar);
#else
    pthread_cond_init(&condVar, nullptr);
#endif
}

GCondition::~GCondition()
{
#ifndef _WIN32
    pthread_cond_destroy(&condVar);
#endif
}

void GCondition::wait(GMutex& mutex)
{
#ifdef _WIN32
    SleepConditionVariableCS(&condVar, mutex.nativeHandle(), INFINITE);
#else
    pthread_cond_wait(&condVar, mutex.nativeHandle());
#endif
}

void GCondition::signal()
{
#ifdef _WIN32
    WakeConditionVariable(&condVar);
#else
    pthread_cond_signal(&condVar);
#endif
}

void GCondition::broadcast()
{
#ifdef _WIN32
    WakeAllConditionVariable(&condVar);
#else
    pthread_cond_broadcast(&condVar);
#endif
}
