#include "bms_registers.h"


uint16_t bms_holding_regs[BMS_REG_COUNT];


/*
 * Secilen sanal BMS slave'in 40130-40153 arasindaki
 * hucre ve sicaklik verilerini yukler.
 */
static void bms_load_slave_data(uint16_t slave)
{
    /*
     * Her slave seciminde once ortak alanlari temizle.
     *
     * LiBat dokumanina gore mevcut olmayan hucre veya
     * sicaklik sensoru 0xFFFF olarak okunur.
     */

    bms_holding_regs[BMS_REG_CELL_COUNT] = 0U;


    /* Cell 1-18: varsayilan olarak mevcut degil */
    for (uint16_t i = BMS_REG_CELL_1_VOLTAGE;
         i <= BMS_REG_CELL_18_VOLTAGE;
         i++)
    {
        bms_holding_regs[i] = 0xFFFFU;
    }


    /* Temperature 1-5: varsayilan olarak mevcut degil */
    for (uint16_t i = BMS_REG_TEMPERATURE_1;
         i <= BMS_REG_TEMPERATURE_5;
         i++)
    {
        bms_holding_regs[i] = 0xFFFFU;
    }


    /* =====================================================
     * SANAL SLAVE 1
     * ===================================================== */
    if (slave == 1U)
    {
        bms_holding_regs[BMS_REG_CELL_COUNT] = 16U;

        bms_holding_regs[BMS_REG_CELL_1_VOLTAGE]  = 3305U;
        bms_holding_regs[BMS_REG_CELL_2_VOLTAGE]  = 3308U;
        bms_holding_regs[BMS_REG_CELL_3_VOLTAGE]  = 3301U;
        bms_holding_regs[BMS_REG_CELL_4_VOLTAGE]  = 3312U;
        bms_holding_regs[BMS_REG_CELL_5_VOLTAGE]  = 3299U;
        bms_holding_regs[BMS_REG_CELL_6_VOLTAGE]  = 3315U;
        bms_holding_regs[BMS_REG_CELL_7_VOLTAGE]  = 3302U;
        bms_holding_regs[BMS_REG_CELL_8_VOLTAGE]  = 3310U;
        bms_holding_regs[BMS_REG_CELL_9_VOLTAGE]  = 3298U;
        bms_holding_regs[BMS_REG_CELL_10_VOLTAGE] = 3307U;
        bms_holding_regs[BMS_REG_CELL_11_VOLTAGE] = 3311U;
        bms_holding_regs[BMS_REG_CELL_12_VOLTAGE] = 3300U;
        bms_holding_regs[BMS_REG_CELL_13_VOLTAGE] = 3309U;
        bms_holding_regs[BMS_REG_CELL_14_VOLTAGE] = 3303U;
        bms_holding_regs[BMS_REG_CELL_15_VOLTAGE] = 3313U;
        bms_holding_regs[BMS_REG_CELL_16_VOLTAGE] = 3306U;

        /*
         * Cell 17 ve Cell 18:
         * 0xFFFF olarak kalir.
         */

        bms_holding_regs[BMS_REG_TEMPERATURE_1] = 250U; /* 25.0 C */
        bms_holding_regs[BMS_REG_TEMPERATURE_2] = 248U; /* 24.8 C */
        bms_holding_regs[BMS_REG_TEMPERATURE_3] = 252U; /* 25.2 C */

        /*
         * Temperature 4 ve 5:
         * 0xFFFF olarak kalir.
         */
    }


    /* =====================================================
     * SANAL SLAVE 2
     * ===================================================== */
    else if (slave == 2U)
    {
        bms_holding_regs[BMS_REG_CELL_COUNT] = 12U;

        bms_holding_regs[BMS_REG_CELL_1_VOLTAGE]  = 3410U;
        bms_holding_regs[BMS_REG_CELL_2_VOLTAGE]  = 3405U;
        bms_holding_regs[BMS_REG_CELL_3_VOLTAGE]  = 3412U;
        bms_holding_regs[BMS_REG_CELL_4_VOLTAGE]  = 3408U;
        bms_holding_regs[BMS_REG_CELL_5_VOLTAGE]  = 3415U;
        bms_holding_regs[BMS_REG_CELL_6_VOLTAGE]  = 3402U;
        bms_holding_regs[BMS_REG_CELL_7_VOLTAGE]  = 3409U;
        bms_holding_regs[BMS_REG_CELL_8_VOLTAGE]  = 3411U;
        bms_holding_regs[BMS_REG_CELL_9_VOLTAGE]  = 3406U;
        bms_holding_regs[BMS_REG_CELL_10_VOLTAGE] = 3413U;
        bms_holding_regs[BMS_REG_CELL_11_VOLTAGE] = 3404U;
        bms_holding_regs[BMS_REG_CELL_12_VOLTAGE] = 3407U;

        /*
         * Cell 13-18:
         * 0xFFFF olarak kalir.
         */

        bms_holding_regs[BMS_REG_TEMPERATURE_1] = 300U; /* 30.0 C */
        bms_holding_regs[BMS_REG_TEMPERATURE_2] = 295U; /* 29.5 C */

        /*
         * Temperature 3-5:
         * 0xFFFF olarak kalir.
         */
    }
}


