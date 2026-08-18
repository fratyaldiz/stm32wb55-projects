# WB55_FreeRTOS_Control_System — Integrated FreeRTOS Control System (STM32WB55)

An embedded control-system project on the NUCLEO-WB55RG board that uses FreeRTOS
concepts **together inside one coherent architecture** — rather than as separate,
standalone demos. Tasks, priorities, a command queue, an interrupt-driven
semaphore, a UART mutex and event flags cooperate to drive a small state machine.

> **Status: In Development — Stage 5 (Event Flags).**
> The task/priority/queue/semaphore/mutex/event-flag backbone is implemented and
> tested. Higher-level behaviour (software timers, a UART command interface, final
> WARNING/FAULT handling, stack-monitor integration) is planned — see
> [Planned / Next Steps](#planned--next-steps).

Generated with STM32CubeMX, built with STM32CubeIDE 2.2.0, using **CMSIS-RTOS2**.

## Project Overview

The aim is to apply FreeRTOS in the way a real embedded product would: a set of
cooperating tasks with clear responsibilities and priorities, exchanging commands
through a queue and coordinating on shared state through event flags. A single
decision-maker task owns the system state; the rest react to it.

## Hardware

Verified against the `.ioc` and source:

- Board: **NUCLEO-WB55RG** (MB1355), MCU **STM32WB55RGV6**
- LEDs: **LD1 (blue) = PB5**, **LD2 (green) = PB0**
- Button: **SW1 = PC4**, GPIO **EXTI**, falling edge, pull-up
- UART: **USART1** on the ST-LINK Virtual COM Port — **PB6 = TX**, **PB7 = RX**,
  **115200 baud, 8-N-1**
- HAL time base: **TIM17**
- RTOS: FreeRTOS via **CMSIS-RTOS2**

## Architecture

The FreeRTOS building blocks are split into their own modules under `Core/*/rtos/`,
so each concept is easy to locate — while all of them run inside one system flow.

| Concept | Module |
|---|---|
| Tasks | `Core/Src/rtos/app_tasks.c` |
| Task priorities | `Core/Inc/rtos/task_priorities.h` |
| Command queue | `Core/Src/rtos/command_queue.c` / `.h` |
| Button binary semaphore | `Core/Src/rtos/button_semaphore.c` / `.h` |
| UART mutex | `Core/Src/rtos/uart_mutex.c` / `.h` |
| Event flags (system state) | `Core/Src/rtos/system_events.c` / `.h` |

### Tasks and priorities

| Task | Priority | Responsibility |
|---|---|---|
| `ControlTask` | High | Owns the system state; consumes commands and sets event flags |
| `UartRxTask` | AboveNormal | Handles UART input |
| `ButtonTask` | Normal | Waits on the button semaphore, sends commands to the queue |
| `LedTask` | Normal | Reacts to the current state via event flags |
| `MonitorTask` | Low | Background/diagnostic task |

Priorities follow how time-critical each job is: the control decision-maker runs
highest, UART input above the general tasks, and background monitoring lowest.

### System flow

```text
SW1
 │  (GPIO EXTI interrupt — ISR only releases the semaphore, no heavy work)
 ▼
Binary Semaphore  (button_semaphore)
 ▼
ButtonTask  ── APP_CMD_TOGGLE_RUN ──►  CommandQueue  (command_queue)
                                            ▼
                                       ControlTask     ← the state decision-maker
                                            ▼
                                       Event Flags  (system_events)
                                            ▼
                                        LedTask
```

- The **interrupt** does no real work: it only releases the binary semaphore to
  wake `ButtonTask`.
- `ButtonTask` translates the press into a command (`APP_CMD_TOGGLE_RUN`) and puts
  it on `CommandQueue`.
- `ControlTask` is the single **decision-maker**: it reads commands and updates the
  system state, publishing it through **event flags**.
- `LedTask` does not know where a state change came from (button, UART, or anything
  else) — it only reacts to the event flags. This decouples inputs from outputs.

### Shared UART (mutex with priority inheritance)

USART1 can be used by several tasks (`ControlTask`, `ButtonTask`, `MonitorTask`),
so access is protected by a mutex (`uart_mutex`). The mutex is created with
**`osMutexPrioInherit`**, so priority inheritance is enabled to avoid unbounded
priority inversion on the shared UART. Pattern: `osMutexAcquire()` → UART access →
`osMutexRelease()`.

## Current Behaviour (Stage 5)

The working, tested behaviour so far is the **IDLE ↔ RUNNING** state transition
driven by the physical button through the full flow above:

- **IDLE:** blue LED off, green LED blinking.
- **RUNNING:** green LED on, blue LED blinking.

The chain *Semaphore → Queue → state transition → Event Flags → LedTask* was
verified both on the LEDs and over the UART terminal.

System states are defined as **IDLE, RUNNING, WARNING, FAULT**; at this stage only
**IDLE ↔ RUNNING** is exercised. WARNING and FAULT are scaffolded but their final
behaviour is not implemented yet.

## Planned / Next Steps

Not yet implemented (do not assume these work yet):

- Software-timer integration
- UART command-line interface (start / stop / fault / reset commands)
- Final WARNING / FAULT behaviour
- Final task stack-monitor integration
- Full industrial-style control-system behaviour

## Build / Run

1. Open **STM32CubeIDE** and import this folder as an existing project.
2. Build and flash to the NUCLEO-WB55RG.
3. Open the ST-LINK COM port at **115200 8-N-1**; press **SW1** to toggle
   IDLE ↔ RUNNING and watch the LEDs and terminal output.

Clean rebuild (STM32CubeIDE 2.2.0 toolchain): **0 errors, 0 warnings**
(ELF `text 40472 / data 16 / bss 29816` bytes; `bss` includes the 24 KB FreeRTOS
heap).

## Repository Notes

- FreeRTOS: `configTOTAL_HEAP_SIZE = 24576`, `configCHECK_FOR_STACK_OVERFLOW = 2`.
- Build outputs (`Debug/`, `.elf`, `.map`, `.o`, `.d`, `.su`, `.cyclo`) are excluded
  via `.gitignore`; only source and project files are committed.
- For standalone, per-concept FreeRTOS experiments, see the sibling project
  [`WB55_FreeRTOS_Basics`](../WB55_FreeRTOS_Basics/README.md).
