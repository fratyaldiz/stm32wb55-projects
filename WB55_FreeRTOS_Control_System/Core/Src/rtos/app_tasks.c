#include "rtos/app_tasks.h"
#include "rtos/task_priorities.h"
#include "rtos/command_queue.h"
#include "rtos/button_semaphore.h"
#include "rtos/uart_mutex.h"
#include "rtos/system_events.h"
#include "main.h"

/*
 * =========================================================
 * FREERTOS CONCEPTS USED SO FAR
 * =========================================================
 *
 * TASKS:
 *   ControlTask, UartRxTask, ButtonTask, LedTask, MonitorTask
 *
 * PRIORITY:
 *   Defined in task_priorities.h
 *
 * SEMAPHORE:
 *   EXTI interrupt -> ButtonTask notification
 *
 * QUEUE:
 *   ButtonTask -> command data -> ControlTask
 *
 * MUTEX:
 *   Multiple tasks -> protected USART1
 *
 * EVENT FLAGS:
 *   ControlTask -> system state flags -> LedTask
 */

static osThreadId_t controlTaskHandle;
static osThreadId_t uartRxTaskHandle;
static osThreadId_t buttonTaskHandle;
static osThreadId_t ledTaskHandle;
static osThreadId_t monitorTaskHandle;

#define CONTROL_TASK_STACK_BYTES   768U
#define UART_RX_TASK_STACK_BYTES   768U
#define BUTTON_TASK_STACK_BYTES    512U
#define LED_TASK_STACK_BYTES       512U
#define MONITOR_TASK_STACK_BYTES  1024U

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

void AppTasks_Init(void)
{
    const osThreadAttr_t controlAttr = {
        .name = "ControlTask",
        .priority = APP_PRIORITY_CONTROL,
        .stack_size = CONTROL_TASK_STACK_BYTES
    };

    const osThreadAttr_t uartRxAttr = {
        .name = "UartRxTask",
        .priority = APP_PRIORITY_UART_RX,
        .stack_size = UART_RX_TASK_STACK_BYTES
    };

    const osThreadAttr_t buttonAttr = {
        .name = "ButtonTask",
        .priority = APP_PRIORITY_BUTTON,
        .stack_size = BUTTON_TASK_STACK_BYTES
    };

    const osThreadAttr_t ledAttr = {
        .name = "LedTask",
        .priority = APP_PRIORITY_LED,
        .stack_size = LED_TASK_STACK_BYTES
    };

    const osThreadAttr_t monitorAttr = {
        .name = "MonitorTask",
        .priority = APP_PRIORITY_MONITOR,
        .stack_size = MONITOR_TASK_STACK_BYTES
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

    (void)argument;

    UartMutex_Log(
        "\r\n"
        "========================================\r\n"
        " FreeRTOS Control System - Stage 5\r\n"
        " EVENT FLAGS\r\n"
        "========================================\r\n"
        "[EVENT] ControlTask publishes system state.\r\n"
        "[EVENT] LedTask waits for state changes.\r\n"
        "[STATE] IDLE\r\n"
        "[SYSTEM] Press SW1 to toggle IDLE / RUNNING.\r\n\r\n"
    );

    /*
     * Publish the initial state once the scheduler is running.
     */
    PublishState(systemState);

    for (;;)
    {
        if (CommandQueue_Receive(&command, osWaitForever) == osOK)
        {
            if ((command.type == APP_CMD_TOGGLE_RUN) &&
                (command.source == APP_CMD_SOURCE_BUTTON))
            {
                if (systemState == SYSTEM_STATE_IDLE)
                {
                    systemState = SYSTEM_STATE_RUNNING;

                    UartMutex_Log(
                        "[QUEUE] APP_CMD_TOGGLE_RUN received.\r\n"
                        "[STATE] IDLE -> RUNNING\r\n"
                    );
                }
                else
                {
                    systemState = SYSTEM_STATE_IDLE;

                    UartMutex_Log(
                        "[QUEUE] APP_CMD_TOGGLE_RUN received.\r\n"
                        "[STATE] RUNNING -> IDLE\r\n"
                    );
                }

                /*
                 * EVENT FLAGS PRODUCER:
                 * ControlTask tells LedTask that state changed.
                 */
                PublishState(systemState);
            }
        }
    }
}

static void UartRxTask(void *argument)
{
    (void)argument;

    for (;;)
    {
        osDelay(1000U);
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

    (void)argument;

    /*
     * LED behavior:
     *
     * IDLE:
     *   Green = slow blink
     *   Blue  = off
     *
     * RUNNING:
     *   Green = on
     *   Blue  = blink
     *
     * WARNING / FAULT:
     *   Reserved now; used in later stages.
     */
    for (;;)
    {
        /*
         * EVENT FLAGS CONSUMER:
         * Wait up to 250 ms. The timeout lets this task continue
         * animating LEDs even if no new state event arrives.
         */
        eventResult = SystemEvents_WaitForStateChange(250U);

        if ((eventResult & osFlagsError) == 0U)
        {
            activeEvents = SystemEvents_Get();
            currentStateFlag = activeEvents & EVENT_STATE_MASK;

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

    for (;;)
    {
        osDelay(5000U);

        UartMutex_Log(
            "[MONITOR] System alive. Mutex-protected UART.\r\n"
        );
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
