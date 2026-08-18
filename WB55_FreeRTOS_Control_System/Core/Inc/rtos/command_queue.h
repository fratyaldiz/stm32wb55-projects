#ifndef COMMAND_QUEUE_H
#define COMMAND_QUEUE_H

#include "cmsis_os2.h"
#include <stdint.h>

/*
 * =========================================================
 * FREERTOS CONCEPT: MESSAGE QUEUE
 * =========================================================
 *
 * Purpose:
 *   Carries commands from producer tasks to ControlTask.
 *
 * Stage 2 producer:
 *   - ButtonTask
 *
 * Later producers:
 *   - UartRxTask
 *   - Software Timer callback
 *
 * Consumer:
 *   - ControlTask
 *
 * Why Queue?
 *   A command is DATA. Queue safely transfers that data
 *   between independent tasks.
 */

typedef enum
{
    APP_CMD_NONE = 0,
    APP_CMD_TOGGLE_RUN,
    APP_CMD_START,
    APP_CMD_STOP,
    APP_CMD_WARNING,
    APP_CMD_FAULT,
    APP_CMD_RESET,
    APP_CMD_STATUS,
    APP_CMD_STACK,
    APP_CMD_WARNING_TIMEOUT
} AppCommandType_t;

typedef enum
{
    APP_CMD_SOURCE_BUTTON = 0,
    APP_CMD_SOURCE_UART,
    APP_CMD_SOURCE_TIMER
} AppCommandSource_t;

typedef struct
{
    AppCommandType_t type;
    AppCommandSource_t source;
} AppCommand_t;

void CommandQueue_Init(void);

osStatus_t CommandQueue_Send(const AppCommand_t *command,
                             uint32_t timeout);

osStatus_t CommandQueue_Receive(AppCommand_t *command,
                                uint32_t timeout);

osMessageQueueId_t CommandQueue_GetHandle(void);

#endif /* COMMAND_QUEUE_H */
