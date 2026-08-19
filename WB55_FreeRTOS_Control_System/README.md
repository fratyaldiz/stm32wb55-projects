# WB55_FreeRTOS_Control_System — Integrated FreeRTOS Control System (STM32WB55)

An embedded control-system project on the NUCLEO-WB55RG board that uses FreeRTOS
concepts **together inside one coherent architecture** — rather than as separate,
standalone demos. Tasks, priorities, a command queue, an interrupt-driven semaphore,
a UART mutex, event flags, software timers, an interrupt-driven UART command
interface and task stack monitoring cooperate to drive a small state machine.

> **Status: Complete through Stage 8 (Stack Monitor + Final Integration).**
> All eight stages are implemented and validated on real hardware.

This is an educational / portfolio embedded-firmware project with real hardware
validation — not a production, safety-certified, or commercial control system.

Generated with STM32CubeMX, built with STM32CubeIDE 2.2.0, using **CMSIS-RTOS2** on
top of the FreeRTOS kernel. Application logic is kept in modular `Core/*/rtos/`
files rather than in the generated `main.c`.

## Project Overview

The goal is to apply FreeRTOS the way a real embedded product would: a set of
cooperating tasks with clear responsibilities and priorities that exchange commands
through queues and coordinate on shared state through event flags. One task —
`ControlTask` — is the single owner of the application state machine. Buttons, the
UART command interface and software timers never touch LEDs or state directly; they
only produce commands/events onto queues that `ControlTask` consumes.

### Stage progression (all complete)

| Stage | Topic | Status |
|-------|-------|:------:|
| 1 | Tasks + priority | ✅ |
| 2 | Message queue | ✅ |
| 3 | Binary semaphore | ✅ |
| 4 | Mutex (priority inheritance) | ✅ |
| 5 | Event flags | ✅ |
| 6 | Software timers | ✅ |
| 7 | UART command interface | ✅ |
| 8 | Stack monitor + final integration | ✅ |

## Hardware

Verified against the `.ioc` and source:

- Board: **NUCLEO-WB55RG** (MB1355), MCU **STM32WB55RGV6**
- LEDs: **LD1 (blue) = PB5**, **LD2 (green) = PB0**
- Button: **SW1 = PC4**, active-low with pull-up, GPIO **EXTI4** falling edge
- UART: **USART1** on the ST-LINK Virtual COM Port — **PB6 = TX**, **PB7 = RX**,
  **115200 baud, 8-N-1**, no flow control; USART1 global interrupt enabled (RX is
  interrupt-driven)
- HAL time base: **TIM17** (SysTick left to the RTOS)
- RTOS: FreeRTOS via **CMSIS-RTOS2**

## Final Architecture

```text
Physical SW1
    |
    v
GPIO EXTI Interrupt
    |
    v
Binary Semaphore
    |
    v
ButtonTask
    |
    +-------------------+
                        |
USART1 RX Interrupt     |
    |                   |
    v                   |
RxByteQueue             |
    |                   |
    v                   |
UartRxTask              |
    |                   |
    +-----> CommandQueue +-----> ControlTask
                                  ^
                                  |
Software Timer callbacks ---------+
                                  |
                                  v
                            State Machine
                                  |
                                  v
                             Event Flags
                                  |
                                  v
                               LedTask

Multiple tasks --> UartMutex --> USART1
MonitorTask     --> task stack monitoring
```

**Design decision:** `ControlTask` is the single owner of the state machine.
`ButtonTask`, `UartRxTask` and the software-timer callbacks do not change LEDs or
state directly — they produce commands/events through queues.

Two clearly distinct primitives are used:

- **Binary semaphore** — "an event happened / wake this task".
- **Queue** — "carry a command/data item".

## Application Tasks and Priorities

Priorities are defined in `Core/Inc/rtos/task_priorities.h`:

| Task | Priority | Responsibility |
|------|----------|----------------|
| `ControlTask` | High (`osPriorityHigh`) | Owns the state machine; consumes `CommandQueue`; validates transitions |
| `UartRxTask` | AboveNormal | Parses complete terminal commands, forwards them as commands (no direct state change) |
| `ButtonTask` | Normal | Wakes on the button semaphore after SW1 EXTI, debounces, sends `APP_CMD_TOGGLE_RUN` |
| `LedTask` | Normal | Reacts to state via event flags; drives LD1/LD2 |
| `MonitorTask` | Low | Periodically checks stack watermarks; quiet in normal operation |

