#ifndef STACK_MONITOR_H
#define STACK_MONITOR_H

/*
 * =========================================================
 * FREERTOS CONCEPT: STACK MANAGEMENT
 * =========================================================
 *
 * Uses CMSIS-RTOS2 osThreadGetStackSpace() to inspect each
 * task's historical minimum remaining stack space.
 *
 * "min free" means:
 * the smallest amount of unused stack ever observed for the
 * task since it started (high-water-mark style information).
 */

void StackMonitor_PrintReport(void);

void StackMonitor_CheckThresholds(void);

#endif /* STACK_MONITOR_H */
