#include "rtos/button_semaphore.h"

/*
 * =========================================================
 * FREERTOS CONCEPT: BINARY SEMAPHORE
 * =========================================================
 *
 * Complete Semaphore lifecycle:
 *
 *   osSemaphoreNew()      -> create binary semaphore
 *   osSemaphoreAcquire()  -> ButtonTask waits / blocks
 *   osSemaphoreRelease()  -> GPIO interrupt wakes ButtonTask
 */

static osSemaphoreId_t buttonSemaphoreHandle = NULL;

void ButtonSemaphore_Init(void)
{
    /*
     * max_count     = 1  -> binary semaphore
     * initial_count = 0  -> starts unavailable
     *
     * ButtonTask will block until the interrupt releases it.
     */
    buttonSemaphoreHandle = osSemaphoreNew(
        1U,
        0U,
        NULL
    );
}

osStatus_t ButtonSemaphore_Wait(uint32_t timeout)
{
    if (buttonSemaphoreHandle == NULL)
    {
        return osErrorParameter;
    }

    return osSemaphoreAcquire(
        buttonSemaphoreHandle,
        timeout
    );
}

void ButtonSemaphore_NotifyFromISR(void)
{
    if (buttonSemaphoreHandle != NULL)
    {
        /*
         * CMSIS-RTOS2 allows osSemaphoreRelease() from ISR context.
         * Keep the interrupt short: only signal the task here.
         */
        (void)osSemaphoreRelease(buttonSemaphoreHandle);
    }
}

osSemaphoreId_t ButtonSemaphore_GetHandle(void)
{
    return buttonSemaphoreHandle;
}
