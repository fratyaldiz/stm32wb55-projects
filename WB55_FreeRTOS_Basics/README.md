# WB55_FreeRTOS_Basics — FreeRTOS / CMSIS-RTOS2 Learning Project

Hands-on FreeRTOS examples on the NUCLEO-WB55RG board: tasks, queues,
interrupt-driven semaphores, a shared-resource mutex, and live demos of
**priority inversion** vs **priority inheritance** — all observed over a UART
terminal.

Internship / learning project. Generated with STM32CubeMX, built with STM32CubeIDE.
CMSIS-RTOS2 API on top of the FreeRTOS kernel.

## Hardware

- Board: **NUCLEO-WB55RG** (MB1355), MCU **STM32WB55RGV6**
- Button: **SW1 = PC4** (EXTI, falling edge)
- On-board LEDs used by the LED tasks (blue / green)
- UART: **USART1** routed to the ST-LINK Virtual COM Port
  - **PB6 = USART1_TX**, **PB7 = USART1_RX**
  - **115200 baud, 8-N-1, no flow control**

> UART note: the ST-LINK VCP on this board is wired to **USART1** (`huart1`),
> **not** LPUART1. The project uses `HAL_UART_Transmit(&huart1, …)`.

## Software / Tools

- STM32CubeMX (project generation, `.ioc`)
- STM32CubeIDE 2.2.0 (build + flash + debug)
- FreeRTOS kernel via the **CMSIS-RTOS2** wrapper (`osThreadNew`, `osMessageQueue*`,
  `osSemaphore*`, `osMutex*`, `osDelay`)
- Any serial terminal (115200 8-N-1) on the ST-LINK COM port

## Topics Covered

### 1. Tasks & Scheduler
- `BlueLedTask`, `GreenLedTask` created with `osThreadNew` at different priorities.
- Illustrates the scheduler and READY / RUNNING / BLOCKED states; tasks yield the
  CPU with `osDelay()`.

### 2. Queue
- `ButtonQueue` carries a blink period from `ButtonTask` to `BlueLedTask`.
- Button presses toggle the LED blink mode between **500 ms** and **100 ms**.

### 3. Interrupt + Binary Semaphore
- **SW1 / PC4** EXTI, falling edge. `HAL_GPIO_EXTI_Callback()` releases
  `ButtonSemaphore` from the ISR.
- `ButtonTask` blocks on `osSemaphoreAcquire(ButtonSemaphore, …)`.
- Software **debounce**, so one physical press produces a single action even when
  the button is held.

### 4. UART Output
- `HAL_UART_Transmit()` on `huart1` sends strings and numeric values converted to
  ASCII, viewed on the terminal.

### 5. Shared-resource Binary Semaphore
- `UartSemaphore` guards the shared **USART1** resource so `BlueLedTask` and
  `GreenLedTask` never interleave their output.
- `osSemaphoreAcquire()` before printing, `osSemaphoreRelease()` after.

### 6. Priority Inversion (binary semaphore)
- `LowTask` = *BelowNormal*, `MediumTask` = *Normal*, `HighTask` = *AboveNormal*,
  sharing `ResourceSemaphore`.
- LOW takes the resource; HIGH waits for it; MEDIUM preempts LOW, making HIGH wait
  even longer — classic **priority inversion**, visible on the terminal.

### 7. Mutex + Priority Inheritance
- Same three tasks, but the shared resource is a CMSIS-RTOS2 **mutex**
  (`ResourceMutex`, created with `osMutexPrioInherit`).
- While HIGH blocks on the mutex, LOW temporarily inherits a higher priority, so
  MEDIUM can no longer delay LOW — **priority inheritance** solves the inversion.

Example terminal output of the mutex demo:

```text
=== PRIORITY INHERITANCE DEMO - MUTEX ===
[LOW] Mutex acquired. Starting low-priority work.
[HIGH] Wants the mutex. HIGH will BLOCK for a short time.
[LOW] Priority inheritance kept LOW ahead of MEDIUM.
[LOW] Work finished. Releasing mutex now.
[HIGH] Mutex acquired! MEDIUM could not delay LOW.
=== END OF MUTEX / PRIORITY INHERITANCE DEMO ===

[MEDIUM] Now running, but only AFTER LOW released the mutex.
[MEDIUM] CPU work finished.
```

## FreeRTOS Heap

Adding the extra demo tasks needed more kernel heap, so the CubeMX FreeRTOS
`configTOTAL_HEAP_SIZE` was raised to **24576 bytes (24 KB)** — see
`Core/Inc/FreeRTOSConfig.h`. Keep this value; a smaller heap makes task/object
creation fail.

## How to Build

1. Open **STM32CubeIDE** and import this folder as an existing project
   (`File → Open Projects from File System…` → select `WB55_FreeRTOS_Basics`).
2. Build (`Project → Build Project`) and flash to the NUCLEO-WB55RG.

Only source and project files needed to rebuild are committed. Build outputs
(`Debug/`, `.elf`, `.map`, `.list`, `.o`, `.d`, `.su`, `.cyclo`, …) are excluded via
`.gitignore`.

> When re-generating from the `.ioc` in CubeMX, keep all user code inside the
> `/* USER CODE BEGIN … */ … /* USER CODE END … */` sections.

## How to Test

1. Flash the firmware and open the ST-LINK COM port at **115200 8-N-1**.
2. Watch the task output; press **SW1** to change the LED blink period.
3. Observe the priority-inversion and priority-inheritance demos printed on the
   terminal.

## Current Status

- FreeRTOS tasks, queue, interrupt semaphore, shared UART semaphore, priority
  inversion and mutex priority-inheritance demos — all working on the board.
- **Build:** `0 errors, 0 warnings` (clean rebuild, STM32CubeIDE 2.2.0 toolchain).
  ELF links (`text 35720 / data 96 / bss 30296` bytes; `bss` includes the 24 KB
  FreeRTOS heap).
