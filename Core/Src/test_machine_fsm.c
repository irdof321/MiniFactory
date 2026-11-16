
#include "test_machine_fsm.h"
#include "machine_fsm.h"
#include <stdbool.h>

/* ============================================================
 * TEST HELPERS
 * ============================================================ */

static void TestCheck(
    TestResult *result,
    bool condition,
    uint32_t line
)
{
    if (condition)
    {
        result->passed++;
    }
    else
    {
        result->failed++;

        if (result->firstFailedLine == 0)
        {
            result->firstFailedLine = line;
        }
    }
}


/* ============================================================
 * EVENT QUEUE TESTS
 * ============================================================ */

TestResult TestEventQueue(void)
{
    TestResult result = {0};

    StateMachineEvent event;
    EventQueueStatus status;
    EventQueue queue;


    /* ========================================================
     * TEST 1 - QUEUE INITIALIZATION
     * ======================================================== */

    status = EventQueueInit(&queue);

    TestCheck(&result, status == EVENT_QUEUE_OK, __LINE__);

    if (status != EVENT_QUEUE_OK)
    {
        return result;
    }

    TestCheck(&result, queue.count == 0, __LINE__);
    TestCheck(&result, queue.head == 0, __LINE__);
    TestCheck(&result, queue.tail == 0, __LINE__);


    /* ========================================================
     * TEST 2 - PUSH THREE EVENTS
     * ======================================================== */

    StateMachineEvent ev_1 = {.id = 1};
    StateMachineEvent ev_2 = {.id = 2};
    StateMachineEvent ev_3 = {.id = 3};

    status = EventQueuePush(&queue, &ev_1);
    TestCheck(&result, status == EVENT_QUEUE_OK, __LINE__);

    status = EventQueuePush(&queue, &ev_2);
    TestCheck(&result, status == EVENT_QUEUE_OK, __LINE__);

    status = EventQueuePush(&queue, &ev_3);
    TestCheck(&result, status == EVENT_QUEUE_OK, __LINE__);

    TestCheck(&result, queue.count == 3, __LINE__);


    /* ========================================================
     * TEST 3 - POP FIRST EVENT
     * ======================================================== */

    status = EventQueuePop(&queue, &event);

    TestCheck(&result, status == EVENT_QUEUE_OK, __LINE__);

    if (status == EVENT_QUEUE_OK)
    {
        TestCheck(&result, event.id == 1, __LINE__);
    }

    TestCheck(&result, queue.count == 2, __LINE__);


    /* ========================================================
     * TEST 4 - POP SECOND EVENT
     * ======================================================== */

    status = EventQueuePop(&queue, &event);

    TestCheck(&result, status == EVENT_QUEUE_OK, __LINE__);

    if (status == EVENT_QUEUE_OK)
    {
        TestCheck(&result, event.id == 2, __LINE__);
    }

    TestCheck(&result, queue.count == 1, __LINE__);


    /* ========================================================
     * TEST 5 - POP THIRD EVENT
     * ======================================================== */

    status = EventQueuePop(&queue, &event);

    TestCheck(&result, status == EVENT_QUEUE_OK, __LINE__);

    if (status == EVENT_QUEUE_OK)
    {
        TestCheck(&result, event.id == 3, __LINE__);
    }

    TestCheck(&result, queue.count == 0, __LINE__);


    /* ========================================================
     * TEST 6 - POP FROM EMPTY QUEUE
     * ======================================================== */

    status = EventQueuePop(&queue, &event);

    TestCheck(&result, status == EVENT_QUEUE_EMPTY, __LINE__);


    /* ========================================================
     * TEST 7 - FILL QUEUE TO MAXIMUM CAPACITY
     * ======================================================== */

    for (uint8_t i = 0; i < MAX_EVENT_QUEUE; ++i)
    {
        StateMachineEvent ev = {.id = i};

        status = EventQueuePush(&queue, &ev);

        TestCheck(&result, status == EVENT_QUEUE_OK, __LINE__);
    }

    TestCheck(
        &result,
        queue.count == MAX_EVENT_QUEUE,
        __LINE__
    );


    /* ========================================================
     * TEST 8 - PUSH INTO FULL QUEUE
     * ======================================================== */

    StateMachineEvent ev_21 = {.id = 21};

    status = EventQueuePush(&queue, &ev_21);

    TestCheck(
        &result,
        status == EVENT_QUEUE_CAPACITY_FULL,
        __LINE__
    );

    TestCheck(
        &result,
        queue.count == MAX_EVENT_QUEUE,
        __LINE__
    );


    /* ========================================================
     * TEST 9 - REMOVE FIVE EVENTS
     * ======================================================== */

    for (uint8_t i = 0; i < 5; ++i)
    {
        StateMachineEvent ev;

        status = EventQueuePop(&queue, &ev);

        TestCheck(&result, status == EVENT_QUEUE_OK, __LINE__);

        if (status == EVENT_QUEUE_OK)
        {
            TestCheck(&result, ev.id == i, __LINE__);
        }
    }

    TestCheck(
        &result,
        queue.count == MAX_EVENT_QUEUE - 5,
        __LINE__
    );


    /* ========================================================
     * TEST 10 - WRAP-AROUND INSERTION
     * ======================================================== */

    for (uint8_t i = 20; i < 25; ++i)
    {
        StateMachineEvent ev = {.id = i};

        status = EventQueuePush(&queue, &ev);

        TestCheck(&result, status == EVENT_QUEUE_OK, __LINE__);
    }

    TestCheck(
        &result,
        queue.count == MAX_EVENT_QUEUE,
        __LINE__
    );


    /* ========================================================
     * TEST 11 - VERIFY FIFO ORDER AFTER WRAP-AROUND
     * ======================================================== */

    for (uint8_t i = 5; i < 25; ++i)
    {
        StateMachineEvent ev;

        status = EventQueuePop(&queue, &ev);

        TestCheck(&result, status == EVENT_QUEUE_OK, __LINE__);

        if (status == EVENT_QUEUE_OK)
        {
            TestCheck(&result, ev.id == i, __LINE__);
        }
    }


    /* ========================================================
     * TEST 12 - VERIFY FINAL EMPTY QUEUE STATE
     * ======================================================== */

    TestCheck(&result, queue.count == 0, __LINE__);
    TestCheck(&result, queue.head == queue.tail, __LINE__);

    /* ========================================================
    * TEST 13 - NULL POINTER VALIDATION
    * ======================================================== */

    TestCheck(&result,
        EventQueuePush(NULL, &event) == EVENT_QUEUE_NOT_VALID_ARGUMENT,
        __LINE__);

    TestCheck(&result,
        EventQueuePush(&queue, NULL) == EVENT_QUEUE_NOT_VALID_ARGUMENT,
        __LINE__);

    TestCheck(&result,
        EventQueuePop(NULL, &event) == EVENT_QUEUE_NOT_VALID_ARGUMENT,
        __LINE__);

    TestCheck(&result,
        EventQueuePop(&queue, NULL) == EVENT_QUEUE_NOT_VALID_ARGUMENT,
        __LINE__);

    /* ========================================================
     * RETURN TEST RESULTS
     * ======================================================== */

    return result;
}




