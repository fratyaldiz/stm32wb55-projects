#include "rtos/app_tasks.h"
#include "rtos/task_priorities.h"
#include "rtos/command_queue.h"
#include "rtos/button_semaphore.h"
#include "rtos/uart_mutex.h"
#include "rtos/system_events.h"
#include "rtos/app_timers.h"
#include "rtos/uart_cli.h"
#include "rtos/stack_monitor.h"
#include "main.h"

#include <stdio.h>

/*
 * =========================================================
 * FREERTOS CONTROL SYSTEM - STAGE 7
 * =========================================================
 *
 * PC terminal
 *     |
 *     v
 * USART1 RX interrupt
 *     |
 *     v
 * RxByteQueue
 *     |
 *     v
 * UartRxTask
 *     |
 *     v
 * CommandQueue
 *     |
 *     v
 * ControlTask  <---- ButtonTask
 *     ^                  ^
 *     |                  |
 * Software Timers    Semaphore / EXTI
 *     |
 *     v
 * State Machine
 *     |
 *     v
 * Event Flags
 *     |
 *     v
 * LedTask
 *
 * UART TX is protected by UartMutex.
 */

static osThreadId_t controlTaskHandle;
static osThreadId_t uartRxTaskHandle;
static osThreadId_t buttonTaskHandle;
static osThreadId_t ledTaskHandle;
static osThreadId_t monitorTaskHandle;


typedef enum
{
    SYSTEM_STATE_IDLE = 0,
    SYSTEM_STATE_RUNNING,
    SYSTEM_STATE_WARNING,
    SYSTEM_STATE_FAULT
} SystemState_t;

static void ControlTask(void *argument);
static void UartRxTask(void *argument);
static void ButtonTask(void *argument);
static void LedTask(void *argument);
static void MonitorTask(void *argument);

static const char *StateName(SystemState_t state)
{
    switch (state)
    {
        case SYSTEM_STATE_IDLE:
            return "IDLE";

        case SYSTEM_STATE_RUNNING:
            return "RUNNING";

        case SYSTEM_STATE_WARNING:
            return "WARNING";

        case SYSTEM_STATE_FAULT:
            return "FAULT";

        default:
            return "UNKNOWN";
    }
}

static void PublishState(SystemState_t state)
{
    switch (state)
    {
        case SYSTEM_STATE_IDLE:
            SystemEvents_SetState(EVENT_STATE_IDLE);
            break;

        case SYSTEM_STATE_RUNNING:
            SystemEvents_SetState(EVENT_STATE_RUNNING);
            break;

        case SYSTEM_STATE_WARNING:
            SystemEvents_SetState(EVENT_STATE_WARNING);
            break;

        case SYSTEM_STATE_FAULT:
            SystemEvents_SetState(EVENT_STATE_FAULT);
            break;

        default:
            SystemEvents_SetState(EVENT_STATE_IDLE);
            break;
    }
}

static void PrintHelp(void)
{
    UartMutex_Log(
        "\r\n"
        "========== UART COMMANDS ==========\r\n"
        "start  : IDLE -> RUNNING\r\n"
        "stop   : RUNNING/WARNING -> IDLE\r\n"
        "warn   : force WARNING while RUNNING\r\n"
        "fault  : enter FAULT state\r\n"
        "reset  : WARNING/FAULT -> IDLE\r\n"
        "status : show current system status\r\n"
        "stack  : show task stack diagnostics\r\n"
        "help   : show this command list\r\n"
        "===================================\r\n\r\n"
    );
}

