#ifndef BUTTON_SEMAPHORE_H
#define BUTTON_SEMAPHORE_H

#include "cmsis_os2.h"
#include <stdint.h>

/*
 * =========================================================
 * FREERTOS CONCEPT: BINARY SEMAPHORE
 * =========================================================
 *
 * Purpose:
 *   Notify ButtonTask that the SW1 GPIO interrupt occurred.
 *
 * Flow:
 *
 *   SW1 press
 *      |
 *      v
 *   EXTI interrupt
 *      |
 *      v
 *   osSemaphoreRelease()
 *      |
 *      v
 *   ButtonTask wakes from osSemaphoreAcquire()
 *
 * Why Semaphore?
 *   We only need to signal that an EVENT happened.
 *   No command data is transferred here.
 */

void ButtonSemaphore_Init(void);

osStatus_t ButtonSemaphore_Wait(uint32_t timeout);

void ButtonSemaphore_NotifyFromISR(void);

osSemaphoreId_t ButtonSemaphore_GetHandle(void);

#endif /* BUTTON_SEMAPHORE_H */