/* ============================================================
 * STATE MACHINE INITIALIZATION TESTS
 * ============================================================ */

TestResult TestStateMachineInitialize(void)
{
    TestResult result = {0};
    StateMachineStatus status;


    /* ========================================================
     * TEST 1 - NULL POINTER VALIDATION
     * ======================================================== */

    status = StateMachineInitialize(NULL);

    TestCheck(
        &result,
        status == STATE_MACHINE_INVALID_ARGUMENT,
        __LINE__
    );


    /* ========================================================
     * TEST 2 - VALID STATE MACHINE INITIALIZATION
     * ======================================================== */

    StateMachine sm;

    status = StateMachineInitialize(&sm);

    TestCheck(
        &result,
        status == STATE_MACHINE_INIT_OK,
        __LINE__
    );

    if (status != STATE_MACHINE_INIT_OK)
    {
        return result;
    }


    /* ========================================================
     * TEST 3 - HIGH PRIORITY QUEUE INITIALIZATION
     * ======================================================== */

    TestCheck(&result, sm.highEventQueue.head == 0, __LINE__);
    TestCheck(&result, sm.highEventQueue.tail == 0, __LINE__);
    TestCheck(&result, sm.highEventQueue.count == 0, __LINE__);


    /* ========================================================
     * TEST 4 - NORMAL PRIORITY QUEUE INITIALIZATION
     * ======================================================== */

    TestCheck(&result, sm.normalEventQueue.head == 0, __LINE__);
    TestCheck(&result, sm.normalEventQueue.tail == 0, __LINE__);
    TestCheck(&result, sm.normalEventQueue.count == 0, __LINE__);


    /* ========================================================
     * TEST 5 - LOW PRIORITY QUEUE INITIALIZATION
     * ======================================================== */

    TestCheck(&result, sm.lowEventQueue.head == 0, __LINE__);
    TestCheck(&result, sm.lowEventQueue.tail == 0, __LINE__);
    TestCheck(&result, sm.lowEventQueue.count == 0, __LINE__);


    return result;
}



