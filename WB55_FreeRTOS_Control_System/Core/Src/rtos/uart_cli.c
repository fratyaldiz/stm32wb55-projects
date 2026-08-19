#include "rtos/uart_cli.h"
#include "main.h"

#include <ctype.h>
#include <string.h>

/*
 * =========================================================
 * UART RX - INTERRUPT DRIVEN COMMAND INTERFACE
 * =========================================================
 *
 * IMPORTANT:
 *
 * The UART interrupt does NOT parse commands and does NOT
 * perform long processing.
 *
 * It only:
 *   1) puts the received byte into RxByteQueue
 *   2) rearms USART1 reception
 *
 * UartRxTask performs the command parsing in task context.
 */

#define UART_RX_BYTE_QUEUE_LENGTH  64U

extern UART_HandleTypeDef huart1;

static osMessageQueueId_t uartRxByteQueueHandle = NULL;
static uint8_t uartRxByte = 0U;

static void NormalizeCommand(char *text)
{
    uint32_t readIndex = 0U;
    uint32_t writeIndex = 0U;
    uint32_t length;

    if (text == NULL)
    {
        return;
    }

    length = (uint32_t)strlen(text);

    /*
     * Skip leading spaces.
     */
    while ((readIndex < length) &&
           isspace((unsigned char)text[readIndex]))
    {
        readIndex++;
    }

    /*
     * Copy while converting to lower-case.
     */
    while (readIndex < length)
    {
        text[writeIndex] =
            (char)tolower((unsigned char)text[readIndex]);

        writeIndex++;
        readIndex++;
    }

    text[writeIndex] = '\0';

    /*
     * Remove trailing spaces.
     */
    while ((writeIndex > 0U) &&
           isspace((unsigned char)text[writeIndex - 1U]))
    {
        writeIndex--;
        text[writeIndex] = '\0';
    }
}

void UartCli_Init(void)
{
    uartRxByteQueueHandle = osMessageQueueNew(
        UART_RX_BYTE_QUEUE_LENGTH,
        sizeof(uint8_t),
        NULL
    );
}

void UartCli_StartReception(void)
{
    if (uartRxByteQueueHandle == NULL)
    {
        return;
    }

    /*
     * Start receiving one byte with interrupt.
     * HAL_UART_RxCpltCallback() rearms the next byte.
     */
    (void)HAL_UART_Receive_IT(
        &huart1,
        &uartRxByte,
        1U
    );
}

UartCliResult_t UartCli_WaitForCommand(AppCommand_t *command,
                                       char *commandText,
                                       uint32_t commandTextSize)
{
    uint8_t receivedByte;
    uint32_t index = 0U;

    if ((command == NULL) ||
        (commandText == NULL) ||
        (commandTextSize < 2U))
    {
        return UART_CLI_RESULT_INVALID;
    }

    for (;;)
    {
        if (osMessageQueueGet(
                uartRxByteQueueHandle,
                &receivedByte,
                NULL,
                osWaitForever) != osOK)
        {
            continue;
        }

        /*
         * Enter may arrive as CR, LF or CR+LF.
         */
        if ((receivedByte == '\r') ||
            (receivedByte == '\n'))
        {
            if (index == 0U)
            {
                /*
                 * Ignore the second character of CR+LF and
                 * empty terminal lines.
                 */
                continue;
            }

            commandText[index] = '\0';
            NormalizeCommand(commandText);
            break;
        }

        /*
         * Handle Backspace / Delete from terminal.
         */
        if ((receivedByte == '\b') ||
            (receivedByte == 0x7FU))
        {
            if (index > 0U)
            {
                index--;
            }

            continue;
        }

        /*
         * Store normal printable characters.
         */
        if ((receivedByte >= 32U) &&
            (receivedByte <= 126U))
        {
            if (index < (commandTextSize - 1U))
            {
                commandText[index] = (char)receivedByte;
                index++;
            }
        }
    }

    command->source = APP_CMD_SOURCE_UART;

    if (strcmp(commandText, "start") == 0)
    {
        command->type = APP_CMD_START;
        return UART_CLI_RESULT_COMMAND;
    }

    if (strcmp(commandText, "stop") == 0)
    {
        command->type = APP_CMD_STOP;
        return UART_CLI_RESULT_COMMAND;
    }

    if (strcmp(commandText, "warn") == 0)
    {
        command->type = APP_CMD_WARNING;
        return UART_CLI_RESULT_COMMAND;
    }

    if (strcmp(commandText, "fault") == 0)
    {
        command->type = APP_CMD_FAULT;
        return UART_CLI_RESULT_COMMAND;
    }

    if (strcmp(commandText, "reset") == 0)
    {
        command->type = APP_CMD_RESET;
        return UART_CLI_RESULT_COMMAND;
    }

    if (strcmp(commandText, "status") == 0)
    {
        command->type = APP_CMD_STATUS;
        return UART_CLI_RESULT_COMMAND;
    }

    if (strcmp(commandText, "stack") == 0)
    {
        command->type = APP_CMD_STACK;
        return UART_CLI_RESULT_COMMAND;
    }

    if (strcmp(commandText, "help") == 0)
    {
        return UART_CLI_RESULT_HELP;
    }

    return UART_CLI_RESULT_INVALID;
}

/*
 * HAL UART receive-complete callback.
 *
 * USART1 IRQ priority was configured at a FreeRTOS-safe value.
 * osMessageQueuePut() is called with timeout = 0 because this
 * callback runs in interrupt context.
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if ((huart != NULL) &&
        (huart->Instance == USART1))
    {
        if (uartRxByteQueueHandle != NULL)
        {
            (void)osMessageQueuePut(
                uartRxByteQueueHandle,
                &uartRxByte,
                0U,
                0U
            );
        }

        /*
         * Rearm reception immediately for the next character.
         */
        (void)HAL_UART_Receive_IT(
            &huart1,
            &uartRxByte,
            1U
        );
    }
}
