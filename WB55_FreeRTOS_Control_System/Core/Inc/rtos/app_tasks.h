#ifndef APP_TASKS_H
#define APP_TASKS_H

#include "cmsis_os2.h"

/*
 * =========================================================
 * FREERTOS CONCEPT: TASKS + STACK SIZES
 * =========================================================
 *
 * Stack sizes are defined here so both app_tasks.c and
 * stack_monitor.c use the same single source of truth.
 */

#define APP_CONTROL_TASK_STACK_BYTES   1024U
#define APP_UART_RX_TASK_STACK_BYTES   1024U
#define APP_BUTTON_TASK_STACK_BYTES     512U
#define APP_LED_TASK_STACK_BYTES        512U
#define APP_MONITOR_TASK_STACK_BYTES   1024U

void AppTasks_Init(void);

osThreadId_t AppTasks_GetControlTaskHandle(void);
osThreadId_t AppTasks_GetUartRxTaskHandle(void);
osThreadId_t AppTasks_GetButtonTaskHandle(void);
osThreadId_t AppTasks_GetLedTaskHandle(void);
osThreadId_t AppTasks_GetMonitorTaskHandle(void);

#endif /* APP_TASKS_H */
