# STM32WB55 On-Chip Modbus Demo

STM32 NUCLEO-WB55RG kartının dahili sıcaklık sensörü, besleme gerilimi, uptime bilgisi, kart üzerindeki LED'leri ve butonları Modbus RTU üzerinden PC'ye açan örnek projedir.

Proje mevcut BMS Modbus RTU emulator firmware'i üzerine eklenmiştir.

## Donanım

* STM32 NUCLEO-WB55RG

  * Board: MB1355D-01
  * MCU: STM32WB55RGV6
* Waveshare USB-TTL FT232
* Harici sensör veya başka elektronik bileşen kullanılmamaktadır.

Kart üzerindeki kaynaklar:

* LD1 Mavi LED → PB5
* LD2 Yeşil LED → PB0
* LD3 Kırmızı LED → PB1
* SW1 → PC4
* SW2 → PD0
* SW3 → PD1
* ADC1 Internal Temperature Sensor
* ADC1 VREFINT

## Modbus Bağlantısı

Modbus RTU haberleşmesi `LPUART1` üzerinden yapılmaktadır.

Bağlantı:

```text
Waveshare TXD  → PA3 / D0 / LPUART1_RX
Waveshare RXD  → PA2 / D1 / LPUART1_TX
Waveshare GND  → STM32 GND
Waveshare VCC  → Bağlı değil
Logic Level    → 3V3
```

Seri port ayarları:

```text
Baud Rate : 9600
Data Bits : 8
Parity    : None
Stop Bits : 1
Unit ID   : 1
```

USART1 / ST-LINK VCP bu Modbus haberleşmesinde kullanılmamaktadır.

## CubeMX Ayarları

### ADC1

ADC1 içerisinde aşağıdaki dahili kanallar etkinleştirildi:

```text
Rank 1 → VREFINT
Rank 2 → Temperature Sensor
```

ADC ayarları:

```text
Resolution              : 12 bit
Data Alignment          : Right
Scan Conversion Mode    : Enabled
Number of Conversions   : 2
Continuous Conversion   : Disabled
Discontinuous Mode      : Disabled
Trigger                 : Software Start
DMA                     : Disabled
Sampling Time           : 640.5 cycles
```

ADC başlangıcında bir kez kalibrasyon yapılmaktadır.

### LED GPIO

```text
PB5 → LD1 → GPIO Output
PB0 → LD2 → GPIO Output
PB1 → LD3 → GPIO Output
```

LED'ler active-high:

```text
GPIO SET   → LED ON
GPIO RESET → LED OFF
```

### Button GPIO

```text
PC4 → SW1 → GPIO Input + Pull-up
PD0 → SW2 → GPIO Input + Pull-up
PD1 → SW3 → GPIO Input + Pull-up
```

Butonlar active-low:

```text
Pin HIGH → Buton bırakılmış → Modbus DI = 0
Pin LOW  → Butona basılmış  → Modbus DI = 1
```

### Uptime

Ek timer kullanılmamaktadır.

Uptime:

```c
HAL_GetTick() / 1000U
```

ile saniye cinsinden hesaplanmaktadır.

## Modbus Holding Register Haritası

PC uygulamasında:

```text
PDU Address = PLC Address - 40000
```

kullanılmaktadır.

| PLC Address | PDU | Açıklama                   | Birim     |
| ----------- | --: | -------------------------- | --------- |
| 40050       |  50 | MCU iç sıcaklığı           | °C × 10   |
| 40051       |  51 | Temperature Sensor raw ADC | ADC count |
| 40052       |  52 | VDDA besleme gerilimi      | mV        |
| 40053       |  53 | MCU uptime                 | saniye    |

Örnek:

```text
40050 = 314
```

şu anlama gelir:

```text
31.4 °C
```

`40052 = 3310` ise:

```text
VDDA = 3.310 V
```

anlamına gelir.

Uptime 16-bit register'da tutulduğu için:

```text
0 ... 65535 → 0 ...
```

şeklinde taşma yapar.

## Dahili Sıcaklık Ölçümü

ADC sequence:

```text
Rank 1 → VREFINT
Rank 2 → Temperature Sensor
```

VREFINT kullanılarak VDDA hesaplanmaktadır.

Firmware ayrıca STM32'nin fabrika sıcaklık kalibrasyon değerlerini kullanarak MCU junction sıcaklığını hesaplamaktadır.

Register 40050 sıcaklığı `0.1 °C` biriminde taşır.

Örnek:

