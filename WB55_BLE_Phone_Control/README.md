# WB55_BLE_Phone_Control — STM32WB55 BLE Phone Control + Health Thermometer

BLE firmware for the NUCLEO-WB55RG board. A phone (ST BLE Toolbox on iPhone)
connects over Bluetooth Low Energy and:

- controls the three on-board LEDs (phone → MCU),
- receives notifications when the three user buttons are pressed (MCU → phone),
- reads the MCU's **internal die temperature** through the standard Bluetooth
  Health Thermometer Service,
- reads on-chip **diagnostics** (VDDA, uptime, last reset reason) over dedicated
  BLE characteristics, backed by an **independent watchdog (IWDG)** for recovery.

Internship / learning project. Generated with STM32CubeMX, built with STM32CubeIDE.

## Project Overview

The firmware runs two BLE features at the same time on one connection:

1. A **Custom P2P Service** for LED control (`FE41`) and button notifications (`FE42`),
   plus a full on-chip **diagnostics** set: VDDA (`FE43`), uptime (`FE44`), reset
   reason (`FE45`), connection stats (`FE46`), button press counters (`FE47`),
   software reset (`FE48`), connection duration (`FE49`) and clear diagnostics (`FE4A`).
2. The **Bluetooth SIG Health Thermometer Service (0x1809)** that reports the
   real MCU internal temperature as a proper temperature value (e.g. `31.0 °C`)
   instead of raw HEX.
3. The **Device Information Service** (manufacturer, model, HW/FW revision) and an
   **independent watchdog (IWDG)** that resets the MCU if the main loop stalls.

The device advertises as **`WB55_CTRL`**.

## Hardware

- Board: **NUCLEO-WB55RG** (MB1355), MCU **STM32WB55RGV6** (Cortex-M4 + Cortex-M0+ radio)
- On-board LEDs: **LD1 = PB5**, **LD2 = PB0**, **LD3 = PB1**
- On-board user buttons: **SW1 = PC4**, **SW2 = PD0**, **SW3 = PD1**
- Temperature source: internal temperature sensor sampled by **ADC1** (no external
  hardware required)
- Phone side: iPhone running **ST BLE Toolbox**

## Software / Tools

- STM32CubeMX (project generation, `.ioc`)
- STM32CubeIDE 2.2.0 (build + flash + debug)
- STM32WB BLE stack: **P2P Server** + **Health Thermometer** application, running
  with the wireless (M0+) firmware on the WB55
- ST BLE Toolbox (iOS) for testing

## BLE Architecture

```text
iPhone / ST BLE Toolbox
         │
         │ BLE
         ▼
STM32 NUCLEO-WB55RG

FE41:
Phone → MCU
LD1 / LD2 / LD3 control

FE42:
MCU → Phone
SW1 / SW2 / SW3 notifications

FE43..FE4A (diagnostics):
VDDA / uptime / reset reason / conn stats /
button counters / SW reset / conn duration / clear

Health Thermometer Service:
MCU internal temperature → °C

Device Information Service + IWDG watchdog
```

Advertising name / device name: **`WB55_CTRL`**.
P2P Server and Health Thermometer run **simultaneously** on the same connection.

## P2P Service

Custom P2P service characteristics:

| Characteristic | Direction    | Purpose                         | Properties                                |
|----------------|--------------|---------------------------------|-------------------------------------------|
| `FE41`         | Phone → MCU  | LED control (LD1/LD2/LD3)       | Read, Write, Write Without Response       |
| `FE42`         | MCU → Phone  | Button notifications (SW1/2/3)  | Notify                                    |
| `FE43`         | MCU → Phone  | VDDA in mV                      | Read                                      |
| `FE44`         | MCU → Phone  | Uptime in seconds               | Read                                      |
| `FE45`         | MCU → Phone  | Last reset reason (bitmask)     | Read                                      |
| `FE46`         | MCU → Phone  | Connection stats                | Read                                      |
| `FE47`         | MCU → Phone  | Button press counters           | Read                                      |
| `FE48`         | Phone → MCU  | Software reset (magic command)  | Write                                     |
| `FE49`         | MCU → Phone  | Current connection duration     | Read                                      |
| `FE4A`         | Phone → MCU  | Clear diagnostics (magic cmd)   | Write                                     |

## LED Command Protocol

Write 2 bytes to **`FE41`** — first byte = LED id, second byte = state:

