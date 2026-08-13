# WB55_BLE_Phone_Control — STM32WB55 BLE Phone Control + Health Thermometer

BLE firmware for the NUCLEO-WB55RG board. A phone (ST BLE Toolbox on iPhone)
connects over Bluetooth Low Energy and:

- controls the three on-board LEDs (phone → MCU),
- receives notifications when the three user buttons are pressed (MCU → phone),
- reads the MCU's **internal die temperature** through the standard Bluetooth
  Health Thermometer Service.

Internship / learning project. Generated with STM32CubeMX, built with STM32CubeIDE.

## Project Overview

The firmware runs two BLE features at the same time on one connection:

1. A **Custom P2P Service** (ST P2P Server template) for LED control and button
   notifications, using the `FE41` and `FE42` characteristics.
2. The **Bluetooth SIG Health Thermometer Service (0x1809)** that reports the
   real MCU internal temperature as a proper temperature value (e.g. `31.0 °C`)
   instead of raw HEX.

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

Health Thermometer Service:
MCU internal temperature → °C
```

Advertising name / device name: **`WB55_CTRL`**.
P2P Server and Health Thermometer run **simultaneously** on the same connection.

## P2P Service

Custom P2P service with two characteristics:

| Characteristic | Direction    | Purpose                         | Properties                                |
|----------------|--------------|---------------------------------|-------------------------------------------|
| `FE41`         | Phone → MCU  | LED control (LD1/LD2/LD3)       | Read, Write, Write Without Response       |
| `FE42`         | MCU → Phone  | Button notifications (SW1/2/3)  | Notify                                    |

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
   actual supply voltage.
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

## Current Status

Physical acceptance test on real board + iPhone — all passing:

| Feature                          | Result |
|----------------------------------|:------:|
| LD1 control                      | ✅     |
| LD2 control                      | ✅     |
| LD3 control                      | ✅     |
| SW1 notification                 | ✅     |
| SW2 notification                 | ✅     |
| SW3 notification                 | ✅     |
| Health Thermometer real temp     | ✅     |
| P2P + HTS running together       | ✅     |

**Build:** `0 errors`. The firmware links successfully
(`text 41188 / data 2325 / bss 3411` bytes). A full clean rebuild emits **13 benign
compiler warnings** — all `-Wunused-*` (unused function / variable) notices in the
ST CubeMX-generated template code (`app_ble.c`, `hts_app.c`); they have **no effect
on functionality**. Incremental IDE builds (no recompile) report `0 warnings`.