void bms_registers_init(void)
{
    /*
     * Once butun register hafizasini sifirla.
     */
    for (uint16_t i = 0U; i < BMS_REG_COUNT; i++)
    {
        bms_holding_regs[i] = 0U;
    }


    /* =====================================================
     * VERSION / DEVICE INFORMATION
     * ===================================================== */

    /* Software Version: 1.0.0 */
    bms_holding_regs[BMS_REG_SOFTWARE_VERSION_MAJOR] = 1U;
    bms_holding_regs[BMS_REG_SOFTWARE_VERSION_MINOR] = 0U;
    bms_holding_regs[BMS_REG_SOFTWARE_VERSION_PATCH] = 0U;


    /* Hardware Version: 1.0.0 */
    bms_holding_regs[BMS_REG_HARDWARE_VERSION_MAJOR] = 1U;
    bms_holding_regs[BMS_REG_HARDWARE_VERSION_MINOR] = 0U;
    bms_holding_regs[BMS_REG_HARDWARE_VERSION_PATCH] = 0U;


    /* Ornek Serial Number */
    bms_holding_regs[BMS_REG_SERIAL_NUMBER_1] = 0x1234U;
    bms_holding_regs[BMS_REG_SERIAL_NUMBER_2] = 0x5678U;
    bms_holding_regs[BMS_REG_SERIAL_NUMBER_3] = 0x9ABCU;
    bms_holding_regs[BMS_REG_SERIAL_NUMBER_4] = 0xDEF0U;


    /* Ornek Bootloader Version */
    bms_holding_regs[BMS_REG_BOOTLOADER_VERSION_1] = 1U;
    bms_holding_regs[BMS_REG_BOOTLOADER_VERSION_2] = 0U;
    bms_holding_regs[BMS_REG_BOOTLOADER_VERSION_3] = 0U;
    bms_holding_regs[BMS_REG_BOOTLOADER_VERSION_4] = 0U;


    /* Ornek Model Number */
    bms_holding_regs[BMS_REG_MODEL_NUMBER] = 1U;


    /* =====================================================
     * MAIN BATTERY MEASUREMENTS
     * ===================================================== */

    /*
     * 40103 / Register 103
     * Pack Voltage
     *
     * Raw = 512
     * 512 * 0.1 V = 51.2 V
     */
    bms_holding_regs[BMS_REG_PACK_VOLTAGE] = 512U;


    /*
     * 40104 / Register 104
     * Pack Current
     *
     * Raw = 250
     * 250 * 0.1 A = 25.0 A
     */
    bms_holding_regs[BMS_REG_PACK_CURRENT] = 250U;


    /*
     * 40105-40106 Reserved.
     * Sifir olarak kalir.
     */


    /*
     * 40107 / Register 107
     * SOC = %87
     */
    bms_holding_regs[BMS_REG_SOC] = 87U;


    /*
     * 40108 / Register 108
     * Minimum Cell Voltage
     */
    bms_holding_regs[BMS_REG_MIN_CELL_VOLTAGE] = 3298U;


    /*
     * 40109 / Register 109
     * Maximum Cell Voltage
     */
    bms_holding_regs[BMS_REG_MAX_CELL_VOLTAGE] = 3315U;


    /*
     * 40110 / Register 110
     *
     * Raw = 235
     * 23.5 C
     */
    bms_holding_regs[BMS_REG_MIN_TEMPERATURE] = 235U;


    /*
     * 40111 / Register 111
     *
     * Raw = 271
     * 27.1 C
     */
    bms_holding_regs[BMS_REG_MAX_TEMPERATURE] = 271U;


    /*
     * 40112-40113 Reserved.
     * Sifir olarak kalir.
     */


    /* =====================================================
     * BATTERY STATUS
     * =====================================================
     *
     * 40114 -> bits 63:48
     * 40115 -> bits 47:32
     * 40116 -> bits 31:16
     * 40117 -> bits 15:0
     *
     * Simdilik sadece Bit 2 aktif:
     *
     * Bit 2 = Over Temperature Warning
     */

    bms_holding_regs[BMS_REG_BATTERY_STATUS_MSB]    = 0x0000U;
    bms_holding_regs[BMS_REG_BATTERY_STATUS_47_32]  = 0x0000U;
    bms_holding_regs[BMS_REG_BATTERY_STATUS_31_16]  = 0x0000U;
    bms_holding_regs[BMS_REG_BATTERY_STATUS_LSB]    = 0x0004U;


    /* =====================================================
     * SLAVE DATA
     * =====================================================
     *
     * Baslangicta:
     *
     * 40129 = 1
     *
     * Dolayisiyla 40130-40153 alanina
     * Slave 1 dataset'i yuklenir.
     */

    bms_holding_regs[BMS_REG_SLAVE_DATA_SELECT] = 1U;

    bms_load_slave_data(1U);


    /* =====================================================
     * MODBUS ADDRESS
     * =====================================================
     *
     * 40154 / Register 154
     *
     * Baslangic Modbus Slave ID = 1
     */

    bms_holding_regs[BMS_REG_MODBUS_ADDRESS] = 1U;
}


