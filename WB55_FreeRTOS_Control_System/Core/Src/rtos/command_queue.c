#include "rtos/command_queue.h"

/*
 * =========================================================
 * FREERTOS CONCEPT: MESSAGE QUEUE
 * =========================================================
 *
 * The complete Queue lifecycle is kept in this file:
 *
 *   osMessageQueueNew()  -> create the queue
 *   osMessageQueuePut()  -> producer sends command data
 *   osMessageQueueGet()  -> consumer receives command data
 */

#define COMMAND_QUEUE_LENGTH  8U

static osMessageQueueId_t commandQueueHandle = NULL;

void CommandQueue_Init(void)
{
    commandQueueHandle = osMessageQueueNew(
        COMMAND_QUEUE_LENGTH,
        sizeof(AppCommand_t),
        NULL
    );
}

osStatus_t CommandQueue_Send(const AppCommand_t *command,
                             uint32_t timeout)
{
    if ((commandQueueHandle == NULL) || (command == NULL))
    {
        return osErrorParameter;
    }

    return osMessageQueuePut(
        commandQueueHandle,
        command,
        0U,
        timeout
    );
}

osStatus_t CommandQueue_Receive(AppCommand_t *command,
                                uint32_t timeout)
{
    if ((commandQueueHandle == NULL) || (command == NULL))
    {
        return osErrorParameter;
    }

    return osMessageQueueGet(
        commandQueueHandle,
        command,
        NULL,
        timeout
    );
}

osMessageQueueId_t CommandQueue_GetHandle(void)
{
    return commandQueueHandle;
}
