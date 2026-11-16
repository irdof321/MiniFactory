
#ifndef MACHINE_FSM_H
#define MACHINE_FSM_H

/* ============================================================
 * INCLUDES
 * ============================================================ */

#include <stddef.h>
#include <stdint.h>
#include <sys/queue.h>


/* ============================================================
 * CONSTANTS
 * ============================================================ */

#define MAX_NAME_LEN 16
#define MAX_EVENT_QUEUE 20


/* ============================================================
 * FORWARD DECLARATIONS
 * ============================================================ */

typedef struct StateMachine StateMachine;


/* ============================================================
 * EVENT
 * ============================================================ */
typedef enum {
    HIGH,
    NORMAL,
    LOW
}EventPriority ;

typedef struct StateMachineEvent
{
    uint8_t id;
    EventPriority priority;

} StateMachineEvent;


/* ============================================================
 * STATE
 * ============================================================ */

typedef struct StateMachineState
{
    uint8_t id;
    char name[MAX_NAME_LEN];

    void (*onEnter)(const StateMachineEvent *event, StateMachine *sm);
    void (*onUpdate)(const StateMachineEvent *event, StateMachine *sm);
    void (*onExit)(const StateMachineEvent *event, StateMachine *sm);

} StateMachineState;


/* ============================================================
 * TRANSITIONS
 * ============================================================ */

typedef struct StateMachineTransition
{
    uint8_t currentStateId;
    uint8_t eventId;
    uint8_t nextStateId;
} StateMachineTransition;


/* ============================================================
 * TRANSITION STATUS
 * ============================================================ */

typedef enum
{
    STATE_MACHINE_TRANSITION_ADDED,
    STATE_MACHINE_TRANSITION_CAPACITY_FULL,
    STATE_MACHINE_TRANSITION_INVALID_ARGUMENT,
    STATE_MACHINE_TRANSITION_ALREADY_EXISTS

} StateMachineTransitionStatus;


/* ============================================================
 * TRANSITION MAP
 * ============================================================ */

typedef struct StateMachineTransitionMap
{
    StateMachineTransition *rules;
    size_t count;
    size_t capacity;

} StateMachineTransitionMap;


/* ============================================================
 * EVENT QUEUE STATUS
 * ============================================================ */

typedef enum
{
    EVENT_QUEUE_OK,
    EVENT_QUEUE_CAPACITY_FULL,
    EVENT_QUEUE_NOT_VALID_ARGUMENT,
    EVENT_QUEUE_EMPTY

} EventQueueStatus;


/* ============================================================
 * EVENT QUEUE
 * ============================================================ */

typedef struct EventQueue
{
    StateMachineEvent stateMachineEvents[MAX_EVENT_QUEUE];
    uint8_t head;
    uint8_t tail;
    uint8_t count;
} EventQueue;


/* ============================================================
 * EVENT QUEUE FUNCTIONS
 * ============================================================ */

EventQueueStatus EventQueuePush(
    EventQueue *queue,
    const StateMachineEvent *event
);

EventQueueStatus EventQueuePop(
    EventQueue *queue,
    StateMachineEvent *event
);

EventQueueStatus EventQueueInit(EventQueue *queue);


/* ============================================================
 * STATE MACHINE STATUS
 * ============================================================ */

typedef enum
{
    STATE_MACHINE_INIT_OK,
    STATE_MACHINE_OK,
    STATE_MACHINE_INIT_ERROR,
    STATE_MACHINE_INVALID_ARGUMENT,
    STATE_MACHINE_QUEUE_FULL,
    STATE_MACHINE_INVALID_PRIORITY,
    STATE_MACHINE_NO_EVENT,
    STATE_MACHINE_ERROR_IN_QUEUE

} StateMachineStatus;


/* ============================================================
 * STATE MACHINE
 * ============================================================ */

struct StateMachine
{
    struct EventQueue highEventQueue;
    struct EventQueue normalEventQueue;
    struct EventQueue lowEventQueue;
};


/* ============================================================
 * STATE MACHINE FUNCTIONS
 * ============================================================ */

StateMachineTransitionStatus StateMachineAddTransition(
    StateMachineTransitionMap *map,
    const StateMachineTransition *rule
);

StateMachineStatus StateMachineInitialize(
    StateMachine *sm
);


StateMachineStatus StateMachinePushEvent(
    StateMachine *sm,
    const StateMachineEvent *event
);

StateMachineStatus StateMachinePopEvent(
    StateMachine *sm,
    StateMachineEvent *event
);


typedef enum
{
    STATE_MACHINE_TRANSITION_FOUND,
    STATE_MACHINE_TRANSITION_NOT_FOUND,
    STATE_MACHINE_TRANSITION_FIND_INVALID_ARGUMENT
} StateMachineFindStatus;

StateMachineFindStatus StateMachineFindTransition(
    const StateMachineTransitionMap *map,
    uint8_t currentStateId,
    uint8_t eventId,
    uint8_t *nextStateId
);



#endif
