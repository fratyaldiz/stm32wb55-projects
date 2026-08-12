# WB55 LiBat Modbus RTU Slave Emülatörü

STM32 **NUCLEO-WB55RG** üzerinde, gerçek bir **LiBat BMS** cihazını taklit eden bir **Modbus RTU Slave** emülatörü. Bir Windows Modbus Master uygulaması, kartı gerçek bir batarya cihazı sanarak register'larını okuyup yazabilir.

```
Windows Modbus Master  ->  UART (ST-LINK VCP veya Waveshare USB-TTL)  ->  STM32  ->  Modbus RTU Slave  ->  LiBat register map
```

## Donanım

- Kart: NUCLEO-WB55RG (rev MB1355D-01) · MCU: STM32WB55RGV6
- İki seri haberleşme yolu (ikisi de test edildi):
  - **USART1** (PB6 TX / PB7 RX) -> on-board ST-LINK Virtual COM Port · **115200 8N1**
  - **LPUART1** (PA2 TX / PA3 RX = Arduino **D1 / D0**) -> harici **Waveshare USB-TTL** · **9600 8N1**

### Waveshare USB-TTL bağlantısı

| Waveshare | STM32 (Arduino header) |
|-----------|------------------------|
| TXD | D0 (PA3, RX) |
| RXD | D1 (PA2, TX) |
| GND | GND |
| VCC / 5V | **bağlanmaz** |

Adaptör voltaj anahtarı: **3V3**. (Uzun jumper kablolarda 115200 sinyal bütünlüğüne takıldığı için harici hat **9600** kullanır; VCP hattı 115200'de çalışır.)

## Desteklenen Modbus fonksiyonları

- **FC03** Read Holding Registers
- **FC06** Write Single Register
- **FC16** Write Multiple Registers
- Exception cevapları: `0x01` Illegal Function, `0x02` Illegal Data Address, `0x03` Illegal Data Value
- CRC-16/MODBUS doğrulama · Unit ID kontrolü · çalışma anında Slave ID değişimi (register 40154)

## LiBat register map (özet)

PDU (protokol) adresi = mantıksal adres − 40000. Firmware **ham** değer döner; ölçekleme PC uygulamasında yorumlanır.

| Mantıksal | İçerik | Ölçek / Not |
|-----------|--------|-------------|
| 40103 | Pack Voltage | ×0.1 V (512 = 51.2 V) |
| 40104 | Pack Current | ×0.1 A |
| 40107 | SOC | % |
| 40108 / 40109 | Min / Max Cell Voltage | mV |
| 40110 / 40111 | Min / Max Temperature | ×0.1 °C |
| 40114–40117 | Battery Status | 64-bit (bit 2 = Over Temperature Warning) |
| 40129 | Slave Data Select | 1 veya 2 (iki sanal batarya seti) |
| 40130 | Cell Count | |
| 40131–40148 | Cell Voltages | mV, yoksa 0xFFFF |
| 40149–40153 | Temperatures | ×0.1 °C, yoksa 0xFFFF |
| 40154 | Modbus Address | 1–247 |

## Dosya yapısı

```
Core/
  Inc/  Src/
    modbus_crc.*    CRC-16/MODBUS (0xA001, init 0xFFFF)
    modbus_rtu.*    Frame çözme, FC03 / FC06 / FC16, exception, slave ID
    bms_registers.* LiBat register modeli + emülasyon değerleri
    main.c          init + ana döngü (ReceiveToIdle -> process -> transmit)
```

## Derleme ve yükleme

1. STM32CubeMX / STM32CubeIDE ile `WB55_UART_Test.ioc` projesini aç.
2. **Build** -> ST-LINK ile **Run** (flash).

## Test

Windows Modbus Master ayarları: COM portu (VCP=115200 / Waveshare=9600), **8N1**, **Unit ID 1**.

- `FC03` · Address **103** · Quantity **9** -> LiBat ölçümleri gelir.
- `FC06` · Address **129** · Value **2** -> ikinci sanal batarya setine geçer.
- `FC16` · Address **103** · Values `600, 111` -> çoklu register yazma.
