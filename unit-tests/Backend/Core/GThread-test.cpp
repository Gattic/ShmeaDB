#include "GThread-test.h"
#include "../../../Backend/Core/GThread.h"
#include "../../unit-test.h"
#include <stdio.h>

using namespace shmea;

static int sharedValue = 0;

static void* setValueThread(void* arg)
{
    sharedValue = *static_cast<int*>(arg);
    return NULL;
}

void GThreadUnitTest()
{
    printf("=== GThread Unit Tests ===\n");

    // Test: start and join a thread that modifies shared state
    {
        sharedValue = 0;
        int expected = 42;
        GThread t;
        ASSERT("GThread not started initially", !t.isStarted());
        bool ok = t.start(setValueThread, &expected);
        ASSERT("GThread start succeeds", ok);
        ASSERT("GThread isStarted after start", t.isStarted());
        t.join();
        ASSERT("Thread modified shared value", sharedValue == 42);
        ASSERT("GThread not started after join", !t.isStarted());
    }

    printf("GThread tests passed.\n");
}
