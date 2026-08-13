# STM32WB55 Projeleri

NUCLEO-WB55RG (STM32WB55RGV6) kartı ile geliştirilen gömülü yazılım projeleri (staj çalışması).
Her klasör ayrı bir STM32CubeIDE projesidir.

## Projeler

### WB55_UART_Test — LiBat BMS Modbus RTU Slave Emülatörü
Gerçek bir LiBat BMS cihazını taklit eden Modbus RTU slave.
- Fonksiyonlar: FC03 (Read Holding), FC06 (Write Single), FC16 (Write Multiple)
- CRC-16/MODBUS, exception handling, çalışma-anı Slave ID (40154), slave data select (40129)
- LiBat register map (Pack Voltage, Current, SOC, sıcaklık, Battery Status, hücre verileri)
- İki hat: ST-LINK VCP (115200) ve harici Waveshare USB-TTL (9600)

### WB55_Learning — Gömülü Temelleri
Temel çevre birimlerini öğrenme projesi:
- GPIO çıkış (LED) ve giriş (buton)
- Harici kesme (EXTI) + debounce
- Timer kesmesiyle periyodik olay

### 03-onchip-modbus — On-Chip Telemetri & Kontrol (Modbus RTU)
STM32WB55'in dahili sıcaklık sensörü, VDDA ve uptime değerlerini ve kart üstündeki
LED/butonları Modbus RTU üzerinden kullanan proje (dış donanım gerektirmez).
- Oku: sıcaklık `40050` (°C×10), sıcaklık ham `40051`, VDDA mV `40052`, uptime `40053`
- Coil→LED: LD1/LD2/LD3 (PB5/PB0/PB1); Discrete Input→buton: SW1/SW2/SW3 (PC4/PD0/PD1)
- LPUART1 (PA2 TX / PA3 RX) 9600 8N1, Waveshare FT232 USB-TTL

### WB55_BLE_Phone_Control — BLE Telefon Kontrolü + Health Thermometer
NUCLEO-WB55RG üzerinde BLE ile telefon (iPhone / ST BLE Toolbox) kontrolü. Cihaz adı `WB55_CTRL`.
- Custom P2P Service: `FE41` (telefon→MCU) 2 bayt LED komutu ile LD1/LD2/LD3 (Read + Write + Write w/o Response)
- `FE42` (MCU→telefon) SW1/SW2/SW3 için notification (polling + debounce)
- Bluetooth SIG Health Thermometer Service (0x1809): MCU **iç die sıcaklığı** (ADC1 + VREFINT + fabrika kalibrasyonu) °C olarak gönderiliyor
- P2P Server ve Health Thermometer aynı anda çalışıyor
- Detay: bkz. [`WB55_BLE_Phone_Control/README.md`](WB55_BLE_Phone_Control/README.md)

## Donanım
- Kart: NUCLEO-WB55RG (MB1355D-01), MCU STM32WB55RGV6
- Araçlar: STM32CubeMX + STM32CubeIDE

## Notlar
- Build çıktıları (`Debug/`) repoya dahil değildir (bkz. `.gitignore`).
- Her proje CubeIDE'de ayrı ayrı açılıp derlenebilir.
