# WB55_FreeRTOS_Basics — FreeRTOS Learning Laboratory (STM32WB55)

A hands-on FreeRTOS learning project on the NUCLEO-WB55RG board. Each core RTOS
concept is implemented and validated **stage by stage** against real peripherals
(UART, LEDs, a push button), so the concepts can be studied — and demonstrated —
one at a time rather than as abstract theory.

Internship / learning project. Generated with STM32CubeMX, built with STM32CubeIDE
2.2.0, using the **CMSIS-RTOS2** API on top of the FreeRTOS kernel.

## Project Overview

The goal of this project is to learn the fundamentals of FreeRTOS on STM32 by
building a small, focused experiment for each concept and observing its behaviour
over a UART terminal and the on-board LEDs.

Because it is organised as a progression of stages, the repository snapshot always
reflects the **most recently completed stage** as the actively running firmware,
while the code from earlier stages is kept in the project for reference. The
current active build demonstrates **task stack monitoring and stack-overflow
detection**; the earlier concepts below were each validated in their own stage.

## Hardware

- Board: **NUCLEO-WB55RG** (MB1355), MCU **STM32WB55RGV6**
- LEDs: **LD1 (blue) = PB5**, **LD2 (green) = PB0**
- Button: **SW1 = PC4**, GPIO **EXTI**, falling edge, pull-up
- UART: **USART1** on the ST-LINK Virtual COM Port — **PB6 = TX**, **PB7 = RX**,
  **115200 baud, 8-N-1**, no flow control
- HAL time base: **TIM17** (so SysTick is free for the RTOS)

## FreeRTOS Concepts Demonstrated

| Concept | How it is exercised | State in current build |
|---|---|---|
| Tasks & scheduler | Multiple `osThreadNew` tasks, `osDelay`, READY/RUNNING/BLOCKED | Active |
| Task priorities & preemption | Tasks created at different CMSIS priorities | Active |
| Queue | Button task sends a blink period to an LED task | Active |
| Binary semaphore | SW1 EXTI ISR releases a semaphore that unblocks a task | Active |
| GPIO EXTI → semaphore → task | ISR only signals; work is done in task context | Active |
| UART communication | `HAL_UART_Transmit` on `huart1`, strings + numeric values | Active |
| Shared UART synchronisation | A semaphore serialises access to the shared USART1 | Active |
| Mutex, priority inversion, priority inheritance | Low/Medium/High tasks + a mutex, observed over UART | Validated in an earlier stage (code retained, not active in the current build) |
| Event flags | Event-flag object created and signalled to represent state | Validated in an earlier stage (retained) |
| Software timers (periodic + one-shot) | `osTimerNew` periodic and one-shot timers | Validated in an earlier stage (timers created, callbacks passive in the current build) |
| Stack management & monitoring | Per-task stack high-water-mark via `osThreadGetStackSpace` | **Active (current stage)** |
| Stack overflow detection | `configCHECK_FOR_STACK_OVERFLOW = 2` + `vApplicationStackOverflowHook` | Active |

> The table is honest about the staged nature of the project: only the concepts
> marked *Active* run in the firmware image built from this snapshot. The others
> were implemented and verified in their own stage and left in the source as
> learning reference.

## Peripherals Used

- **GPIO** — LEDs (PB5, PB0) and the SW1 button (PC4, EXTI falling edge, pull-up)
- **USART1** — 115200 8-N-1 over the ST-LINK VCP for terminal output
- **TIM17** — HAL time base
- **NVIC / EXTI** — SW1 external interrupt feeding a binary semaphore

## Architecture / Learning Progression

The project grew as a sequence of stages, each adding one FreeRTOS mechanism on top
of the previous ones:

1. Tasks, scheduler and priorities (LED tasks blinking at task priority).
2. Queue — a button task pushes a blink period to an LED task.
3. Interrupt-driven binary semaphore — SW1 EXTI wakes a task (with debounce).
4. UART output and a semaphore that guards the shared USART1 resource.
5. Mutex with Low/Medium/High tasks to observe **priority inversion** and how a
   priority-inheritance mutex resolves it.
6. Event flags and software timers (periodic and one-shot).
7. **Stack management** — measuring per-task stack head-room and detecting overflow.

Each stage was tested on the board before moving on, so the codebase doubles as a
step-by-step reference.

## Stack Monitoring and Overflow Detection

The current active stage focuses on stack safety:

- **Monitoring:** a dedicated `MonitorTask` periodically prints each task's minimum
  free stack ("high-water-mark") over UART using `osThreadGetStackSpace()`. A small
  `volatile` demo buffer inside the button task shows how stack usage moves the
  high-water-mark.
- **Overflow detection:** FreeRTOS is configured with
  `configCHECK_FOR_STACK_OVERFLOW = 2` and implements `vApplicationStackOverflowHook`.
  Overflow was reproduced **only as a controlled test** by temporarily oversizing a
  task's local data; the code in this snapshot is back to a **safe** state, so no
  task is intentionally overflowing its stack.
- FreeRTOS heap: `configTOTAL_HEAP_SIZE = 24576` bytes — raised as more tasks and
  kernel objects were added; see `Core/Inc/FreeRTOSConfig.h`.

## Build / Run

1. Open **STM32CubeIDE** and import this folder as an existing project.
2. Build (`Project → Build Project`) and flash to the NUCLEO-WB55RG.
3. Open the ST-LINK COM port at **115200 8-N-1** to watch the task/stack output;
   press **SW1** to interact.

Clean rebuild (STM32CubeIDE 2.2.0 toolchain): **0 errors, 0 warnings**
(ELF `text 40428 / data 96 / bss 30320` bytes; `bss` includes the 24 KB FreeRTOS
heap).

## Repository Notes

- Build outputs (`Debug/`, `.elf`, `.map`, `.list`, `.o`, `.d`, `.su`, `.cyclo`) are
  excluded via `.gitignore`; only source and project files needed to rebuild are
  committed.
- When re-generating from the `.ioc` in CubeMX, keep user code inside the
  `/* USER CODE BEGIN … */ … /* USER CODE END … */` sections.
- For an **integrated** use of these same concepts inside a single control-system
  architecture, see the sibling project
  [`WB55_FreeRTOS_Control_System`](../WB55_FreeRTOS_Control_System/README.md).
