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

## Donanım
- Kart: NUCLEO-WB55RG (MB1355D-01), MCU STM32WB55RGV6
- Araçlar: STM32CubeMX + STM32CubeIDE

## Notlar
- Build çıktıları (`Debug/`) repoya dahil değildir (bkz. `.gitignore`).
- Her proje CubeIDE'de ayrı ayrı açılıp derlenebilir.
