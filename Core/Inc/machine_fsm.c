#include "machine_fsm.h"
#include <stddef.h>
#include <stdint.h>

/* ============================================================
 * PRIVATE STATE LOOKUP HELPER
 * ============================================================ */

/* Locate a state definition by ID. Returns NULL when no match exists. */
static const StateMachineState *StateMachineFindState(
    const StateMachineDefinition *definition,
    uint8_t stateId
)
{
    if (definition == NULL || definition->states == NULL) {
        return NULL;
    }

    for (size_t i = 0; i < definition->stateCount; i++) {
        if (stateId == definition->states[i].id) {
            return &definition->states[i];
        }
    }

    return NULL;
}

/* Second identical helper definition retained unchanged from the supplied source. */
static const StateMachineState *StateMachineFindState(
    const StateMachineDefinition *definition,
    uint8_t stateId
)
{
    if (definition == NULL || definition->states == NULL) {
        return NULL;
    }

    for (size_t i = 0; i < definition->stateCount; i++) {
        if (stateId == definition->states[i].id) {
            return &definition->states[i];
        }
    }

    return NULL;
}

/* ============================================================
 * EVENT QUEUE OPERATIONS
 * ============================================================ */

/* Reset the indices and element count of a fixed-capacity event queue. */
EventQueueStatus EventQueueInit(EventQueue *queue)
{
    if(queue == NULL)
    {
        return EVENT_QUEUE_NOT_VALID_ARGUMENT;
    }

    queue->head = 0U;
    queue->tail = 0U;
    queue->count = 0U;

    return EVENT_QUEUE_OK;
}

/* Copy an event into the queue using FIFO ring-buffer ordering. */
EventQueueStatus EventQueuePush(
    EventQueue *queue,
    const StateMachineEvent *event
)
{
    if(event == NULL || queue == NULL)
    {
        return EVENT_QUEUE_NOT_VALID_ARGUMENT;
    }
    if(queue->count >= MAX_EVENT_QUEUE)
    {
        return EVENT_QUEUE_CAPACITY_FULL;
    }

    queue->stateMachineEvents[queue->tail] = *event;
    queue->count++;
    queue->tail = (queue->tail +1)%MAX_EVENT_QUEUE;

    return EVENT_QUEUE_OK;
}

/* Remove the oldest event from the queue and copy it to the output. */
EventQueueStatus EventQueuePop(
    EventQueue *queue,
    StateMachineEvent *event
)
{
    if(event == NULL || queue == NULL)
    {
        return EVENT_QUEUE_NOT_VALID_ARGUMENT;
    }
    if(queue->count == 0)
    {
        return EVENT_QUEUE_EMPTY;
    }
    *event = queue->stateMachineEvents[queue->head];
    queue->head = (queue->head + 1U) % MAX_EVENT_QUEUE;
    queue->count--;
   return EVENT_QUEUE_OK;
}

/* ============================================================
 * TRANSITION MAP OPERATIONS
 * ============================================================ */

/* Append a unique state/event transition to the caller-owned rule table. */
StateMachineTransitionStatus StateMachineAddTransition(
    StateMachineTransitionMap *map,
    const StateMachineTransition *rule
)
{
    if (map == NULL || rule == NULL) {
        return STATE_MACHINE_TRANSITION_INVALID_ARGUMENT;
    }
    if (map->rules == NULL)
    {
        return STATE_MACHINE_TRANSITION_INVALID_ARGUMENT;
    }
    if (map->count > map->capacity)
    {
        return STATE_MACHINE_TRANSITION_INVALID_ARGUMENT;
    }
    for (size_t i = 0; i < map->count; i++) {
        if (map->rules[i].currentStateId == rule->currentStateId &&
            map->rules[i].eventId == rule->eventId)
        {
            return STATE_MACHINE_TRANSITION_ALREADY_EXISTS;
        }
    }
    if (map->count >= map->capacity)
    {
        return STATE_MACHINE_TRANSITION_CAPACITY_FULL;
    }

    map->rules[map->count] = *rule;
    map->count++;

    return STATE_MACHINE_TRANSITION_ADDED;
}

