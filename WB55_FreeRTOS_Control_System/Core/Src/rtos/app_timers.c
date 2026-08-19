#include "rtos/app_timers.h"
#include "rtos/command_queue.h"

/*
 * =========================================================
 * FREERTOS CONCEPT: SOFTWARE TIMERS
 * =========================================================
 *
 * Complete Software Timer lifecycle shown here:
 *
 *   osTimerNew()    -> create periodic / one-shot timers
 *   osTimerStart()  -> start or restart a timer
 *   osTimerStop()   -> stop a running timer
 *
 * Timer callbacks are intentionally SHORT.
 *
 * Timer callback
 *      |
 *      v
 * CommandQueue
 *      |
 *      v
 * ControlTask
 *
 * This keeps the state-machine decision in one place.
 */

#define HEARTBEAT_PERIOD_MS      5000U
#define RUN_WARNING_DELAY_MS    10000U

static osTimerId_t heartbeatTimerHandle = NULL;
static osTimerId_t runWarningTimerHandle = NULL;

static void HeartbeatTimerCallback(void *argument);
static void RunWarningTimerCallback(void *argument);

static uint32_t AppTimers_MsToTicks(uint32_t milliseconds)
{
    uint32_t tickFrequency = osKernelGetTickFreq();

    /*
     * Round upward so the requested duration is not shortened.
     */
    return (uint32_t)(
        (((uint64_t)milliseconds * (uint64_t)tickFrequency) + 999ULL)
        / 1000ULL
    );
}

void AppTimers_Init(void)
{
    const osTimerAttr_t heartbeatAttr = {
        .name = "HeartbeatTimer"
    };

    const osTimerAttr_t runWarningAttr = {
        .name = "RunWarningTimer"
    };

    /*
     * PERIODIC TIMER:
     * Callback repeats after each period until stopped.
     */
    heartbeatTimerHandle = osTimerNew(
        HeartbeatTimerCallback,
        osTimerPeriodic,
        NULL,
        &heartbeatAttr
    );

    /*
     * ONE-SHOT TIMER:
     * Callback executes once and the timer stops automatically.
     */
    runWarningTimerHandle = osTimerNew(
        RunWarningTimerCallback,
        osTimerOnce,
        NULL,
        &runWarningAttr
    );
}

osStatus_t AppTimers_StartHeartbeat(void)
{
    if (heartbeatTimerHandle == NULL)
    {
        return osErrorParameter;
    }

    return osTimerStart(
        heartbeatTimerHandle,
        AppTimers_MsToTicks(HEARTBEAT_PERIOD_MS)
    );
}

osStatus_t AppTimers_StopHeartbeat(void)
{
    if (heartbeatTimerHandle == NULL)
    {
        return osErrorParameter;
    }

    if (osTimerIsRunning(heartbeatTimerHandle) == 0U)
    {
        return osOK;
    }

    return osTimerStop(heartbeatTimerHandle);
}

osStatus_t AppTimers_StartRunWarning(void)
{
    if (runWarningTimerHandle == NULL)
    {
        return osErrorParameter;
    }

    /*
     * Calling osTimerStart() again restarts the one-shot period.
     */
    return osTimerStart(
        runWarningTimerHandle,
        AppTimers_MsToTicks(RUN_WARNING_DELAY_MS)
    );
}

osStatus_t AppTimers_StopRunWarning(void)
{
    if (runWarningTimerHandle == NULL)
    {
        return osErrorParameter;
    }

    if (osTimerIsRunning(runWarningTimerHandle) == 0U)
    {
        return osOK;
    }

    return osTimerStop(runWarningTimerHandle);
}

static void HeartbeatTimerCallback(void *argument)
{
    AppCommand_t command;

    (void)argument;

    /*
     * Keep callback short:
     * do not print to UART here.
     */
    command.type = APP_CMD_TIMER_HEARTBEAT;
    command.source = APP_CMD_SOURCE_TIMER;

    (void)CommandQueue_Send(&command, 0U);
}

static void RunWarningTimerCallback(void *argument)
{
    AppCommand_t command;

    (void)argument;

    /*
     * ONE-SHOT timer expired.
     * It only reports the timeout; ControlTask decides what to do.
     */
    command.type = APP_CMD_RUN_TIMEOUT;
    command.source = APP_CMD_SOURCE_TIMER;

    (void)CommandQueue_Send(&command, 0U);
}

osTimerId_t AppTimers_GetHeartbeatHandle(void)
{
    return heartbeatTimerHandle;
}

osTimerId_t AppTimers_GetRunWarningHandle(void)
{
    return runWarningTimerHandle;
}