void AppTasks_Init(void)
{
    const osThreadAttr_t controlAttr = {
        .name = "ControlTask",
        .priority = APP_PRIORITY_CONTROL,
        .stack_size = APP_CONTROL_TASK_STACK_BYTES
    };

    const osThreadAttr_t uartRxAttr = {
        .name = "UartRxTask",
        .priority = APP_PRIORITY_UART_RX,
        .stack_size = APP_UART_RX_TASK_STACK_BYTES
    };

    const osThreadAttr_t buttonAttr = {
        .name = "ButtonTask",
        .priority = APP_PRIORITY_BUTTON,
        .stack_size = APP_BUTTON_TASK_STACK_BYTES
    };

    const osThreadAttr_t ledAttr = {
        .name = "LedTask",
        .priority = APP_PRIORITY_LED,
        .stack_size = APP_LED_TASK_STACK_BYTES
    };

    const osThreadAttr_t monitorAttr = {
        .name = "MonitorTask",
        .priority = APP_PRIORITY_MONITOR,
        .stack_size = APP_MONITOR_TASK_STACK_BYTES
    };

    controlTaskHandle = osThreadNew(ControlTask, NULL, &controlAttr);
    uartRxTaskHandle = osThreadNew(UartRxTask, NULL, &uartRxAttr);
    buttonTaskHandle = osThreadNew(ButtonTask, NULL, &buttonAttr);
    ledTaskHandle = osThreadNew(LedTask, NULL, &ledAttr);
    monitorTaskHandle = osThreadNew(MonitorTask, NULL, &monitorAttr);

    if ((controlTaskHandle == NULL) ||
        (uartRxTaskHandle == NULL) ||
        (buttonTaskHandle == NULL) ||
        (ledTaskHandle == NULL) ||
        (monitorTaskHandle == NULL))
    {
        Error_Handler();
    }
}