/* Find the next state ID for the given source state and event ID. */
StateMachineFindStatus StateMachineFindTransition(
    const StateMachineTransitionMap *map,
    uint8_t currentStateId,
    uint8_t eventId,
    uint8_t *nextStateId
)
{
    if (map == NULL || nextStateId == NULL) {
        return STATE_MACHINE_TRANSITION_FIND_INVALID_ARGUMENT;
    }
    if (map->rules == NULL) {
        return STATE_MACHINE_TRANSITION_FIND_INVALID_ARGUMENT;    
    }
    if (map->count > map->capacity)
    {
        return STATE_MACHINE_TRANSITION_FIND_INVALID_ARGUMENT;
    }
    for (size_t i = 0; i < map->count; i++) {
        if (currentStateId == map->rules[i].currentStateId &&
            eventId == map->rules[i].eventId)
        {
            *nextStateId = map->rules[i].nextStateId;
            return STATE_MACHINE_TRANSITION_FOUND;
        }
    }

    return  STATE_MACHINE_TRANSITION_NOT_FOUND;
}

/* ============================================================
 * STATE MACHINE INITIALIZATION AND START
 * ============================================================ */

/* Bind an instance to its definition and initialize its event queues. */
StateMachineStatus StateMachineInitialize(
    StateMachine *sm,
    const StateMachineDefinition *definition
)
{
    if (sm == NULL || definition == NULL) {
        return STATE_MACHINE_INVALID_ARGUMENT;
    }
    if (definition->states == NULL || definition->transitionMap == NULL) {
        return STATE_MACHINE_INVALID_ARGUMENT;    
    }
    if (definition->transitionMap->count >
        definition->transitionMap->capacity)
    {
        return STATE_MACHINE_INVALID_ARGUMENT;
    }
    uint8_t initial_state_found = 0;
    for (size_t i = 0; i < definition->stateCount; i++) {
        if(definition->initialStateId == definition->states[i].id)
        {
            initial_state_found = 1;
            break;
        }
    }
    if (initial_state_found == 0) {
        return STATE_MACHINE_INVALID_ARGUMENT;
    }
    sm->definition = definition;
    sm->currentStateId = definition->initialStateId;

    EventQueueStatus status;
    status = EventQueueInit(&sm->highEventQueue);
    if (status != EVENT_QUEUE_OK)
    {
        return STATE_MACHINE_INIT_ERROR;
    }
    status = EventQueueInit(&sm->normalEventQueue);
    if (status != EVENT_QUEUE_OK)
    {
        return STATE_MACHINE_INIT_ERROR;
    }
    status = EventQueueInit(&sm->lowEventQueue);
    if (status != EVENT_QUEUE_OK)
    {
        return STATE_MACHINE_INIT_ERROR;
    }

    sm->lifecycle = STATE_MACHINE_LIFECYCLE_INITIALIZED;

    return STATE_MACHINE_INIT_OK;
}

/* Start an initialized instance and invoke the initial state entry callback. */
StateMachineStatus StateMachineStart(StateMachine *sm)
{
    if (sm == NULL) {
        return STATE_MACHINE_INVALID_ARGUMENT;
    }
    if (sm->definition == NULL) {
        return STATE_MACHINE_INVALID_ARGUMENT;
    }
    if (sm->lifecycle != STATE_MACHINE_LIFECYCLE_INITIALIZED)
    {
        return STATE_MACHINE_INVALID_LIFECYCLE;
    }
    if (sm->definition->stateCount == 0) {    
        return STATE_MACHINE_INVALID_ARGUMENT;
    }
    const StateMachineState *initialState = NULL;
    for (size_t i = 0; i < sm->definition->stateCount; i++) {        
        if (sm->definition->initialStateId == sm->definition->states[i].id) {
            initialState = &sm->definition->states[i];
            break;
        }
    }
    if (initialState == NULL) {
        return STATE_MACHINE_STATE_NOT_FOUND;
    }
    if (sm->definition->states == NULL)
    {
        return STATE_MACHINE_INVALID_ARGUMENT;
    }
    sm->lifecycle = STATE_MACHINE_LIFECYCLE_RUNNING;
    if (initialState->onEnter != NULL) {
        initialState->onEnter(sm, NULL);
    }

    return STATE_MACHINE_OK;
}

/* ============================================================
 * STATE MACHINE EVENT ROUTING
 * ============================================================ */

/* Route an event to the queue corresponding to its priority. */
StateMachineStatus StateMachinePushEvent(
    StateMachine *sm,
    const StateMachineEvent *event
)
{
    if (sm == NULL || event == NULL)
    {
        return STATE_MACHINE_INVALID_ARGUMENT;
    }
    struct EventQueue* queue;
    switch (event->priority) {
        case HIGH:
        queue = &sm->highEventQueue;
        break;
        
        case NORMAL:
        queue = &sm->normalEventQueue;
        break;
        
        case LOW:
        queue = &sm->lowEventQueue;
        break;

        default:
        return STATE_MACHINE_INVALID_PRIORITY;
        break;
    }

    EventQueueStatus status = EventQueuePush(queue, event);
    if (status == EVENT_QUEUE_CAPACITY_FULL ) {
        return STATE_MACHINE_QUEUE_FULL;
    }
    if (status == EVENT_QUEUE_NOT_VALID_ARGUMENT ) {
        return STATE_MACHINE_INVALID_ARGUMENT;
    }

    return STATE_MACHINE_OK;
}

