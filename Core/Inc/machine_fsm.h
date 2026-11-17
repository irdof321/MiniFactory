#ifndef MACHINE_FSM_H
#define MACHINE_FSM_H

/* ============================================================
 * INCLUDES
 * ============================================================ */

#include <stddef.h>
#include <stdint.h>
#include <sys/queue.h>


/* ============================================================
 * LIBRARY CONSTANTS
 * ============================================================ */

#define MAX_NAME_LEN 16
#define MAX_EVENT_QUEUE 20


/* ============================================================
 * FORWARD DECLARATIONS
 * ============================================================ */

/* Allow callbacks to reference StateMachine before its full definition. */
typedef struct StateMachine StateMachine;


/* ============================================================
 * EVENT TYPES
 * ============================================================ */

/* Queue selection: higher priorities are serviced first. */
typedef enum {
    HIGH,
    NORMAL,
    LOW
}EventPriority ;

/* Application-defined event identifier and its queue priority. */
typedef struct StateMachineEvent
{
    uint8_t id;
    EventPriority priority;

} StateMachineEvent;


/* ============================================================
 * STATE TYPES AND CALLBACKS
 * ============================================================ */

/* Repeated forward declaration preserved from the supplied header. */
typedef struct StateMachine StateMachine;

/* A state callback receives the current FSM instance and an optional event. */
typedef void (*StateMachineCallback)(
    StateMachine *sm,
    const StateMachineEvent *event
);

/* Immutable state description referenced by a machine definition. */
typedef struct StateMachineState
{
    uint8_t id;
    char name[MAX_NAME_LEN];

    StateMachineCallback onEnter;
    StateMachineCallback onUpdate;
    StateMachineCallback onExit;

} StateMachineState;


/* ============================================================
 * TRANSITION TYPES
 * ============================================================ */

/* A transition is selected by the pair (currentStateId, eventId). */
typedef struct StateMachineTransition
{
    uint8_t currentStateId;
    uint8_t eventId;
    uint8_t nextStateId;
} StateMachineTransition;

/* The caller owns the rule storage; count tracks the populated entries. */
typedef struct StateMachineTransitionMap
{
    StateMachineTransition *rules;
    size_t count;
    size_t capacity;

} StateMachineTransitionMap;


/* ============================================================
 * EVENT QUEUE TYPES
 * ============================================================ */

/* Event queue operation results. */
typedef enum
{
    EVENT_QUEUE_OK,
    EVENT_QUEUE_CAPACITY_FULL,
    EVENT_QUEUE_NOT_VALID_ARGUMENT,
    EVENT_QUEUE_EMPTY

} EventQueueStatus;

/* Fixed-size FIFO ring buffer used by each priority level. */
typedef struct EventQueue
{
    StateMachineEvent stateMachineEvents[MAX_EVENT_QUEUE];
    uint8_t head;
    uint8_t tail;
    uint8_t count;
} EventQueue;


/* ============================================================
 * STATE MACHINE DEFINITION AND RUNTIME INSTANCE
 * ============================================================ */

/* Read-only configuration shared by one or more FSM instances. */
typedef struct StateMachineDefinition
{
    const StateMachineState *states;
    size_t stateCount;

    const StateMachineTransitionMap *transitionMap;

    uint8_t initialStateId;
} StateMachineDefinition;

/* Execution lifecycle, independent of application state IDs. */
typedef enum
{
    STATE_MACHINE_LIFECYCLE_INITIALIZED,
    STATE_MACHINE_LIFECYCLE_RUNNING,
    STATE_MACHINE_LIFECYCLE_PAUSED,
    STATE_MACHINE_LIFECYCLE_STOPPED
} StateMachineLifecycle;

/* Per-instance state and independent priority queues. */
struct StateMachine
{
    const StateMachineDefinition *definition;

    StateMachineLifecycle lifecycle;

    uint8_t currentStateId;

    EventQueue highEventQueue;
    EventQueue normalEventQueue;
    EventQueue lowEventQueue;
};


/* ============================================================
 * OPERATION STATUS TYPES
 * ============================================================ */

/* State machine initialization, runtime and event operation results. */
typedef enum
{
    STATE_MACHINE_INIT_OK,
    STATE_MACHINE_OK,
    STATE_MACHINE_INIT_ERROR,
    STATE_MACHINE_INVALID_ARGUMENT,
    STATE_MACHINE_QUEUE_FULL,
    STATE_MACHINE_INVALID_PRIORITY,
    STATE_MACHINE_NO_EVENT,
    STATE_MACHINE_ERROR_IN_QUEUE,
    STATE_MACHINE_STATE_NOT_FOUND,
    STATE_MACHINE_INVALID_LIFECYCLE

} StateMachineStatus;

/* Transition insertion results. */
typedef enum
{
    STATE_MACHINE_TRANSITION_ADDED,
    STATE_MACHINE_TRANSITION_CAPACITY_FULL,
    STATE_MACHINE_TRANSITION_INVALID_ARGUMENT,
    STATE_MACHINE_TRANSITION_ALREADY_EXISTS

} StateMachineTransitionStatus;

/* Transition lookup results. */
typedef enum
{
    STATE_MACHINE_TRANSITION_FOUND,
    STATE_MACHINE_TRANSITION_NOT_FOUND,
    STATE_MACHINE_TRANSITION_FIND_INVALID_ARGUMENT
} StateMachineFindStatus;


/* ============================================================
 * EVENT QUEUE API
 * ============================================================ */

/* Initialize a queue to an empty state. */
EventQueueStatus EventQueueInit(EventQueue *queue);

/* Enqueue an event by value; returns a capacity error if full. */
EventQueueStatus EventQueuePush(
    EventQueue *queue,
    const StateMachineEvent *event
);

/* Remove and return the oldest queued event through the output pointer. */
EventQueueStatus EventQueuePop(
    EventQueue *queue,
    StateMachineEvent *event
);


/* ============================================================
 * TRANSITION MAP API
 * ============================================================ */

/* Add a rule to the transition map, rejecting duplicate state/event pairs. */
StateMachineTransitionStatus StateMachineAddTransition(
    StateMachineTransitionMap *map,
    const StateMachineTransition *rule
);

/* Find the destination state ID for a state/event pair. */
StateMachineFindStatus StateMachineFindTransition(
    const StateMachineTransitionMap *map,
    uint8_t currentStateId,
    uint8_t eventId,
    uint8_t *nextStateId
);


/* ============================================================
 * STATE MACHINE LIFECYCLE API
 * ============================================================ */

/* Initialize an instance using a shared FSM definition. */
StateMachineStatus StateMachineInitialize(
    StateMachine *sm,
    const StateMachineDefinition *definition
);

/* Enter the initial state and switch the instance to RUNNING. */
StateMachineStatus StateMachineStart(StateMachine *sm);

/* Process at most one queued event and invoke the active state's update. */
StateMachineStatus StateMachineUpdate(StateMachine *sm);


/* ============================================================
 * STATE MACHINE EVENT API
 * ============================================================ */

/* Enqueue an event in the matching priority queue. */
StateMachineStatus StateMachinePushEvent(
    StateMachine *sm,
    const StateMachineEvent *event
);

/* Retrieve one event, ordered by priority and then FIFO. */
StateMachineStatus StateMachinePopEvent(
    StateMachine *sm,
    StateMachineEvent *event
);




#endif