static void ControlTask(void *argument)
{
    AppCommand_t command;
    SystemState_t systemState = SYSTEM_STATE_IDLE;
    char statusBuffer[160];

    (void)argument;

    UartMutex_Log(
        "\r\n"
        "========================================\r\n"
        " FreeRTOS Control System - Stage 8\r\n"
        " FINAL INTEGRATION + STACK MONITOR\r\n"
        "========================================\r\n"
        "[UART] USART1 RX is interrupt-driven.\r\n"
        "[UART] Type 'help' and press Enter.\r\n"
        "[STATE] IDLE\r\n\r\n"
    );

    PublishState(systemState);

    if (AppTimers_StartHeartbeat() == osOK)
    {
        UartMutex_Log(
            "[TIMER] HeartbeatTimer started (periodic / 5 s).\r\n"
        );
    }

    for (;;)
    {
        if (CommandQueue_Receive(&command, osWaitForever) != osOK)
        {
            continue;
        }

        /*
         * =====================================================
         * BUTTON COMMAND
         * =====================================================
         */
        if ((command.type == APP_CMD_TOGGLE_RUN) &&
            (command.source == APP_CMD_SOURCE_BUTTON))
        {
            if (systemState == SYSTEM_STATE_IDLE)
            {
                systemState = SYSTEM_STATE_RUNNING;

                UartMutex_Log(
                    "[BUTTON] Command accepted.\r\n"
                    "[STATE] IDLE -> RUNNING\r\n"
                );

                (void)AppTimers_StartRunWarning();
            }
            else if (systemState == SYSTEM_STATE_RUNNING)
            {
                systemState = SYSTEM_STATE_IDLE;

                (void)AppTimers_StopRunWarning();

                UartMutex_Log(
                    "[BUTTON] Command accepted.\r\n"
                    "[STATE] RUNNING -> IDLE\r\n"
                );
            }
            else if (systemState == SYSTEM_STATE_WARNING)
            {
                systemState = SYSTEM_STATE_IDLE;

                UartMutex_Log(
                    "[BUTTON] WARNING acknowledged.\r\n"
                    "[STATE] WARNING -> IDLE\r\n"
                );
            }
            else
            {
                UartMutex_Log(
                    "[BUTTON] Rejected: system is in FAULT. Use 'reset'.\r\n"
                );
            }

            PublishState(systemState);
        }

        /*
         * =====================================================
         * UART: START
         * =====================================================
         */
        else if ((command.type == APP_CMD_START) &&
                 (command.source == APP_CMD_SOURCE_UART))
        {
            if (systemState == SYSTEM_STATE_IDLE)
            {
                systemState = SYSTEM_STATE_RUNNING;
                (void)AppTimers_StartRunWarning();

                UartMutex_Log(
                    "[CONTROL] START accepted.\r\n"
                    "[STATE] IDLE -> RUNNING\r\n"
                    "[TIMER] RunWarningTimer started (one-shot / 10 s).\r\n"
                );

                PublishState(systemState);
            }
            else if (systemState == SYSTEM_STATE_RUNNING)
            {
                UartMutex_Log(
                    "[CONTROL] START ignored: already RUNNING.\r\n"
                );
            }
            else
            {
                UartMutex_Log(
                    "[CONTROL] START rejected: reset WARNING/FAULT first.\r\n"
                );
            }
        }

        /*
         * =====================================================
         * UART: STOP
         * =====================================================
         */
        else if ((command.type == APP_CMD_STOP) &&
                 (command.source == APP_CMD_SOURCE_UART))
        {
            if ((systemState == SYSTEM_STATE_RUNNING) ||
                (systemState == SYSTEM_STATE_WARNING))
            {
                (void)AppTimers_StopRunWarning();

                UartMutex_Log("[CONTROL] STOP accepted.\r\n");

                if (systemState == SYSTEM_STATE_RUNNING)
                {
                    UartMutex_Log("[STATE] RUNNING -> IDLE\r\n");
                }
                else
                {
                    UartMutex_Log("[STATE] WARNING -> IDLE\r\n");
                }

                systemState = SYSTEM_STATE_IDLE;
                PublishState(systemState);
            }
            else if (systemState == SYSTEM_STATE_IDLE)
            {
                UartMutex_Log(
                    "[CONTROL] STOP ignored: already IDLE.\r\n"
                );
            }
            else
            {
                UartMutex_Log(
                    "[CONTROL] STOP rejected in FAULT. Use 'reset'.\r\n"
                );
            }
        }

        /*
         * =====================================================
         * UART: WARN
         * =====================================================
         */
        else if ((command.type == APP_CMD_WARNING) &&
                 (command.source == APP_CMD_SOURCE_UART))
        {
            if (systemState == SYSTEM_STATE_RUNNING)
            {
                (void)AppTimers_StopRunWarning();
                systemState = SYSTEM_STATE_WARNING;

                UartMutex_Log(
                    "[CONTROL] WARN accepted.\r\n"
                    "[STATE] RUNNING -> WARNING\r\n"
                );

                PublishState(systemState);
            }
            else
            {
                UartMutex_Log(
                    "[CONTROL] WARN rejected: system must be RUNNING.\r\n"
                );
            }
        }

        /*
         * =====================================================
         * UART: FAULT
         * =====================================================
         */
        else if ((command.type == APP_CMD_FAULT) &&
                 (command.source == APP_CMD_SOURCE_UART))
        {
            if (systemState != SYSTEM_STATE_FAULT)
            {
                (void)AppTimers_StopRunWarning();

                UartMutex_Log("[CONTROL] FAULT command accepted.\r\n");

                if (systemState == SYSTEM_STATE_IDLE)
                {
                    UartMutex_Log("[STATE] IDLE -> FAULT\r\n");
                }
                else if (systemState == SYSTEM_STATE_RUNNING)
                {
                    UartMutex_Log("[STATE] RUNNING -> FAULT\r\n");
                }
                else
                {
                    UartMutex_Log("[STATE] WARNING -> FAULT\r\n");
                }

                systemState = SYSTEM_STATE_FAULT;
                PublishState(systemState);
            }
            else
            {
                UartMutex_Log(
                    "[CONTROL] FAULT ignored: already in FAULT.\r\n"
                );
            }
        }

        /*
         * =====================================================
         * UART: RESET
         * =====================================================
         */
        else if ((command.type == APP_CMD_RESET) &&
                 (command.source == APP_CMD_SOURCE_UART))
        {
            if (systemState == SYSTEM_STATE_FAULT)
            {
                systemState = SYSTEM_STATE_IDLE;

                UartMutex_Log(
                    "[CONTROL] RESET accepted.\r\n"
                    "[STATE] FAULT -> IDLE\r\n"
                );

                PublishState(systemState);
            }
            else if (systemState == SYSTEM_STATE_WARNING)
            {
                systemState = SYSTEM_STATE_IDLE;

                UartMutex_Log(
                    "[CONTROL] RESET accepted.\r\n"
                    "[STATE] WARNING -> IDLE\r\n"
                );

                PublishState(systemState);
            }
            else
            {
                UartMutex_Log(
                    "[CONTROL] RESET ignored: no WARNING/FAULT active.\r\n"
                );
            }
        }

        /*
         * =====================================================
         * UART: STATUS
         * =====================================================
         */
        else if ((command.type == APP_CMD_STATUS) &&
                 (command.source == APP_CMD_SOURCE_UART))
        {
            uint32_t tickFrequency = osKernelGetTickFreq();
            uint32_t uptimeSeconds = 0U;

            if (tickFrequency != 0U)
            {
                uptimeSeconds =
                    osKernelGetTickCount() / tickFrequency;
            }

            (void)snprintf(
                statusBuffer,
                sizeof(statusBuffer),
                "\r\n========== SYSTEM STATUS ==========\r\n"
                "State  : %s\r\n"
                "Uptime : %lu s\r\n"
                "===================================\r\n\r\n",
                StateName(systemState),
                (unsigned long)uptimeSeconds
            );

            UartMutex_Log(statusBuffer);
        }

        /*
         * =====================================================
         * UART: STACK
         * =====================================================
         */
        else if ((command.type == APP_CMD_STACK) &&
                 (command.source == APP_CMD_SOURCE_UART))
        {
            /*
             * FREERTOS STACK MANAGEMENT:
             * Print the stack watermark of every application task.
             */
            StackMonitor_PrintReport();
        }

        /*
         * =====================================================
         * PERIODIC SOFTWARE TIMER
         * =====================================================
         */
        else if ((command.type == APP_CMD_TIMER_HEARTBEAT) &&
                 (command.source == APP_CMD_SOURCE_TIMER))
        {
            /* Periodic heartbeat received.
               No UART log in final version. */
        }

        /*
         * =====================================================
         * ONE-SHOT SOFTWARE TIMER
         * =====================================================
         */
        else if ((command.type == APP_CMD_RUN_TIMEOUT) &&
                 (command.source == APP_CMD_SOURCE_TIMER))
        {
            if (systemState == SYSTEM_STATE_RUNNING)
            {
                systemState = SYSTEM_STATE_WARNING;

                UartMutex_Log(
                    "[TIMER] RunWarningTimer expired -> one-shot event received.\r\n"
                    "[STATE] RUNNING -> WARNING\r\n"
                );

                PublishState(systemState);
            }
        }
    }
}