```text
314 → 31.4 °C
332 → 33.2 °C
```

Test sırasında MCU üzerine parmakla dokunulduğunda sıcaklığın yaklaşık:

```text
31.4 °C → 33.2 °C
```

arasında yükseldiği gözlemlenmiştir.

## Coil Haritası

Modbus FC01 ve FC05 desteklenmektedir.

| Coil | Donanım         |
| ---: | --------------- |
|    0 | LD1 Mavi LED    |
|    1 | LD2 Yeşil LED   |
|    2 | LD3 Kırmızı LED |

FC05 örneği:

```text
Function : 05 Write Single Coil
Address  : 0
Value    : ON
```

Sonuç:

```text
LD1 mavi LED yanar.
```

OFF gönderildiğinde LED söner.

FC01 ile mevcut coil durumları okunabilir.

## Discrete Input Haritası

Modbus FC02 desteklenmektedir.

| DI | Donanım |
| -: | ------- |
|  0 | SW1     |
|  1 | SW2     |
|  2 | SW3     |

Örnek:

```text
Function : 02 Read Discrete Inputs
Address  : 0
Quantity : 3
```

SW1 basılıysa:

```text
DI0 = 1
```

SW1 bırakıldığında:

```text
DI0 = 0
```

## Desteklenen Modbus Function Code'ları

Firmware şu anda:

```text
FC01 → Read Coils
FC02 → Read Discrete Inputs
FC03 → Read Holding Registers
FC05 → Write Single Coil
FC06 → Write Single Register
FC16 → Write Multiple Registers
```

desteklemektedir.

FC03, FC06 ve FC16 mevcut BMS emulator işlevleriyle birlikte korunmuştur. Yeni FC01, FC02 ve FC05 desteği mevcut Modbus işleme yapısına eklenmiştir.

## PC Testi

PC uygulaması:

```text
Modbus Communication Suite
```

Ayarlar:

```text
Protocol : Modbus RTU
COM      : Waveshare USB-TTL COM portu
Baud     : 9600
Data     : 8
Parity   : None
Stop     : 1
Unit     : 1
```

### Sıcaklık

```text
FC03
Address  : 40050
Quantity : 1
Poll     : 500 ms
```

MCU üzerine parmakla dokunulduğunda değer yükselmelidir.

### VDDA

```text
FC03
Address  : 40052
Quantity : 1
```

Örnek:

```text
3308
3310
3312
```

gibi yaklaşık 3.3 V değerler görülür.

### Uptime

```text
FC03
Address  : 40053
Quantity : 1
```

Değer saniye bazında sürekli artar.

### LED

```text
FC05
Address  : 0
Value    : ON
```

LD1 yanar.

```text
Address 1 → LD2
Address 2 → LD3
```

### Buton

```text
FC02
Address  : 0
Quantity : 3
```

SW1/SW2/SW3 basıldığında ilgili DI değeri `1` olur.

### Eski BMS Register Testi

Mevcut firmware'in bozulmadığını kontrol etmek için:

```text
FC03
Address  : 40103
Quantity : 1
```

beklenen değer:

```text
512
```

olmalıdır.

## Proje Yapısı

Önemli firmware dosyaları:

```text
Core/
├── Inc/
│   ├── bms_registers.h
│   ├── modbus_crc.h
│   └── modbus_rtu.h
│
└── Src/
    ├── bms_registers.c
    ├── main.c
    ├── modbus_crc.c
    └── modbus_rtu.c
```

`main.c` içerisinde board digital I/O ve dahili ADC verileri periyodik olarak güncellenmektedir. Mevcut Modbus Receive → Process → Transmit akışı korunmuştur.

## Kabul Testleri

Proje üzerinde aşağıdaki testler başarıyla tamamlanmıştır:

```text
[PASS] FC03 40050 → MCU sıcaklığı °C × 10
[PASS] Parmakla ısıtıldığında sıcaklık artıyor
[PASS] FC03 40051 → Temperature Sensor raw ADC
[PASS] FC03 40052 → VDDA mV
[PASS] FC03 40053 → uptime artıyor

[PASS] FC05 Coil 0 → LD1
[PASS] FC05 Coil 1 → LD2
[PASS] FC05 Coil 2 → LD3

[PASS] FC02 DI0 → SW1
[PASS] FC02 DI1 → SW2
[PASS] FC02 DI2 → SW3

[PASS] Eski FC03 BMS register erişimi çalışmaya devam ediyor
```