/* ============================================================
 * STATE MACHINE PUSH EVENT TESTS
 * ============================================================ */

TestResult TestStateMachinePushEvent(void)
{
    TestResult result = {0};

    StateMachine sm;
    StateMachineStatus status;
    StateMachineEvent receivedEvent;


    /* ========================================================
     * TEST 1 - STATE MACHINE INITIALIZATION
     * ======================================================== */

    TestCheck(
        &result,
        StateMachineInitialize(&sm) == STATE_MACHINE_INIT_OK,
        __LINE__
    );

    if (result.failed != 0)
    {
        return result;
    }


    /* ========================================================
     * TEST 2 - HIGH PRIORITY EVENT
     * ======================================================== */

    StateMachineEvent highEvent = {.id = 1, .priority = HIGH};

    status = StateMachinePushEvent(&sm, &highEvent);

    TestCheck(&result, status == STATE_MACHINE_OK, __LINE__);
    TestCheck(&result, sm.highEventQueue.count == 1, __LINE__);
    TestCheck(&result, sm.normalEventQueue.count == 0, __LINE__);
    TestCheck(&result, sm.lowEventQueue.count == 0, __LINE__);


    /* ========================================================
     * TEST 3 - NORMAL PRIORITY EVENT
     * ======================================================== */

    StateMachineEvent normalEvent = {.id = 2, .priority = NORMAL};

    status = StateMachinePushEvent(&sm, &normalEvent);

    TestCheck(&result, status == STATE_MACHINE_OK, __LINE__);
    TestCheck(&result, sm.highEventQueue.count == 1, __LINE__);
    TestCheck(&result, sm.normalEventQueue.count == 1, __LINE__);
    TestCheck(&result, sm.lowEventQueue.count == 0, __LINE__);


    /* ========================================================
     * TEST 4 - LOW PRIORITY EVENT
     * ======================================================== */

    StateMachineEvent lowEvent = {.id = 3, .priority = LOW};

    status = StateMachinePushEvent(&sm, &lowEvent);

    TestCheck(&result, status == STATE_MACHINE_OK, __LINE__);
    TestCheck(&result, sm.highEventQueue.count == 1, __LINE__);
    TestCheck(&result, sm.normalEventQueue.count == 1, __LINE__);
    TestCheck(&result, sm.lowEventQueue.count == 1, __LINE__);


    /* ========================================================
     * TEST 5 - VERIFY EVENT ROUTING
     * ======================================================== */

    EventQueueStatus queueStatus;

    queueStatus = EventQueuePop(&sm.highEventQueue, &receivedEvent);
    TestCheck(&result, queueStatus == EVENT_QUEUE_OK, __LINE__);
    if (queueStatus == EVENT_QUEUE_OK)
    {
        TestCheck(&result, receivedEvent.id == 1, __LINE__);
        TestCheck(&result, receivedEvent.priority == HIGH, __LINE__);
    }

    queueStatus = EventQueuePop(&sm.normalEventQueue, &receivedEvent);
    TestCheck(&result, queueStatus == EVENT_QUEUE_OK, __LINE__);
    if (queueStatus == EVENT_QUEUE_OK)
    {
        TestCheck(&result, receivedEvent.id == 2, __LINE__);
        TestCheck(&result, receivedEvent.priority == NORMAL, __LINE__);
    }

    queueStatus = EventQueuePop(&sm.lowEventQueue, &receivedEvent);
    TestCheck(&result, queueStatus == EVENT_QUEUE_OK, __LINE__);
    if (queueStatus == EVENT_QUEUE_OK)
    {
        TestCheck(&result, receivedEvent.id == 3, __LINE__);
        TestCheck(&result, receivedEvent.priority == LOW, __LINE__);
    }


    /* ========================================================
     * TEST 6 - NULL POINTER VALIDATION
     * ======================================================== */

    TestCheck(
        &result,
        StateMachinePushEvent(NULL, &highEvent) == STATE_MACHINE_INVALID_ARGUMENT,
        __LINE__
    );

    TestCheck(
        &result,
        StateMachinePushEvent(&sm, NULL) == STATE_MACHINE_INVALID_ARGUMENT,
        __LINE__
    );


    /* ========================================================
     * TEST 7 - INVALID EVENT PRIORITY
     * ======================================================== */

    StateMachineEvent invalidEvent = {.id = 99, .priority = 255};

    status = StateMachinePushEvent(&sm, &invalidEvent);

    TestCheck(
        &result,
        status == STATE_MACHINE_INVALID_PRIORITY,
        __LINE__
    );

    TestCheck(&result, sm.highEventQueue.count == 0, __LINE__);
    TestCheck(&result, sm.normalEventQueue.count == 0, __LINE__);
    TestCheck(&result, sm.lowEventQueue.count == 0, __LINE__);


    /* ========================================================
     * TEST 8 - FILL HIGH PRIORITY QUEUE
     * ======================================================== */

    for (uint8_t i = 0; i < MAX_EVENT_QUEUE; ++i)
    {
        StateMachineEvent ev = {.id = i, .priority = HIGH};

        status = StateMachinePushEvent(&sm, &ev);

        TestCheck(&result, status == STATE_MACHINE_OK, __LINE__);
    }

    TestCheck(
        &result,
        sm.highEventQueue.count == MAX_EVENT_QUEUE,
        __LINE__
    );


    /* ========================================================
     * TEST 9 - PUSH INTO FULL HIGH PRIORITY QUEUE
     * ======================================================== */

    StateMachineEvent extraEvent = {.id = 100, .priority = HIGH};

    status = StateMachinePushEvent(&sm, &extraEvent);

    TestCheck(
        &result,
        status == STATE_MACHINE_QUEUE_FULL,
        __LINE__
    );

    TestCheck(
        &result,
        sm.highEventQueue.count == MAX_EVENT_QUEUE,
        __LINE__
    );


    /* ========================================================
     * TEST 10 - CHECK PRIORITY QUEUE INDEPENDENCE
     * ======================================================== */

    StateMachineEvent extraNormalEvent = {.id = 101, .priority = NORMAL};
    StateMachineEvent extraLowEvent = {.id = 102, .priority = LOW};

    status = StateMachinePushEvent(&sm, &extraNormalEvent);
    TestCheck(&result, status == STATE_MACHINE_OK, __LINE__);

    status = StateMachinePushEvent(&sm, &extraLowEvent);
    TestCheck(&result, status == STATE_MACHINE_OK, __LINE__);

    TestCheck(&result, sm.highEventQueue.count == MAX_EVENT_QUEUE, __LINE__);
    TestCheck(&result, sm.normalEventQueue.count == 1, __LINE__);
    TestCheck(&result, sm.lowEventQueue.count == 1, __LINE__);


    /* ========================================================
     * TEST 11 - VERIFY FIFO ORDER IN HIGH PRIORITY QUEUE
     * ======================================================== */

    for (uint8_t i = 0; i < MAX_EVENT_QUEUE; ++i)
    {
        queueStatus = EventQueuePop(&sm.highEventQueue, &receivedEvent);

        TestCheck(&result, queueStatus == EVENT_QUEUE_OK, __LINE__);

        if (queueStatus == EVENT_QUEUE_OK)
        {
            TestCheck(&result, receivedEvent.id == i, __LINE__);
        }
    }

    TestCheck(&result, sm.highEventQueue.count == 0, __LINE__);


    return result;
}