static void UartRxTask(void *argument)
{
    AppCommand_t command;
    UartCliResult_t result;
    char commandText[32];
    char logBuffer[80];

    (void)argument;

    /*
     * Start USART1 RX only after:
     * - scheduler is running
     * - RxByteQueue exists
     * - UartRxTask exists
     */
    UartCli_StartReception();

    UartMutex_Log(
        "[UART] Receiver ready. Type 'help' and press Enter.\r\n"
    );

    for (;;)
    {
        /*
         * UartRxTask BLOCKS here waiting for a complete text line.
         */
        result = UartCli_WaitForCommand(
            &command,
            commandText,
            sizeof(commandText)
        );

        if (result == UART_CLI_RESULT_COMMAND)
        {
            (void)snprintf(
                logBuffer,
                sizeof(logBuffer),
                "[UART RX] command = %s\r\n",
                commandText
            );

            UartMutex_Log(logBuffer);

            /*
             * UART task becomes another CommandQueue PRODUCER.
             */
            if (CommandQueue_Send(&command, 100U) != osOK)
            {
                UartMutex_Log(
                    "[UART] ERROR: CommandQueue is full.\r\n"
                );
            }
        }
        else if (result == UART_CLI_RESULT_HELP)
        {
            PrintHelp();
        }
        else
        {
            UartMutex_Log(
                "[UART] Unknown command. Type 'help'.\r\n"
            );
        }
    }
}

static void ButtonTask(void *argument)
{
    AppCommand_t command;

    (void)argument;

    for (;;)
    {
        if (ButtonSemaphore_Wait(osWaitForever) == osOK)
        {
            osDelay(30U);

            if (HAL_GPIO_ReadPin(
                    SW1_GPIO_Port,
                    SW1_Pin) == GPIO_PIN_RESET)
            {
                UartMutex_Log(
                    "[BUTTON] SW1 -> semaphore woke ButtonTask.\r\n"
                );

                command.type = APP_CMD_TOGGLE_RUN;
                command.source = APP_CMD_SOURCE_BUTTON;

                (void)CommandQueue_Send(
                    &command,
                    0U
                );

                while (HAL_GPIO_ReadPin(
                           SW1_GPIO_Port,
                           SW1_Pin) == GPIO_PIN_RESET)
                {
                    osDelay(10U);
                }

                osDelay(20U);

                while (ButtonSemaphore_Wait(0U) == osOK)
                {
                    /* Drain mechanical bounce events. */
                }
            }
        }
    }
}