Task implementations live in `Core/Src/rtos/app_tasks.c`.

## State Machine

States: **IDLE, RUNNING, WARNING, FAULT**. `ControlTask` is the only place that
changes state, and it validates every transition.

| Command | Behaviour |
|---------|-----------|
| `start` | IDLE → RUNNING (starts the 10 s RunWarningTimer). Ignored if already RUNNING. Rejected in WARNING/FAULT (reset required first). |
| `stop` | RUNNING → IDLE (cancels RunWarningTimer); WARNING → IDLE. Ignored if already IDLE. Rejected in FAULT (reset required). |
| `warn` | Valid only while RUNNING: RUNNING → WARNING (stops RunWarningTimer). Rejected otherwise. |
| `fault` | Any non-FAULT state → FAULT (stops RunWarningTimer if running). |
| `reset` | WARNING → IDLE, FAULT → IDLE. Ignored if no WARNING/FAULT. |
| `status` | Prints current state and uptime. |
| `stack` | Prints per-task stack diagnostics. |
| `help` | Prints the available commands. |

The one-shot RunWarningTimer provides an automatic RUNNING → WARNING transition if
RUNNING persists for 10 s without operator action.

## Module Mapping

The project is intentionally modular so each FreeRTOS concept is easy to locate:

| Concept | Module |
|---------|--------|
| Tasks | `Core/Src/rtos/app_tasks.c` |
| Priorities | `Core/Inc/rtos/task_priorities.h` |
| Message queue | `Core/Src/rtos/command_queue.c` |
| Binary semaphore | `Core/Src/rtos/button_semaphore.c` |
| UART mutex | `Core/Src/rtos/uart_mutex.c` |
| Event flags | `Core/Src/rtos/system_events.c` |
| Software timers | `Core/Src/rtos/app_timers.c` |
| UART command interface | `Core/Src/rtos/uart_cli.c` |
| Stack management | `Core/Src/rtos/stack_monitor.c` |

## SW1 Button Behaviour

The SW1 path remains fully operational alongside the UART interface:

```text
SW1 -> EXTI -> Binary Semaphore -> ButtonTask -> CommandQueue -> ControlTask
```

The EXTI ISR is short: it only releases the semaphore. `ButtonTask` debounces and
sends `APP_CMD_TOGGLE_RUN`. Observed behaviour:

- IDLE + SW1 → RUNNING
- RUNNING + SW1 → IDLE
- WARNING + SW1 → IDLE / warning acknowledgement
- FAULT + SW1 → rejected (FAULT cannot be bypassed with the button)

Only SW1 is used; SW2/SW3 are intentionally not part of this project.

## UART Command Interface

USART1 RX is interrupt-driven. `HAL_UART_RxCpltCallback()` only stores the received
byte in an internal byte queue and re-arms `HAL_UART_Receive_IT()`; no parsing or
state logic runs in interrupt context. `UartRxTask` assembles complete lines and
forwards them to `ControlTask` through `CommandQueue`.

```text
USART1 RX ISR -> RxByteQueue -> UartRxTask -> parse line -> CommandQueue -> ControlTask
```

Supported commands: `start`, `stop`, `warn`, `fault`, `reset`, `status`, `stack`,
`help`.

## Software Timer Behaviour

Software timers are in `Core/Src/rtos/app_timers.c`. Callbacks are short — they only
post a command onto `CommandQueue`; they never block on UART, touch LEDs, or make
state decisions.

- **HeartbeatTimer** — periodic, 5 s. Posts `APP_CMD_TIMER_HEARTBEAT`.
  During Stage 6 testing this printed a `[TIMER] HeartbeatTimer expired …` line every
  5 seconds. In the final build that repetitive log is **intentionally silent** to
  keep the console clean: the timer and its queue-based path remain fully active, and
  `ControlTask` consumes the heartbeat event without emitting UART output.
- **RunWarningTimer** — one-shot, 10 s. Started when the system enters RUNNING; if
  RUNNING lasts 10 s it posts `APP_CMD_RUN_TIMEOUT`, which `ControlTask` turns into
  RUNNING → WARNING. It is stopped whenever the system leaves RUNNING early (stop,
  warn, or fault).

