
/* ============================================================
 * INCLUDES
 * ============================================================ */

#include "machine_fsm.h"
#include <stddef.h>
#include <stdint.h>


/* ============================================================
 * TRANSITION MANAGEMENT
 * ============================================================ */

StateMachineTransitionStatus StateMachineAddTransition(
    StateMachineTransitionMap *map,
    StateMachineTransition *rule
)
{
    if (map->count >= map->capacity)
    {
        return STATE_MACHINE_TRANSITION_CAPACITY_FULL;
    }

    map->rules[map->count] = *rule;
    map->count += 1;

    return STATE_MACHINE_TRANSITION_ADDED;
}


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


