#include "GCondVar-test.h"
#include "../../../Backend/Core/GMutex.h"
#include "../../../Backend/Core/GThread.h"
#include "../../../Backend/Core/GCondVar.h"
#include "../../unit-test.h"
#include <stdio.h>

using namespace shmea;

struct CondVarTestState
{
    GMutex*   mutex;
    GCondVar* cond;
    int       ready;
    int       result;
};

static void* waiterThread(void* arg)
{
    CondVarTestState* state = static_cast<CondVarTestState*>(arg);
    state->mutex->lock();
    while (!state->ready)
        state->cond->wait(*state->mutex);
    state->result = 99;
    state->mutex->unlock();
    return NULL;
}

void GCondVarUnitTest()
{
    printf("=== GCondVar Unit Tests ===\n");

    // Test: producer signals waiter
    {
        GMutex   m;
        GCondVar cv;
        CondVarTestState state;
        state.mutex  = &m;
        state.cond   = &cv;
        state.ready  = 0;
        state.result = 0;

        GThread t;
        t.start(waiterThread, &state);

        m.lock();
        state.ready = 1;
        cv.signal();
        m.unlock();

        t.join();
        ASSERT("Waiter set result after signal", state.result == 99);
    }

    printf("GCondVar tests passed.\n");
}