## Stack Monitoring

Stack diagnostics are in `Core/Src/rtos/stack_monitor.c`, using
`osThreadGetStackSpace()`. Task stack sizes have a **single source of truth** in
`Core/Inc/rtos/app_tasks.h`:

| Task | Stack |
|------|-------|
| ControlTask | 1024 B |
| UartRxTask | 1024 B |
| ButtonTask | 512 B |
| LedTask | 512 B |
| MonitorTask | 1024 B |

The `stack` command prints, per task, the total size, the minimum historical free
stack (high-water-mark), and the approximate maximum used (`total − min free`).
`MonitorTask` checks the watermarks periodically and only warns if a task's minimum
free stack falls below the **128 B** threshold — normal operation stays quiet. This
project does **not** deliberately overflow a stack; that experiment belongs to the
sibling `WB55_FreeRTOS_Basics` project.

Hardware-measured values from the final test:

| Task | Total | Min free | Max used (≈) |
|------|------:|---------:|-------------:|
| ControlTask | 1024 B | 472 B | 552 B |
| UartRxTask | 1024 B | 480 B | 544 B |
| ButtonTask | 512 B | 300 B | 212 B |
| LedTask | 512 B | 232 B | 280 B |
| MonitorTask | 1024 B | 704 B | 320 B |

## LED Behaviour

`ControlTask` publishes the state via event flags (`Core/Src/rtos/system_events.c`);
`LedTask` consumes them. Exactly one state is active at a time.

| State | LD1 (blue) | LD2 (green) |
|-------|------------|-------------|
| IDLE | off | blinking |
| RUNNING | blinking | on |
| WARNING | alternating rapidly with green | alternating rapidly with blue |
| FAULT | blinking together rapidly | blinking together rapidly |

## FreeRTOS Concepts Demonstrated

Tasks & scheduler, task priorities & preemption, a message queue, a binary semaphore
driven from an EXTI interrupt, a mutex with **priority inheritance**
(`osMutexPrioInherit`) guarding shared UART output, event flags for state
publication, periodic and one-shot software timers with short queue-posting
callbacks, an interrupt-driven UART command interface, and task stack-watermark
monitoring.

## Build / Run

1. Open **STM32CubeIDE** and import this folder as an existing project.
2. Build and flash to the NUCLEO-WB55RG.
3. Open the ST-LINK COM port at **115200 8-N-1**. Type `help` for commands, or press
   **SW1** to toggle IDLE ↔ RUNNING.

Clean rebuild (STM32CubeIDE 2.2.0 toolchain): **0 errors, 0 warnings**
(ELF `text 53556 / data 96 / bss 30160` bytes; `bss` includes the 24 KB FreeRTOS
heap).

## Example Terminal Session

```text
> status
State: IDLE
> start
[STATE] IDLE -> RUNNING
> status
State: RUNNING
> warn
[STATE] RUNNING -> WARNING
> reset
[STATE] WARNING -> IDLE
> fault
[STATE] IDLE -> FAULT
> start
[CONTROL] START rejected: reset WARNING/FAULT first.
> reset
[STATE] FAULT -> IDLE
> status
State: IDLE
```

## Hardware-Tested Validation

The following were verified on the physical board (not merely planned):

- SW1 interrupt → semaphore → task flow
- Queue command flow
- UART command parsing for all eight commands
- IDLE / RUNNING / WARNING / FAULT state machine
- START rejection while in WARNING/FAULT
- reset recovery from WARNING and FAULT
- event-flag → LED state propagation
- periodic HeartbeatTimer and one-shot RunWarningTimer (including timeout and early
  cancellation)
- stack-watermark monitoring via the `stack` command

## Repository Notes

- FreeRTOS: `configTOTAL_HEAP_SIZE = 24576`, `configCHECK_FOR_STACK_OVERFLOW = 2`.
- Build outputs (`Debug/`, `.elf`, `.map`, `.o`, `.d`, `.su`, `.cyclo`) are excluded
  via `.gitignore`; only source and project files are committed.
- For standalone, per-concept FreeRTOS experiments (including the deliberate
  stack-overflow test), see the sibling project
  [`WB55_FreeRTOS_Basics`](../WB55_FreeRTOS_Basics/README.md).