/* Take one event, preferring HIGH, then NORMAL, then LOW priority. */
StateMachineStatus StateMachinePopEvent(
    StateMachine *sm,
    StateMachineEvent *event
)
{
    if (sm == NULL || event == NULL) {
        return STATE_MACHINE_INVALID_ARGUMENT;
    }
    EventQueueStatus status;
    
    status = EventQueuePop(&sm->highEventQueue, event);
    if (status != EVENT_QUEUE_OK && status != EVENT_QUEUE_EMPTY) {
        return STATE_MACHINE_ERROR_IN_QUEUE;
    }
    if (status == EVENT_QUEUE_EMPTY) {
    
        status = EventQueuePop(&sm->normalEventQueue, event);
        if (status != EVENT_QUEUE_OK && status != EVENT_QUEUE_EMPTY) {
            return STATE_MACHINE_ERROR_IN_QUEUE;
        }
    }
    if (status == EVENT_QUEUE_EMPTY) {
    
        status = EventQueuePop(&sm->lowEventQueue, event);
        if (status != EVENT_QUEUE_OK && status != EVENT_QUEUE_EMPTY) {
            return STATE_MACHINE_ERROR_IN_QUEUE;
        }
    }

    if (status == EVENT_QUEUE_EMPTY) {
        return STATE_MACHINE_NO_EVENT;
    }

    return STATE_MACHINE_OK;

}

/* ============================================================
 * STATE MACHINE EXECUTION
 * ============================================================ */

/* Process at most one event and update the active state. */
StateMachineStatus StateMachineUpdate(StateMachine *sm)
{
    /* ========================================================
     * VALIDATE STATE MACHINE
     * ======================================================== */

    if (sm == NULL || sm->definition == NULL) {
        return STATE_MACHINE_INVALID_ARGUMENT;
    }

    if (sm->lifecycle != STATE_MACHINE_LIFECYCLE_RUNNING) {
        return STATE_MACHINE_INVALID_LIFECYCLE;
    }

    if (sm->definition->transitionMap == NULL) {
        return STATE_MACHINE_INVALID_ARGUMENT;
    }

    const StateMachineState *currentState =
        StateMachineFindState(
            sm->definition,
            sm->currentStateId
        );

    if (currentState == NULL) {
        return STATE_MACHINE_STATE_NOT_FOUND;
    }

    /* ========================================================
     * POP ONE EVENT
     * ======================================================== */

    StateMachineEvent currentEvent = {0};

    StateMachineStatus status =
        StateMachinePopEvent(sm, &currentEvent);

    if (status != STATE_MACHINE_OK &&
        status != STATE_MACHINE_NO_EVENT)
    {
        return status;
    }

    /* ========================================================
     * PROCESS EVENT AND TRANSITION
     * ======================================================== */

    if (status == STATE_MACHINE_OK)
    {
        uint8_t nextStateId = 0;

        StateMachineFindStatus transitionStatus =
            StateMachineFindTransition(
                sm->definition->transitionMap,
                sm->currentStateId,
                currentEvent.id,
                &nextStateId
            );

        if (transitionStatus == STATE_MACHINE_TRANSITION_FOUND)
        {
            const StateMachineState *nextState =
                StateMachineFindState(
                    sm->definition,
                    nextStateId
                );

            if (nextState == NULL) {
                return STATE_MACHINE_STATE_NOT_FOUND;
            }

            /* Exit current state */

            if (currentState->onExit != NULL) {
                currentState->onExit(sm, &currentEvent);
            }

            /* Change active state */

            sm->currentStateId = nextStateId;
            currentState = nextState;

            /* Enter new state */

            if (currentState->onEnter != NULL) {
                currentState->onEnter(sm, &currentEvent);
            }
        }
        else if (transitionStatus != STATE_MACHINE_TRANSITION_NOT_FOUND)
        {
            return STATE_MACHINE_INVALID_ARGUMENT;
        }
    }

    /* ========================================================
     * UPDATE ACTIVE STATE
     * ======================================================== */

    if (currentState->onUpdate != NULL) {
        currentState->onUpdate(sm, NULL);
    }

    return STATE_MACHINE_OK;
}