| Bytes    | Action    |
|----------|-----------|
| `01 01`  | LD1 ON    |
| `01 00`  | LD1 OFF   |
| `02 01`  | LD2 ON    |
| `02 00`  | LD2 OFF   |
| `03 01`  | LD3 ON    |
| `03 00`  | LD3 OFF   |

`FE41` supports **Read + Write + Write Without Response**.

## Button Notification Protocol

The firmware polls the three buttons with software **debounce** and sends a 2-byte
notification on **`FE42`** — first byte = button id, second byte = pressed/released:

| Bytes    | Meaning        |
|----------|----------------|
| `01 01`  | SW1 pressed    |
| `01 00`  | SW1 released   |
| `02 01`  | SW2 pressed    |
| `02 00`  | SW2 released   |
| `03 01`  | SW3 pressed    |
| `03 00`  | SW3 released   |

Note: the earlier `04XX` temperature packet on `FE42` was **removed**. `FE42` now
carries only SW1/SW2/SW3 notifications; temperature is reported through the Health
Thermometer Service instead.

## Diagnostics Characteristics (FE43 – FE4A)

On-chip diagnostics characteristics. All multi-byte values are **little-endian**.

- **`FE43` — VDDA**: `uint16`, millivolts. Computed from VREFINT (see below).
  Example: `EC 0C` = `0x0CEC` = 3308 mV = **3.308 V**.
- **`FE44` — Uptime**: `uint32`, seconds since boot.
- **`FE45` — Reset Reason**: `uint16` bitmask. The MCU captures the RCC reset flags
  once at boot, clears them, and writes the value into the `FE45` GATT attribute:

  | Bit | Flag      | Meaning                    |
  |-----|-----------|----------------------------|
  | 0   | `PINRST`  | Pin (NRST) reset           |
  | 1   | `BORRST`  | Brown-out reset            |
  | 2   | `SFTRST`  | Software reset             |
  | 3   | `IWDGRST` | Independent watchdog reset |
  | 4   | `WWDGRST` | Window watchdog reset      |
  | 5   | `LPWRRST` | Low-power reset            |
  | 6   | `OBLRST`  | Option-byte loader reset   |

  Examples: `01 00` = `0x0001` = `PINRST` (normal power-up / button reset).
  `09 00` = `0x0009` = `PINRST | IWDGRST` (recovery after a watchdog timeout).
  `05 00` = `0x0005` = `PINRST | SFTRST` (after an `FE48` software reset).
- **`FE46` — Connection Stats** (read): active connection state, total connection
  count and total disconnection count.
- **`FE47` — Button Press Counters** (read): debounced press counters for
  SW1 / SW2 / SW3, each `uint16`. Kept in RAM until the next MCU reset.
- **`FE48` — Software Reset** (write-only): accepts only the 2-byte magic command
  **`A5 5A`**; after a 200 ms delay it calls `NVIC_SystemReset()`. Any other value is
  ignored. After the reset `FE45` reads `05 00` (`SFTRST`).
- **`FE49` — Connection Duration** (read): `uint32`, seconds. Reset to 0 at the start
  of every BLE connection; increases while connected.
- **`FE4A` — Clear Diagnostics** (write-only): accepts only the 2-byte magic command
  **`C3 3C`**; clears the connection/disconnection counts and the SW1/SW2/SW3 press
  counters **without** resetting the MCU. The active connection state, `FE49`
  connection duration and `FE45` reset reason are **preserved**.

## Watchdog (IWDG) Recovery

An **Independent Watchdog** protects against a stalled main loop:

- Prescaler `64`, Window `4095`, Reload `4095` → nominal timeout ≈ **8.2 s**.
- Production firmware feeds it with `HAL_IWDG_Refresh(&hiwdg)` in the main loop.
- If the loop stops refreshing, the IWDG resets the MCU; the next boot reports the
  event through `FE45` (`IWDGRST` bit set).

> The temporary SW1-triggered starvation test used to validate the watchdog was
> removed. Production firmware has **no** SW1 watchdog control.

## Device Information Service

Standard BLE **Device Information Service** exposes:

| Field             | Value                |
|-------------------|----------------------|
| Manufacturer      | `STMicroelectronics` |
| Model             | `NUCLEO-WB55RG`      |
| Hardware Revision | `MB1355D-01`         |
| Firmware Revision | `1.0.0`              |

