#ifndef UART_MUTEX_H
#define UART_MUTEX_H

#include "cmsis_os2.h"

/*
 * =========================================================
 * FREERTOS CONCEPT: MUTEX
 * =========================================================
 *
 * Purpose:
 *   Protect USART1 because multiple tasks may write to UART.
 *
 * Flow:
 *
 *   Task A ----\
 *   Task B -----+--> UART Mutex --> USART1
 *   Task C ----/
 *
 * Why Mutex?
 *   UART is a shared resource. Only one task should use it
 *   at a time, otherwise messages can overlap/interleave.
 */

void UartMutex_Init(void);

void UartMutex_Log(const char *message);

osMutexId_t UartMutex_GetHandle(void);

#endif /* UART_MUTEX_H */