uint16_t bms_reg_read(uint16_t index)
{
    if (index < BMS_REG_COUNT)
    {
        return bms_holding_regs[index];
    }

    return 0U;
}


uint8_t bms_reg_write(uint16_t index, uint16_t value)
{
    /*
     * =====================================================
     * 40129 / Register 129
     * Slave Data Select
     * =====================================================
     *
     * Simdilik emulasyonumuzda iki sanal BMS var:
     *
     * 1 = Slave 1
     * 2 = Slave 2
     */

    if (index == BMS_REG_SLAVE_DATA_SELECT)
    {
        if (value < 1U || value > 2U)
        {
            return 0U;
        }


        /*
         * 40129 registerini yeni secimle guncelle.
         */
        bms_holding_regs[index] = value;


        /*
         * KRITIK DEGISIKLIK:
         *
         * Secilen slave'e ait 40130-40153
         * verilerini register hafizasina yukle.
         *
         * 40129 = 1 -> Slave 1 dataset
         * 40129 = 2 -> Slave 2 dataset
         */
        bms_load_slave_data(value);


        return 1U;
    }


    /*
     * =====================================================
     * 40154 / Register 154
     * Modbus Slave Address
     * =====================================================
     *
     * Gecerli aralik:
     *
     * 1..247
     */

    if (index == BMS_REG_MODBUS_ADDRESS)
    {
        if (value < 1U || value > 247U)
        {
            return 0U;
        }


        bms_holding_regs[index] = value;


        return 1U;
    }


    /*
     * Diger registerlar: emulator test amacli dogrudan yazilir
     * (FC16 ile Master'dan deger enjekte edilebilir).
     */
    if (index < BMS_REG_COUNT)
    {
        bms_holding_regs[index] = value;
        return 1U;
    }

    return 0U;
}
