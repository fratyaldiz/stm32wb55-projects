#ifndef SYSTEM_EVENTS_H
#define SYSTEM_EVENTS_H

#include "cmsis_os2.h"
#include <stdint.h>

/*
 * =========================================================
 * FREERTOS CONCEPT: EVENT FLAGS
 * =========================================================
 *
 * Event Flags are bit-based system signals.
 *
 * ControlTask is the source of truth for the system state.
 * It updates these flags whenever the state changes.
 *
 * LedTask waits for EVENT_STATE_CHANGED and then reads the
 * current state bit to decide the LED behavior.
 */

#define EVENT_STATE_IDLE       (1U << 0)
#define EVENT_STATE_RUNNING    (1U << 1)
#define EVENT_STATE_WARNING    (1U << 2)
#define EVENT_STATE_FAULT      (1U << 3)
#define EVENT_STATE_CHANGED    (1U << 4)

#define EVENT_STATE_MASK       (EVENT_STATE_IDLE    | \
                                EVENT_STATE_RUNNING | \
                                EVENT_STATE_WARNING | \
                                EVENT_STATE_FAULT)

void SystemEvents_Init(void);

void SystemEvents_SetState(uint32_t stateFlag);

uint32_t SystemEvents_Get(void);

uint32_t SystemEvents_WaitForStateChange(uint32_t timeout);

osEventFlagsId_t SystemEvents_GetHandle(void);

#endif /* SYSTEM_EVENTS_H */
