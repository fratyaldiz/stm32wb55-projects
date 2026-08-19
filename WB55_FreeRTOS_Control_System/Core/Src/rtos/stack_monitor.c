#include "rtos/stack_monitor.h"
#include "rtos/app_tasks.h"
#include "rtos/uart_mutex.h"

#include "cmsis_os2.h"

#include <stdio.h>

/*
 * =========================================================
 * FREERTOS CONCEPT: STACK MANAGEMENT
 * =========================================================
 *
 * Presentation file:
 *   Core/Src/rtos/stack_monitor.c
 *
 * Key CMSIS-RTOS2 API:
 *   osThreadGetStackSpace()
 *
 * The returned value is used as a stack watermark:
 *
 *   total stack
 *       -
 *   minimum free stack
 *       =
 *   approximate maximum stack used
 */

#define STACK_WARNING_THRESHOLD_BYTES  128U

typedef struct
{
    const char *name;
    osThreadId_t handle;
    uint32_t totalBytes;
} StackTaskInfo_t;

static void StackMonitor_PrintOne(const StackTaskInfo_t *taskInfo)
{
    uint32_t minFreeBytes;
    uint32_t maxUsedBytes;
    char line[120];

    if ((taskInfo == NULL) || (taskInfo->handle == NULL))
    {
        return;
    }

    minFreeBytes = osThreadGetStackSpace(taskInfo->handle);

    if (minFreeBytes <= taskInfo->totalBytes)
    {
        maxUsedBytes = taskInfo->totalBytes - minFreeBytes;
    }
    else
    {
        maxUsedBytes = 0U;
    }

    (void)snprintf(
        line,
        sizeof(line),
        "%-11s total=%4lu B | min free=%4lu B | max used~=%4lu B\r\n",
        taskInfo->name,
        (unsigned long)taskInfo->totalBytes,
        (unsigned long)minFreeBytes,
        (unsigned long)maxUsedBytes
    );

    UartMutex_Log(line);
}

void StackMonitor_PrintReport(void)
{
    const StackTaskInfo_t taskList[] = {
        {
            "ControlTask",
            AppTasks_GetControlTaskHandle(),
            APP_CONTROL_TASK_STACK_BYTES
        },
        {
            "UartRxTask",
            AppTasks_GetUartRxTaskHandle(),
            APP_UART_RX_TASK_STACK_BYTES
        },
        {
            "ButtonTask",
            AppTasks_GetButtonTaskHandle(),
            APP_BUTTON_TASK_STACK_BYTES
        },
        {
            "LedTask",
            AppTasks_GetLedTaskHandle(),
            APP_LED_TASK_STACK_BYTES
        },
        {
            "MonitorTask",
            AppTasks_GetMonitorTaskHandle(),
            APP_MONITOR_TASK_STACK_BYTES
        }
    };

    uint32_t i;

    UartMutex_Log(
        "\r\n"
        "=============== STACK STATUS ===============\r\n"
    );

    for (i = 0U; i < (sizeof(taskList) / sizeof(taskList[0])); i++)
    {
        StackMonitor_PrintOne(&taskList[i]);
    }

    UartMutex_Log(
        "============================================\r\n"
        "min free = historical minimum unused stack\r\n"
        "max used~= total - min free\r\n\r\n"
    );
}

void StackMonitor_CheckThresholds(void)
{
    const StackTaskInfo_t taskList[] = {
        {
            "ControlTask",
            AppTasks_GetControlTaskHandle(),
            APP_CONTROL_TASK_STACK_BYTES
        },
        {
            "UartRxTask",
            AppTasks_GetUartRxTaskHandle(),
            APP_UART_RX_TASK_STACK_BYTES
        },
        {
            "ButtonTask",
            AppTasks_GetButtonTaskHandle(),
            APP_BUTTON_TASK_STACK_BYTES
        },
        {
            "LedTask",
            AppTasks_GetLedTaskHandle(),
            APP_LED_TASK_STACK_BYTES
        },
        {
            "MonitorTask",
            AppTasks_GetMonitorTaskHandle(),
            APP_MONITOR_TASK_STACK_BYTES
        }
    };

    uint32_t i;
    uint32_t minFreeBytes;
    char warning[100];

    /*
     * MonitorTask calls this periodically.
     * It stays quiet while all tasks have enough stack.
     */
    for (i = 0U; i < (sizeof(taskList) / sizeof(taskList[0])); i++)
    {
        if (taskList[i].handle == NULL)
        {
            continue;
        }

        minFreeBytes = osThreadGetStackSpace(taskList[i].handle);

        if (minFreeBytes < STACK_WARNING_THRESHOLD_BYTES)
        {
            (void)snprintf(
                warning,
                sizeof(warning),
                "[STACK WARNING] %s min free = %lu B\r\n",
                taskList[i].name,
                (unsigned long)minFreeBytes
            );

            UartMutex_Log(warning);
        }
    }
}
