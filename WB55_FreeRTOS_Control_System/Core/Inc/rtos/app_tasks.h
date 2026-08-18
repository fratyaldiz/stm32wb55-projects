#ifndef APP_TASKS_H
#define APP_TASKS_H

#include "cmsis_os2.h"

/* FREERTOS CONCEPT: TASKS */
void AppTasks_Init(void);

osThreadId_t AppTasks_GetControlTaskHandle(void);
osThreadId_t AppTasks_GetUartRxTaskHandle(void);
osThreadId_t AppTasks_GetButtonTaskHandle(void);
osThreadId_t AppTasks_GetLedTaskHandle(void);
osThreadId_t AppTasks_GetMonitorTaskHandle(void);

#endif
