
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
 * SHARED TEST STATE MACHINE DEFINITION
 * ============================================================ */

static const StateMachineState testStates[] = {
    {.id = 1, .name = "IDLE"},
    {.id = 2, .name = "RUNNING"}
};

static StateMachineTransition testRules[2] = {0};

static StateMachineTransitionMap testTransitionMap = {
    .rules = testRules,
    .count = 0,
    .capacity = 2
};

static const StateMachineDefinition testDefinition = {
    .states = testStates,
    .stateCount = 2,
    .transitionMap = &testTransitionMap,
    .initialStateId = 1
};


/* ============================================================
 * STATE MACHINE START TEST CALLBACKS
 * ============================================================ */

static uint32_t startEnterCount = 0;
static uint32_t startUpdateCount = 0;
static uint32_t startExitCount = 0;

static StateMachine *startCallbackInstance = NULL;
static bool startCallbackEventWasNull = false;

static void TestStartOnEnter(
    StateMachine *sm,
    const StateMachineEvent *event
)
{
    startEnterCount++;
    startCallbackInstance = sm;
    startCallbackEventWasNull = (event == NULL);
}

static void TestStartOnUpdate(
    StateMachine *sm,
    const StateMachineEvent *event
)
{
    (void)sm;
    (void)event;
    startUpdateCount++;
}

