
#ifndef TEST_MACHINE_FSM_H
#define TEST_MACHINE_FSM_H

#include <stdint.h>
#include "machine_fsm.h"

/* ============================================================
 * TEST RESULT
 * ============================================================ */

typedef struct TestResult
{
    uint32_t passed;
    uint32_t failed;
    uint32_t firstFailedLine;
} TestResult;


/* ============================================================
 * TEST FUNCTIONS
 * ============================================================ */

TestResult TestEventQueue(void);

TestResult TestStateMachineInitialize(void);

TestResult TestStateMachinePushEvent(void);

TestResult TestStateMachinePopEvent(void);

TestResult TestStateMachineAddTransition(void);

TestResult TestStateMachineFindTransition(void);

TestResult TestStateMachineStart(void);

#endif