#ifndef APP_TIMERS_H
#define APP_TIMERS_H

#include "cmsis_os2.h"
#include <stdint.h>

/*
 * =========================================================
 * FREERTOS CONCEPT: SOFTWARE TIMERS
 * =========================================================
 *
 * Two timers are used in this stage:
 *
 * 1) HeartbeatTimer - PERIODIC
 *    Expires every 5 seconds.
 *
 * 2) RunWarningTimer - ONE-SHOT
 *    Starts when the system enters RUNNING.
 *    If RUNNING continues for 10 seconds, it expires once.
 *
 * IMPORTANT:
 * Timer callbacks do NOT make system-state decisions and do
 * NOT perform blocking UART transmissions.
 *
 * They only send a lightweight command through CommandQueue.
 * ControlTask remains the single owner of the state machine.
 */

void AppTimers_Init(void);

osStatus_t AppTimers_StartHeartbeat(void);
osStatus_t AppTimers_StopHeartbeat(void);

osStatus_t AppTimers_StartRunWarning(void);
osStatus_t AppTimers_StopRunWarning(void);

osTimerId_t AppTimers_GetHeartbeatHandle(void);
osTimerId_t AppTimers_GetRunWarningHandle(void);

#endif /* APP_TIMERS_H */