## Health Thermometer Service

Standard Bluetooth SIG **Health Thermometer Service (UUID `0x1809`)**. The measured
MCU temperature is pushed through the **Temperature Measurement** characteristic in
IEEE-11073 FLOAT format, so ST BLE Toolbox shows it directly as, e.g., `31.0 °C`,
`32.0 °C` — not raw HEX.

## Internal Temperature Measurement

> **Important:** this is the **MCU internal die temperature**, not ambient / room
> temperature. It reflects the chip's own junction temperature and will read above
> room temperature.

Measurement chain:

1. **ADC1** samples the internal temperature sensor channel and the internal
   voltage reference **VREFINT**.
2. VREFINT is used to compute the real **VDDA**, so the reading is corrected for the
   actual supply voltage. The same shared ADC helper produces the VDDA value exposed
   on `FE43`.
3. The temperature is computed from the **STM32 factory calibration values**
   (`TS_CAL1` / `TS_CAL2`) using the ST calibration macros.
4. The result is formatted for the Health Thermometer Temperature Measurement
   characteristic and notified to the phone.

## How to Build

1. Open **STM32CubeIDE** and import this folder as an existing project
   (`File → Open Projects from File System…` → select `WB55_BLE_Phone_Control`).
2. Make sure the WB55 **wireless stack (M0+ firmware)** is programmed on the board
   (a full BLE stack such as `stm32wb5x_BLE_Stack_full_fw` is required).
3. Build (`Project → Build Project`) and flash to the NUCLEO-WB55RG.

Only source and project files needed to rebuild are committed. Build outputs
(`Debug/`, `.elf`, `.map`, `.list`, `.o`, `.d`, …) are excluded via `.gitignore`.

> When re-generating code from the `.ioc` in CubeMX, keep all user code inside the
> `/* USER CODE BEGIN … */ … /* USER CODE END … */` sections — the LED/button/HTS
> logic lives there and is preserved across regeneration.

## How to Test with ST BLE Toolbox

1. Flash the firmware and reset the board.
2. In **ST BLE Toolbox**, scan and connect to **`WB55_CTRL`**.
3. **LED control:** open the P2P service, write to `FE41`, e.g. `01 01` to turn LD1
   on, `01 00` to turn it off (`02`/`03` for LD2/LD3).
4. **Button notifications:** enable notifications on `FE42`, then press SW1/SW2/SW3
   and watch the `xx 01` / `xx 00` notifications.
5. **Temperature:** open the Health Thermometer Service and read the temperature —
   it is shown directly in °C.
6. **Diagnostics:** read `FE43` (VDDA, mV), `FE44` (uptime, s), `FE45` (reset
   reason), `FE46` (connection stats), `FE47` (button counters) and `FE49`
   (connection duration). A normal power-up reads `FE45` = `01 00`.
7. **Software reset:** write `A5 5A` to `FE48`; the board resets after ~200 ms and
   `FE45` then reads `05 00` (`SFTRST`).
8. **Clear diagnostics:** write `C3 3C` to `FE4A`; connection/button counters reset
   to 0 while the connection stays up and `FE45` is unchanged.

## Current Status

Physical acceptance test on real board + iPhone — all passing:

| Feature                              | Result |
|--------------------------------------|:------:|
| LD1 / LD2 / LD3 control (`FE41`)      | ✅     |
| SW1 / SW2 / SW3 notification (`FE42`) | ✅     |
| VDDA read (`FE43`, `EC0C` = 3.308 V)  | ✅     |
| Uptime read (`FE44`)                  | ✅     |
| Reset reason (`FE45`, `0100`/`0900`)  | ✅     |
| Connection stats (`FE46`)             | ✅     |
| Button press counters (`FE47`)        | ✅     |
| Software reset (`FE48`, `A5 5A`)       | ✅     |
| Connection duration (`FE49`)          | ✅     |
| Clear diagnostics (`FE4A`, `C3 3C`)    | ✅     |
| IWDG watchdog reset + recovery        | ✅     |
| Health Thermometer real temp (31.0 °C)| ✅     |
| Device Information Service            | ✅     |
| P2P + HTS running together            | ✅     |

**Build:** `0 errors, 0 warnings` (clean rebuild, STM32CubeIDE 2.2.0 toolchain).
The firmware links successfully (`text 45456 / data 2405 / bss 3979` bytes).
