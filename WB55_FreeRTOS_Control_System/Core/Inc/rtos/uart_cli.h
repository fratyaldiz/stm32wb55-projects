#ifndef UART_CLI_H
#define UART_CLI_H

#include "cmsis_os2.h"
#include "rtos/command_queue.h"
#include <stdint.h>

/*
 * =========================================================
 * UART COMMAND INTERFACE
 * =========================================================
 *
 * USART1 RX works interrupt-driven.
 *
 * PC terminal
 *      |
 *      v
 * USART1 RX interrupt
 *      |
 *      v
 * RxByteQueue
 *      |
 *      v
 * UartRxTask
 *      |
 *      v
 * Parse text command
 *      |
 *      v
 * CommandQueue
 *      |
 *      v
 * ControlTask
 *
 * Supported commands:
 *   start
 *   stop
 *   warn
 *   fault
 *   reset
 *   status
 *   stack
 *   help
 */

typedef enum
{
    UART_CLI_RESULT_COMMAND = 0,
    UART_CLI_RESULT_HELP,
    UART_CLI_RESULT_INVALID
} UartCliResult_t;

void UartCli_Init(void);

void UartCli_StartReception(void);

UartCliResult_t UartCli_WaitForCommand(AppCommand_t *command,
                                       char *commandText,
                                       uint32_t commandTextSize);

#endif /* UART_CLI_H */
