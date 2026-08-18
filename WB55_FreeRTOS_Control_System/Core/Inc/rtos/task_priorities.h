#ifndef TASK_PRIORITIES_H
#define TASK_PRIORITIES_H

#include "cmsis_os2.h"

/*
 * FREERTOS CONCEPT: TASK PRIORITIES
 *
 * ControlTask  : High        -> central state/fault decisions
 * UartRxTask   : AboveNormal -> incoming commands should be handled quickly
 * ButtonTask   : Normal      -> user input
 * LedTask      : Normal      -> status indication
 * MonitorTask  : Low         -> diagnostics can wait
 */
#define APP_PRIORITY_CONTROL   osPriorityHigh
#define APP_PRIORITY_UART_RX   osPriorityAboveNormal
#define APP_PRIORITY_BUTTON    osPriorityNormal
#define APP_PRIORITY_LED       osPriorityNormal
#define APP_PRIORITY_MONITOR   osPriorityLow

#endif
