#ifndef GCONDITION_H
#define GCONDITION_H

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

class GMutex; // Forward declaration (you likely already have this)

class GCondition
{
public:
    GCondition();
    ~GCondition();

    void wait(GMutex& mutex);
    void signal();
    void broadcast();

private:
#ifdef _WIN32
    CONDITION_VARIABLE condVar;
#else
    pthread_cond_t condVar;
#endif
};

#endif