static void TestStartOnExit(
    StateMachine *sm,
    const StateMachineEvent *event
)
{
    (void)sm;
    (void)event;
    startExitCount++;
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
    StateMachine sm = {0};
    StateMachineStatus status;

    /* ========================================================
     * TEST 1 - NULL POINTER VALIDATION
     * ======================================================== */

    TestCheck(
        &result,
        StateMachineInitialize(NULL, &testDefinition)
            == STATE_MACHINE_INVALID_ARGUMENT,
        __LINE__
    );

    TestCheck(
        &result,
        StateMachineInitialize(&sm, NULL)
            == STATE_MACHINE_INVALID_ARGUMENT,
        __LINE__
    );

    /* ========================================================
     * TEST 2 - VALID STATE MACHINE INITIALIZATION
     * ======================================================== */

    status = StateMachineInitialize(&sm, &testDefinition);

    TestCheck(
        &result,
        status == STATE_MACHINE_INIT_OK,
        __LINE__
    );

    if (status != STATE_MACHINE_INIT_OK)
    {
        return result;
    }

    TestCheck(&result, sm.definition == &testDefinition, __LINE__);
    TestCheck(&result, sm.currentStateId == 1, __LINE__);

    /* Keep the existing queue initialization checks here. */

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
        StateMachineInitialize(&sm, &testDefinition) == STATE_MACHINE_INIT_OK,
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



/* ============================================================
 * STATE MACHINE POP EVENT TESTS
 * ============================================================ */

TestResult TestStateMachinePopEvent(void)
{
    TestResult result = {0};

    StateMachine sm;
    StateMachineStatus status;
    StateMachineEvent receivedEvent = {0};


    /* ========================================================
     * TEST 1 - INITIALIZE STATE MACHINE
     * ======================================================== */

    TestCheck(
        &result,
        StateMachineInitialize(&sm, &testDefinition) == STATE_MACHINE_INIT_OK,
        __LINE__
    );

    if (result.failed != 0)
    {
        return result;
    }


    /* ========================================================
     * TEST 2 - INSERT EVENTS WITH MIXED PRIORITIES
     * ======================================================== */

    StateMachineEvent events[] = {
        {.id = 1, .priority = LOW},
        {.id = 2, .priority = HIGH},
        {.id = 3, .priority = NORMAL},
        {.id = 4, .priority = HIGH},
        {.id = 5, .priority = LOW}
    };

    for (uint8_t i = 0; i < 5; ++i)
    {
        status = StateMachinePushEvent(&sm, &events[i]);

        TestCheck(
            &result,
            status == STATE_MACHINE_OK,
            __LINE__
        );
    }


    /* ========================================================
     * TEST 3 - VERIFY PRIORITY QUEUE COUNTS
     * ======================================================== */

    TestCheck(&result, sm.highEventQueue.count == 2, __LINE__);
    TestCheck(&result, sm.normalEventQueue.count == 1, __LINE__);
    TestCheck(&result, sm.lowEventQueue.count == 2, __LINE__);


    /* ========================================================
     * TEST 4 - VERIFY PRIORITY AND FIFO ORDER
     * ======================================================== */

    uint8_t expectedIds[] = {2, 4, 3, 1, 5};

    for (uint8_t i = 0; i < 5; ++i)
    {
        status = StateMachinePopEvent(&sm, &receivedEvent);

        TestCheck(
            &result,
            status == STATE_MACHINE_OK,
            __LINE__
        );

        if (status == STATE_MACHINE_OK)
        {
            TestCheck(
                &result,
                receivedEvent.id == expectedIds[i],
                __LINE__
            );
        }
    }


    /* ========================================================
     * TEST 5 - VERIFY ALL QUEUES ARE EMPTY
     * ======================================================== */

    TestCheck(&result, sm.highEventQueue.count == 0, __LINE__);
    TestCheck(&result, sm.normalEventQueue.count == 0, __LINE__);
    TestCheck(&result, sm.lowEventQueue.count == 0, __LINE__);


    /* ========================================================
     * TEST 6 - POP WHEN ALL QUEUES ARE EMPTY
     * ======================================================== */

    status = StateMachinePopEvent(&sm, &receivedEvent);

    TestCheck(
        &result,
        status == STATE_MACHINE_NO_EVENT,
        __LINE__
    );


    /* ========================================================
     * TEST 7 - NULL POINTER VALIDATION
     * ======================================================== */

    TestCheck(
        &result,
        StateMachinePopEvent(NULL, &receivedEvent)
            == STATE_MACHINE_INVALID_ARGUMENT,
        __LINE__
    );

    TestCheck(
        &result,
        StateMachinePopEvent(&sm, NULL)
            == STATE_MACHINE_INVALID_ARGUMENT,
        __LINE__
    );

    return result;
}



/* ============================================================
 * STATE MACHINE TRANSITION TESTS
 * ============================================================ */

TestResult TestStateMachineAddTransition(void)
{
    TestResult result = {0};
    StateMachineTransitionStatus status;

    StateMachineTransition rules[3] = {0};

    StateMachineTransitionMap map = {
        .rules = rules,
        .count = 0,
        .capacity = 3
    };


    /* ========================================================
     * TEST 1 - INITIAL TRANSITION MAP STATE
     * ======================================================== */

    TestCheck(&result, map.count == 0, __LINE__);
    TestCheck(&result, map.capacity == 3, __LINE__);


    /* ========================================================
     * TEST 2 - ADD FIRST TRANSITION
     * ======================================================== */

    StateMachineTransition rule1 = {
        .currentStateId = 1,
        .eventId = 10,
        .nextStateId = 2
    };

    status = StateMachineAddTransition(&map, &rule1);

    TestCheck(&result,
        status == STATE_MACHINE_TRANSITION_ADDED,
        __LINE__);

    TestCheck(&result, map.count == 1, __LINE__);
    TestCheck(&result, map.rules[0].currentStateId == 1, __LINE__);
    TestCheck(&result, map.rules[0].eventId == 10, __LINE__);
    TestCheck(&result, map.rules[0].nextStateId == 2, __LINE__);


    /* ========================================================
     * TEST 3 - REJECT DUPLICATE TRANSITION
     * ======================================================== */

    StateMachineTransition duplicateRule = {
        .currentStateId = 1,
        .eventId = 10,
        .nextStateId = 3
    };

    status = StateMachineAddTransition(&map, &duplicateRule);

    TestCheck(&result,
        status == STATE_MACHINE_TRANSITION_ALREADY_EXISTS,
        __LINE__);

    TestCheck(&result, map.count == 1, __LINE__);


    /* ========================================================
     * TEST 4 - SAME EVENT FROM DIFFERENT STATE
     * ======================================================== */

    StateMachineTransition rule2 = {
        .currentStateId = 2,
        .eventId = 10,
        .nextStateId = 1
    };

    status = StateMachineAddTransition(&map, &rule2);

    TestCheck(&result,
        status == STATE_MACHINE_TRANSITION_ADDED,
        __LINE__);

    TestCheck(&result, map.count == 2, __LINE__);


    /* ========================================================
     * TEST 5 - SAME STATE WITH DIFFERENT EVENT
     * ======================================================== */

    StateMachineTransition rule3 = {
        .currentStateId = 1,
        .eventId = 11,
        .nextStateId = 3
    };

    status = StateMachineAddTransition(&map, &rule3);

    TestCheck(&result,
        status == STATE_MACHINE_TRANSITION_ADDED,
        __LINE__);

    TestCheck(&result, map.count == 3, __LINE__);


    /* ========================================================
     * TEST 6 - REJECT TRANSITION WHEN MAP IS FULL
     * ======================================================== */

    StateMachineTransition extraRule = {
        .currentStateId = 3,
        .eventId = 12,
        .nextStateId = 1
    };

    status = StateMachineAddTransition(&map, &extraRule);

    TestCheck(&result,
        status == STATE_MACHINE_TRANSITION_CAPACITY_FULL,
        __LINE__);

    TestCheck(&result, map.count == 3, __LINE__);


    /* ========================================================
     * TEST 7 - NULL POINTER VALIDATION
     * ======================================================== */

    TestCheck(&result,
        StateMachineAddTransition(NULL, &rule1)
            == STATE_MACHINE_TRANSITION_INVALID_ARGUMENT,
        __LINE__);

    TestCheck(&result,
        StateMachineAddTransition(&map, NULL)
            == STATE_MACHINE_TRANSITION_INVALID_ARGUMENT,
        __LINE__);

    StateMachineTransitionMap invalidMap = {
        .rules = NULL,
        .count = 0,
        .capacity = 3
    };

    TestCheck(&result,
        StateMachineAddTransition(&invalidMap, &rule1)
            == STATE_MACHINE_TRANSITION_INVALID_ARGUMENT,
        __LINE__);


    /* ========================================================
     * TEST 8 - VERIFY TRANSITIONS ARE COPIED BY VALUE
     * ======================================================== */

    rule1.nextStateId = 99;

    TestCheck(&result,
        map.rules[0].nextStateId == 2,
        __LINE__);


    return result;
}



/* ============================================================
 * STATE MACHINE FIND TRANSITION TESTS
 * ============================================================ */

TestResult TestStateMachineFindTransition(void)
{
    TestResult result = {0};
    StateMachineFindStatus status;
    uint8_t nextStateId = 0;

    StateMachineTransition rules[3] = {0};

    StateMachineTransitionMap map = {
        .rules = rules,
        .count = 0,
        .capacity = 3
    };


    /* ========================================================
     * TEST 1 - INITIALIZE TRANSITION MAP
     * ======================================================== */

    StateMachineTransition rule1 = {
        .currentStateId = 1,
        .eventId = 10,
        .nextStateId = 2
    };

    StateMachineTransition rule2 = {
        .currentStateId = 2,
        .eventId = 10,
        .nextStateId = 3
    };

    TestCheck(
        &result,
        StateMachineAddTransition(&map, &rule1)
            == STATE_MACHINE_TRANSITION_ADDED,
        __LINE__
    );

    TestCheck(
        &result,
        StateMachineAddTransition(&map, &rule2)
            == STATE_MACHINE_TRANSITION_ADDED,
        __LINE__
    );

    if (result.failed != 0)
    {
        return result;
    }


    /* ========================================================
     * TEST 2 - FIND FIRST TRANSITION
     * ======================================================== */

    nextStateId = 0;

    status = StateMachineFindTransition(
        &map, 1, 10, &nextStateId
    );

    TestCheck(
        &result,
        status == STATE_MACHINE_TRANSITION_FOUND,
        __LINE__
    );

    TestCheck(&result, nextStateId == 2, __LINE__);


    /* ========================================================
     * TEST 3 - FIND SECOND TRANSITION
     * ======================================================== */

    nextStateId = 0;

    status = StateMachineFindTransition(
        &map, 2, 10, &nextStateId
    );

    TestCheck(
        &result,
        status == STATE_MACHINE_TRANSITION_FOUND,
        __LINE__
    );

    TestCheck(&result, nextStateId == 3, __LINE__);


    /* ========================================================
     * TEST 4 - UNKNOWN EVENT ID
     * ======================================================== */

    nextStateId = 99;

    status = StateMachineFindTransition(
        &map, 1, 11, &nextStateId
    );

    TestCheck(
        &result,
        status == STATE_MACHINE_TRANSITION_NOT_FOUND,
        __LINE__
    );

    TestCheck(&result, nextStateId == 99, __LINE__);


    /* ========================================================
     * TEST 5 - UNKNOWN STATE ID
     * ======================================================== */

    nextStateId = 99;

    status = StateMachineFindTransition(
        &map, 5, 10, &nextStateId
    );

    TestCheck(
        &result,
        status == STATE_MACHINE_TRANSITION_NOT_FOUND,
        __LINE__
    );

    TestCheck(&result, nextStateId == 99, __LINE__);


    /* ========================================================
     * TEST 6 - EMPTY TRANSITION MAP
     * ======================================================== */

    StateMachineTransitionMap emptyMap = {
        .rules = rules,
        .count = 0,
        .capacity = 3
    };

    nextStateId = 99;

    status = StateMachineFindTransition(
        &emptyMap, 1, 10, &nextStateId
    );

    TestCheck(
        &result,
        status == STATE_MACHINE_TRANSITION_NOT_FOUND,
        __LINE__
    );

    TestCheck(&result, nextStateId == 99, __LINE__);


    /* ========================================================
     * TEST 7 - NULL POINTER VALIDATION
     * ======================================================== */

    TestCheck(
        &result,
        StateMachineFindTransition(NULL, 1, 10, &nextStateId)
            == STATE_MACHINE_TRANSITION_FIND_INVALID_ARGUMENT,
        __LINE__
    );

    TestCheck(
        &result,
        StateMachineFindTransition(&map, 1, 10, NULL)
            == STATE_MACHINE_TRANSITION_FIND_INVALID_ARGUMENT,
        __LINE__
    );


    /* ========================================================
     * TEST 8 - NULL TRANSITION ARRAY
     * ======================================================== */

    StateMachineTransitionMap invalidMap = {
        .rules = NULL,
        .count = 0,
        .capacity = 3
    };

    TestCheck(
        &result,
        StateMachineFindTransition(&invalidMap, 1, 10, &nextStateId)
            == STATE_MACHINE_TRANSITION_FIND_INVALID_ARGUMENT,
        __LINE__
    );


    /* ========================================================
     * TEST 9 - INVALID TRANSITION MAP COUNT
     * ======================================================== */

    StateMachineTransitionMap corruptedMap = {
        .rules = rules,
        .count = 4,
        .capacity = 3
    };

    nextStateId = 99;

    status = StateMachineFindTransition(
        &corruptedMap, 1, 10, &nextStateId
    );

    TestCheck(
        &result,
        status == STATE_MACHINE_TRANSITION_FIND_INVALID_ARGUMENT,
        __LINE__
    );

    TestCheck(&result, nextStateId == 99, __LINE__);


    return result;
}


/* ============================================================
 * STATE MACHINE START TESTS
 * ============================================================ */

TestResult TestStateMachineStart(void)
{
    TestResult result = {0};
    StateMachineStatus status;

    startEnterCount = 0;
    startUpdateCount = 0;
    startExitCount = 0;
    startCallbackInstance = NULL;
    startCallbackEventWasNull = false;

    const StateMachineState states[] = {
        {
            .id = 1,
            .name = "IDLE",
            .onEnter = TestStartOnEnter,
            .onUpdate = TestStartOnUpdate,
            .onExit = TestStartOnExit
        },
        {
            .id = 2,
            .name = "RUNNING"
        }
    };

    StateMachineTransition rules[1] = {0};

    StateMachineTransitionMap map = {
        .rules = rules,
        .count = 0,
        .capacity = 1
    };

    const StateMachineDefinition definition = {
        .states = states,
        .stateCount = 2,
        .transitionMap = &map,
        .initialStateId = 1
    };

    StateMachine sm = {0};

    /* ========================================================
     * TEST 1 - INITIALIZE STATE MACHINE
     * ======================================================== */

    status = StateMachineInitialize(&sm, &definition);

    TestCheck(&result,
        status == STATE_MACHINE_INIT_OK, __LINE__);

    if (status != STATE_MACHINE_INIT_OK)
    {
        return result;
    }

    TestCheck(&result,
        sm.lifecycle == STATE_MACHINE_LIFECYCLE_INITIALIZED,
        __LINE__);

    TestCheck(&result, sm.currentStateId == 1, __LINE__);


    /* ========================================================
     * TEST 2 - QUEUE EVENT BEFORE START
     * ======================================================== */

    StateMachineEvent event = {
        .id = 10,
        .priority = HIGH
    };

    TestCheck(&result,
        StateMachinePushEvent(&sm, &event) == STATE_MACHINE_OK,
        __LINE__);


    /* ========================================================
     * TEST 3 - START STATE MACHINE
     * ======================================================== */

    status = StateMachineStart(&sm);

    TestCheck(&result, status == STATE_MACHINE_OK, __LINE__);

    TestCheck(&result,
        sm.lifecycle == STATE_MACHINE_LIFECYCLE_RUNNING,
        __LINE__);

    TestCheck(&result, startEnterCount == 1, __LINE__);
    TestCheck(&result, startCallbackInstance == &sm, __LINE__);
    TestCheck(&result, startCallbackEventWasNull, __LINE__);

    TestCheck(&result, startUpdateCount == 0, __LINE__);
    TestCheck(&result, startExitCount == 0, __LINE__);

    TestCheck(&result,
        sm.highEventQueue.count == 1,
        __LINE__);


    /* ========================================================
     * TEST 4 - REJECT SECOND START
     * ======================================================== */

    status = StateMachineStart(&sm);

    TestCheck(&result,
        status == STATE_MACHINE_INVALID_LIFECYCLE,
        __LINE__);

    TestCheck(&result, startEnterCount == 1, __LINE__);


    /* ========================================================
     * TEST 5 - NULL POINTER VALIDATION
     * ======================================================== */

    TestCheck(&result,
        StateMachineStart(NULL) == STATE_MACHINE_INVALID_ARGUMENT,
        __LINE__);

    StateMachine invalidSm = {0};

    TestCheck(&result,
        StateMachineStart(&invalidSm) == STATE_MACHINE_INVALID_ARGUMENT,
        __LINE__);


    /* ========================================================
     * TEST 6 - INVALID LIFECYCLE
     * ======================================================== */

    sm.lifecycle = STATE_MACHINE_LIFECYCLE_PAUSED;

    TestCheck(&result,
        StateMachineStart(&sm) == STATE_MACHINE_INVALID_LIFECYCLE,
        __LINE__);

    TestCheck(&result, startEnterCount == 1, __LINE__);

    sm.lifecycle = STATE_MACHINE_LIFECYCLE_STOPPED;

    TestCheck(&result,
        StateMachineStart(&sm) == STATE_MACHINE_INVALID_LIFECYCLE,
        __LINE__);


    /* ========================================================
     * TEST 7 - STATE WITHOUT CALLBACK
     * ======================================================== */

    const StateMachineDefinition silentDefinition = {
        .states = states,
        .stateCount = 2,
        .transitionMap = &map,
        .initialStateId = 2
    };

    StateMachine silentSm = {0};

    status = StateMachineInitialize(&silentSm, &silentDefinition);

    TestCheck(&result,
        status == STATE_MACHINE_INIT_OK,
        __LINE__);

    if (status == STATE_MACHINE_INIT_OK)
    {
        status = StateMachineStart(&silentSm);

        TestCheck(&result,
            status == STATE_MACHINE_OK,
            __LINE__);

        TestCheck(&result,
            silentSm.lifecycle == STATE_MACHINE_LIFECYCLE_RUNNING,
            __LINE__);

        TestCheck(&result, startEnterCount == 1, __LINE__);
    }


    /* ========================================================
     * TEST 8 - INITIAL STATE NOT FOUND
     * ======================================================== */

    StateMachineDefinition missingDefinition = definition;
    missingDefinition.initialStateId = 99;

    StateMachine missingSm = {0};
    missingSm.definition = &missingDefinition;
    missingSm.currentStateId = 99;
    missingSm.lifecycle = STATE_MACHINE_LIFECYCLE_INITIALIZED;

    status = StateMachineStart(&missingSm);

    TestCheck(&result,
        status == STATE_MACHINE_STATE_NOT_FOUND,
        __LINE__);

    TestCheck(&result,
        missingSm.lifecycle == STATE_MACHINE_LIFECYCLE_INITIALIZED,
        __LINE__);


    /* ========================================================
     * TEST 9 - EMPTY STATE DEFINITION
     * ======================================================== */

    StateMachineDefinition emptyDefinition = definition;
    emptyDefinition.stateCount = 0;

    StateMachine emptySm = {0};
    emptySm.definition = &emptyDefinition;
    emptySm.lifecycle = STATE_MACHINE_LIFECYCLE_INITIALIZED;

    TestCheck(&result,
        StateMachineStart(&emptySm) == STATE_MACHINE_INVALID_ARGUMENT,
        __LINE__);


    return result;
}
