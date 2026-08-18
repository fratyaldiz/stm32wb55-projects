#include "rtos/system_events.h"

/*
 * =========================================================
 * FREERTOS CONCEPT: EVENT FLAGS
 * =========================================================
 *
 * Complete Event Flags lifecycle:
 *
 *   osEventFlagsNew()    -> create event flags object
 *   osEventFlagsClear()  -> clear old state bits
 *   osEventFlagsSet()    -> set new state + change event
 *   osEventFlagsWait()   -> LedTask waits for state change
 *   osEventFlagsGet()    -> read active state bits
 */

static osEventFlagsId_t systemEventsHandle = NULL;

void SystemEvents_Init(void)
{
    systemEventsHandle = osEventFlagsNew(NULL);
}

void SystemEvents_SetState(uint32_t stateFlag)
{
    if (systemEventsHandle == NULL)
    {
        return;
    }

    /*
     * Only one state bit should be active at a time.
     * First clear previous state, then publish the new state.
     */
    (void)osEventFlagsClear(
        systemEventsHandle,
        EVENT_STATE_MASK
    );

    (void)osEventFlagsSet(
        systemEventsHandle,
        stateFlag | EVENT_STATE_CHANGED
    );
}

uint32_t SystemEvents_Get(void)
{
    if (systemEventsHandle == NULL)
    {
        return 0U;
    }

    return osEventFlagsGet(systemEventsHandle);
}

uint32_t SystemEvents_WaitForStateChange(uint32_t timeout)
{
    if (systemEventsHandle == NULL)
    {
        return 0U;
    }

    /*
     * EVENT_STATE_CHANGED is automatically cleared when received.
     * State bits remain active until ControlTask changes state.
     */
    return osEventFlagsWait(
        systemEventsHandle,
        EVENT_STATE_CHANGED,
        osFlagsWaitAny,
        timeout
    );
}

osEventFlagsId_t SystemEvents_GetHandle(void)
{
    return systemEventsHandle;
}