static void LedTask(void *argument)
{
    uint32_t eventResult;
    uint32_t activeEvents;
    uint32_t currentStateFlag = 0U;
    uint8_t animationPhase = 0U;

    (void)argument;

    for (;;)
    {
        eventResult = SystemEvents_WaitForStateChange(250U);

        if ((eventResult & osFlagsError) == 0U)
        {
            activeEvents = SystemEvents_Get();
            currentStateFlag = activeEvents & EVENT_STATE_MASK;
            animationPhase = 0U;

            if (currentStateFlag == EVENT_STATE_IDLE)
            {
                UartMutex_Log(
                    "[EVENT] LedTask received IDLE state.\r\n"
                );
            }
            else if (currentStateFlag == EVENT_STATE_RUNNING)
            {
                UartMutex_Log(
                    "[EVENT] LedTask received RUNNING state.\r\n"
                );
            }
            else if (currentStateFlag == EVENT_STATE_WARNING)
            {
                UartMutex_Log(
                    "[EVENT] LedTask received WARNING state.\r\n"
                );
            }
            else if (currentStateFlag == EVENT_STATE_FAULT)
            {
                UartMutex_Log(
                    "[EVENT] LedTask received FAULT state.\r\n"
                );
            }
        }

        if (currentStateFlag == EVENT_STATE_IDLE)
        {
            HAL_GPIO_WritePin(
                LD1_GPIO_Port,
                LD1_Pin,
                GPIO_PIN_RESET
            );

            HAL_GPIO_TogglePin(
                LD2_GPIO_Port,
                LD2_Pin
            );
        }
        else if (currentStateFlag == EVENT_STATE_RUNNING)
        {
            HAL_GPIO_WritePin(
                LD2_GPIO_Port,
                LD2_Pin,
                GPIO_PIN_SET
            );

            HAL_GPIO_TogglePin(
                LD1_GPIO_Port,
                LD1_Pin
            );
        }
        else if (currentStateFlag == EVENT_STATE_WARNING)
        {
            animationPhase ^= 1U;

            HAL_GPIO_WritePin(
                LD1_GPIO_Port,
                LD1_Pin,
                animationPhase ? GPIO_PIN_SET : GPIO_PIN_RESET
            );

            HAL_GPIO_WritePin(
                LD2_GPIO_Port,
                LD2_Pin,
                animationPhase ? GPIO_PIN_RESET : GPIO_PIN_SET
            );
        }
        else if (currentStateFlag == EVENT_STATE_FAULT)
        {
            animationPhase ^= 1U;

            /*
             * FAULT indication:
             * both LEDs blink together rapidly.
             */
            HAL_GPIO_WritePin(
                LD1_GPIO_Port,
                LD1_Pin,
                animationPhase ? GPIO_PIN_SET : GPIO_PIN_RESET
            );

            HAL_GPIO_WritePin(
                LD2_GPIO_Port,
                LD2_Pin,
                animationPhase ? GPIO_PIN_SET : GPIO_PIN_RESET
            );
        }
        else
        {
            HAL_GPIO_WritePin(
                LD1_GPIO_Port,
                LD1_Pin,
                GPIO_PIN_RESET
            );

            HAL_GPIO_WritePin(
                LD2_GPIO_Port,
                LD2_Pin,
                GPIO_PIN_RESET
            );
        }
    }
}

static void MonitorTask(void *argument)
{
    (void)argument;

    /*
     * Low-priority health monitoring.
     *
     * Every 15 seconds it checks the stack watermarks.
     * It prints only if a task drops below the warning threshold,
     * so normal UART output is not flooded.
     */
    for (;;)
    {
        osDelay(15000U);
        StackMonitor_CheckThresholds();
    }
}

osThreadId_t AppTasks_GetControlTaskHandle(void)
{
    return controlTaskHandle;
}

osThreadId_t AppTasks_GetUartRxTaskHandle(void)
{
    return uartRxTaskHandle;
}

osThreadId_t AppTasks_GetButtonTaskHandle(void)
{
    return buttonTaskHandle;
}

osThreadId_t AppTasks_GetLedTaskHandle(void)
{
    return ledTaskHandle;
}

osThreadId_t AppTasks_GetMonitorTaskHandle(void)
{
    return monitorTaskHandle;
}
