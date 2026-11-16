
/* ============================================================
 * INCLUDES
 * ============================================================ */

#include "machine_fsm.h"
#include <stddef.h>
#include <stdint.h>





/* ============================================================
 * EVENT QUEUE - PUSH
 * ============================================================ */

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


/* ============================================================
 * EVENT QUEUE - POP
 * ============================================================ */

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
 * EVENT QUEUE - INITIALIZATION
 * ============================================================ */

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


/* ============================================================
 * STATE MACHINE - INITIALIZATION
 * ============================================================ */

StateMachineStatus StateMachineInitialize(StateMachine *sm)
{
    if (sm == NULL) {
        return STATE_MACHINE_INVALID_ARGUMENT;
    }

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
    return STATE_MACHINE_INIT_OK;
}


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


