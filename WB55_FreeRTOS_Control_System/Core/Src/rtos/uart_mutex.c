#include "rtos/uart_mutex.h"
#include "main.h"

#include <string.h>

/*
 * =========================================================
 * FREERTOS CONCEPT: MUTEX
 * =========================================================
 *
 * Complete Mutex lifecycle:
 *
 *   osMutexNew()      -> create mutex
 *   osMutexAcquire()  -> lock shared UART
 *   HAL_UART_Transmit()
 *   osMutexRelease()  -> unlock UART
 *
 * Multiple tasks can safely call UartMutex_Log().
 */

extern UART_HandleTypeDef huart1;

static osMutexId_t uartMutexHandle = NULL;

void UartMutex_Init(void)
{
    const osMutexAttr_t mutexAttr = {
        .name = "UartMutex",
        .attr_bits = osMutexPrioInherit
    };

    uartMutexHandle = osMutexNew(&mutexAttr);
}

void UartMutex_Log(const char *message)
{
    if ((uartMutexHandle == NULL) || (message == NULL))
    {
        return;
    }

    if (osMutexAcquire(uartMutexHandle, osWaitForever) == osOK)
    {
        HAL_UART_Transmit(
            &huart1,
            (uint8_t *)message,
            (uint16_t)strlen(message),
            1000U
        );

        (void)osMutexRelease(uartMutexHandle);
    }
}

osMutexId_t UartMutex_GetHandle(void)
{
    return uartMutexHandle;
}
