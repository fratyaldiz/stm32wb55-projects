#include "modbus_rtu.h"
#include "modbus_crc.h"
#include "bms_registers.h"

/*
 * Aktif Modbus Slave ID.
 */
static uint8_t modbus_slave_id = MODBUS_DEFAULT_SLAVE_ID;

/*
 * Coil hafizasi:
 *
 * coil 0 -> LD1
 * coil 1 -> LD2
 * coil 2 -> LD3
 */
static uint8_t modbus_coils[MODBUS_COIL_COUNT];

/*
 * Discrete Input hafizasi:
 *
 * DI 0 -> SW1
 * DI 1 -> SW2
 * DI 2 -> SW3
 */
static uint8_t modbus_discrete_inputs[MODBUS_DISCRETE_INPUT_COUNT];


/* =========================================================
 * PRIVATE HELPERS
 * ========================================================= */

/*
 * Exception response:
 *
 * [addr]
 * [fc | 0x80]
 * [exception code]
 * [CRC low]
 * [CRC high]
 */
static uint16_t build_exception(
    uint8_t *resp,
    uint8_t addr,
    uint8_t fc,
    uint8_t code)
{
    resp[0] = addr;
    resp[1] = (uint8_t)(fc | 0x80U);
    resp[2] = code;

    uint16_t crc = modbus_crc16(resp, 3U);

    resp[3] = (uint8_t)(crc & 0xFFU);
    resp[4] = (uint8_t)(crc >> 8);

    return 5U;
}


/* =========================================================
 * PUBLIC FUNCTIONS
 * ========================================================= */

void modbus_rtu_init(void)
{
    /*
     * BMS register hafizasinda tanimli Modbus Address
     * degerini aktif Slave ID olarak al.
     */
    uint16_t configured_id =
        bms_reg_read(BMS_REG_MODBUS_ADDRESS);

    if (configured_id >= 1U && configured_id <= 247U)
    {
        modbus_slave_id = (uint8_t)configured_id;
    }
    else
    {
        modbus_slave_id = MODBUS_DEFAULT_SLAVE_ID;
    }

    /*
     * Baslangicta butun LED coil'leri OFF.
     */
    for (uint16_t i = 0U; i < MODBUS_COIL_COUNT; i++)
    {
        modbus_coils[i] = 0U;
    }

    /*
     * Baslangicta butun discrete input'lar pasif.
     * main.c ilk dongude gercek buton durumlarini
     * buraya yazacak.
     */
    for (uint16_t i = 0U; i < MODBUS_DISCRETE_INPUT_COUNT; i++)
    {
        modbus_discrete_inputs[i] = 0U;
    }
}


uint8_t modbus_rtu_get_slave_id(void)
{
    return modbus_slave_id;
}


uint8_t modbus_get_coil(uint16_t index)
{
    if (index < MODBUS_COIL_COUNT)
    {
        return modbus_coils[index];
    }

    return 0U;
}


void modbus_set_discrete_input(uint16_t index, uint8_t state)
{
    if (index < MODBUS_DISCRETE_INPUT_COUNT)
    {
        modbus_discrete_inputs[index] =
            (state != 0U) ? 1U : 0U;
    }
}


uint8_t modbus_get_discrete_input(uint16_t index)
{
    if (index < MODBUS_DISCRETE_INPUT_COUNT)
    {
        return modbus_discrete_inputs[index];
    }

    return 0U;
}


uint16_t modbus_rtu_process(
    const uint8_t *req,
    uint16_t reqlen,
    uint8_t *resp)
{
    /*
     * En kisa anlamli RTU frame:
     *
     * address
     * function
     * CRC low
     * CRC high
     */
    if (reqlen < 4U)
    {
        return 0U;
    }


    /* =====================================================
     * CRC CHECK
     * ===================================================== */

    uint16_t crc_calc =
        modbus_crc16(req, (uint16_t)(reqlen - 2U));

    uint16_t crc_recv =
        (uint16_t)req[reqlen - 2U] |
        ((uint16_t)req[reqlen - 1U] << 8);

    if (crc_calc != crc_recv)
    {
        /*
         * CRC hatali frame'e cevap verilmez.
         */
        return 0U;
    }


    /* =====================================================
     * SLAVE ADDRESS CHECK
     * ===================================================== */

    uint8_t addr = req[0];

    if (addr != modbus_slave_id)
    {
        /*
         * Frame baska cihaza ait.
         */
        return 0U;
    }

    uint8_t fc = req[1];


    /* =====================================================
     * FC01 - READ COILS
     * ===================================================== */

    if (fc == MB_FC_READ_COILS)
    {
        /*
         * Request:
         *
         * [0] Slave Address
         * [1] 0x01
         * [2] Start Address Hi
         * [3] Start Address Lo
         * [4] Quantity Hi
         * [5] Quantity Lo
         * [6] CRC Lo
         * [7] CRC Hi
         */

        if (reqlen != 8U)
        {
            return build_exception(
                resp,
                addr,
                fc,
                MB_EX_ILLEGAL_VALUE
            );
        }

        uint16_t start =
            ((uint16_t)req[2] << 8) |
            (uint16_t)req[3];

        uint16_t qty =
            ((uint16_t)req[4] << 8) |
            (uint16_t)req[5];

        /*
         * Modbus FC01 quantity:
         * 1..2000 bit.
         */
        if (qty < 1U || qty > 2000U)
        {
            return build_exception(
                resp,
                addr,
                fc,
                MB_EX_ILLEGAL_VALUE
            );
        }

        /*
         * Bizde sadece 3 coil var.
         */
        if ((uint32_t)start + (uint32_t)qty >
            (uint32_t)MODBUS_COIL_COUNT)
        {
            return build_exception(
                resp,
                addr,
                fc,
                MB_EX_ILLEGAL_ADDRESS
            );
        }

        /*
         * Bitler Modbus standardinda LSB-first paketlenir.
         *
         * Ornek:
         *
         * coil0 = 1
         * coil1 = 0
         * coil2 = 1
         *
         * data byte = 00000101b = 0x05
         */

        uint8_t byte_count =
            (uint8_t)((qty + 7U) / 8U);

        resp[0] = addr;
        resp[1] = fc;
        resp[2] = byte_count;

        for (uint8_t i = 0U; i < byte_count; i++)
        {
            resp[3U + i] = 0U;
        }

        for (uint16_t i = 0U; i < qty; i++)
        {
            if (modbus_coils[start + i] != 0U)
            {
                uint16_t byte_index = i / 8U;
                uint16_t bit_index  = i % 8U;

                resp[3U + byte_index] |=
                    (uint8_t)(1U << bit_index);
            }
        }

        uint16_t idx =
            (uint16_t)(3U + byte_count);

        uint16_t crc =
            modbus_crc16(resp, idx);

        resp[idx++] = (uint8_t)(crc & 0xFFU);
        resp[idx++] = (uint8_t)(crc >> 8);

        return idx;
    }


    /* =====================================================
     * FC02 - READ DISCRETE INPUTS
     * ===================================================== */

    if (fc == MB_FC_READ_DISCRETE_INPUTS)
    {
        /*
         * Request format FC01 ile aynidir.
         */

        if (reqlen != 8U)
        {
            return build_exception(
                resp,
                addr,
                fc,
                MB_EX_ILLEGAL_VALUE
            );
        }

        uint16_t start =
            ((uint16_t)req[2] << 8) |
            (uint16_t)req[3];

        uint16_t qty =
            ((uint16_t)req[4] << 8) |
            (uint16_t)req[5];

        if (qty < 1U || qty > 2000U)
        {
            return build_exception(
                resp,
                addr,
                fc,
                MB_EX_ILLEGAL_VALUE
            );
        }

        /*
         * Bizde sadece 3 discrete input var.
         */
        if ((uint32_t)start + (uint32_t)qty >
            (uint32_t)MODBUS_DISCRETE_INPUT_COUNT)
        {
            return build_exception(
                resp,
                addr,
                fc,
                MB_EX_ILLEGAL_ADDRESS
            );
        }

        uint8_t byte_count =
            (uint8_t)((qty + 7U) / 8U);

        resp[0] = addr;
        resp[1] = fc;
        resp[2] = byte_count;

        for (uint8_t i = 0U; i < byte_count; i++)
        {
            resp[3U + i] = 0U;
        }

        for (uint16_t i = 0U; i < qty; i++)
        {
            if (modbus_discrete_inputs[start + i] != 0U)
            {
                uint16_t byte_index = i / 8U;
                uint16_t bit_index  = i % 8U;

                resp[3U + byte_index] |=
                    (uint8_t)(1U << bit_index);
            }
        }

        uint16_t idx =
            (uint16_t)(3U + byte_count);

        uint16_t crc =
            modbus_crc16(resp, idx);

        resp[idx++] = (uint8_t)(crc & 0xFFU);
        resp[idx++] = (uint8_t)(crc >> 8);

        return idx;
    }


    /* =====================================================
     * FC03 - READ HOLDING REGISTERS
     * ===================================================== */

    if (fc == MB_FC_READ_HOLDING)
    {
        /*
         * Request:
         *
         * [0] Slave Address
         * [1] 0x03
         * [2] Start Address Hi
         * [3] Start Address Lo
         * [4] Quantity Hi
         * [5] Quantity Lo
         * [6] CRC Lo
         * [7] CRC Hi
         */

        if (reqlen != 8U)
        {
            return build_exception(
                resp,
                addr,
                fc,
                MB_EX_ILLEGAL_VALUE
            );
        }

        uint16_t start =
            ((uint16_t)req[2] << 8) |
            (uint16_t)req[3];

        uint16_t qty =
            ((uint16_t)req[4] << 8) |
            (uint16_t)req[5];

        /*
         * Modbus FC03:
         * 1..125 register.
         */
        if (qty < 1U || qty > 125U)
        {
            return build_exception(
                resp,
                addr,
                fc,
                MB_EX_ILLEGAL_VALUE
            );
        }

        if ((uint32_t)start + (uint32_t)qty >
            (uint32_t)BMS_REG_COUNT)
        {
            return build_exception(
                resp,
                addr,
                fc,
                MB_EX_ILLEGAL_ADDRESS
            );
        }

        resp[0] = addr;
        resp[1] = fc;
        resp[2] = (uint8_t)(qty * 2U);

        uint16_t idx = 3U;

        for (uint16_t i = 0U; i < qty; i++)
        {
            uint16_t val =
                bms_reg_read((uint16_t)(start + i));

            resp[idx++] =
                (uint8_t)(val >> 8);

            resp[idx++] =
                (uint8_t)(val & 0xFFU);
        }

        uint16_t crc =
            modbus_crc16(resp, idx);

        resp[idx++] =
            (uint8_t)(crc & 0xFFU);

        resp[idx++] =
            (uint8_t)(crc >> 8);

        return idx;
    }


    /* =====================================================
     * FC05 - WRITE SINGLE COIL
     * ===================================================== */

    if (fc == MB_FC_WRITE_SINGLE_COIL)
    {
        /*
         * Request:
         *
         * [0] Slave Address
         * [1] 0x05
         * [2] Coil Address Hi
         * [3] Coil Address Lo
         * [4] Value Hi
         * [5] Value Lo
         * [6] CRC Lo
         * [7] CRC Hi
         *
         * ON  = 0xFF00
         * OFF = 0x0000
         */

        if (reqlen != 8U)
        {
            return build_exception(
                resp,
                addr,
                fc,
                MB_EX_ILLEGAL_VALUE
            );
        }

        uint16_t coil =
            ((uint16_t)req[2] << 8) |
            (uint16_t)req[3];

        uint16_t value =
            ((uint16_t)req[4] << 8) |
            (uint16_t)req[5];

        if (coil >= MODBUS_COIL_COUNT)
        {
            return build_exception(
                resp,
                addr,
                fc,
                MB_EX_ILLEGAL_ADDRESS
            );
        }

        /*
         * Modbus FC05 sadece:
         *
         * 0xFF00 -> ON
         * 0x0000 -> OFF
         *
         * kabul eder.
         */
        if (value == 0xFF00U)
        {
            modbus_coils[coil] = 1U;
        }
        else if (value == 0x0000U)
        {
            modbus_coils[coil] = 0U;
        }
        else
        {
            return build_exception(
                resp,
                addr,
                fc,
                MB_EX_ILLEGAL_VALUE
            );
        }

        /*
         * Basarili FC05 cevabi request'in echo'sudur.
         * Ilk 6 byte'i kopyalayip CRC'yi yeniden hesapliyoruz.
         */
        resp[0] = req[0];
        resp[1] = req[1];
        resp[2] = req[2];
        resp[3] = req[3];
        resp[4] = req[4];
        resp[5] = req[5];

        uint16_t crc =
            modbus_crc16(resp, 6U);

        resp[6] = (uint8_t)(crc & 0xFFU);
        resp[7] = (uint8_t)(crc >> 8);

        return 8U;
    }


    /* =====================================================
     * FC06 - WRITE SINGLE REGISTER
     * ===================================================== */

    if (fc == MB_FC_WRITE_SINGLE_REGISTER)
    {
        if (reqlen != 8U)
        {
            return build_exception(
                resp,
                addr,
                fc,
                MB_EX_ILLEGAL_VALUE
            );
        }

        uint16_t reg =
            ((uint16_t)req[2] << 8) |
            (uint16_t)req[3];

        uint16_t value =
            ((uint16_t)req[4] << 8) |
            (uint16_t)req[5];

        if (reg >= BMS_REG_COUNT)
        {
            return build_exception(
                resp,
                addr,
                fc,
                MB_EX_ILLEGAL_ADDRESS
            );
        }

        /*
         * FC06 ile yalnizca:
         *
         * 40129 -> Slave Data Select
         * 40154 -> Modbus Address
         *
         * yazilabilir.
         */
        if ((reg != BMS_REG_MODBUS_ADDRESS) &&
            (reg != BMS_REG_SLAVE_DATA_SELECT))
        {
            return build_exception(
                resp,
                addr,
                fc,
                MB_EX_ILLEGAL_ADDRESS
            );
        }

        if (bms_reg_write(reg, value) == 0U)
        {
            return build_exception(
                resp,
                addr,
                fc,
                MB_EX_ILLEGAL_VALUE
            );
        }

        /*
         * FC06 basarili response request'in echo'sudur.
         */
        resp[0] = req[0];
        resp[1] = req[1];
        resp[2] = req[2];
        resp[3] = req[3];
        resp[4] = req[4];
        resp[5] = req[5];

        uint16_t crc =
            modbus_crc16(resp, 6U);

        resp[6] =
            (uint8_t)(crc & 0xFFU);

        resp[7] =
            (uint8_t)(crc >> 8);

        /*
         * Modbus Address degistiyse aktif ID'yi de guncelle.
         *
         * Cevap eski addr ile zaten olusturuldu.
         */
        if (reg == BMS_REG_MODBUS_ADDRESS)
        {
            modbus_slave_id = (uint8_t)value;
        }

        return 8U;
    }


    /* =====================================================
     * FC16 - WRITE MULTIPLE REGISTERS
     * ===================================================== */

    if (fc == MB_FC_WRITE_MULTIPLE)
    {
        /*
         * Request:
         *
         * [0] Slave Address
         * [1] 0x10
         * [2] Start Address Hi
         * [3] Start Address Lo
         * [4] Quantity Hi
         * [5] Quantity Lo
         * [6] Byte Count
         * [7..] Data
         * [..] CRC Lo
         * [..] CRC Hi
         */

        if (reqlen < 11U)
        {
            return build_exception(
                resp,
                addr,
                fc,
                MB_EX_ILLEGAL_VALUE
            );
        }

        uint16_t start =
            ((uint16_t)req[2] << 8) |
            (uint16_t)req[3];

        uint16_t qty =
            ((uint16_t)req[4] << 8) |
            (uint16_t)req[5];

        uint8_t bc = req[6];

        if (qty < 1U ||
            qty > 123U ||
            bc != (uint8_t)(qty * 2U))
        {
            return build_exception(
                resp,
                addr,
                fc,
                MB_EX_ILLEGAL_VALUE
            );
        }

        if (reqlen != (uint16_t)(9U + bc))
        {
            return build_exception(
                resp,
                addr,
                fc,
                MB_EX_ILLEGAL_VALUE
            );
        }

        if ((uint32_t)start + (uint32_t)qty >
            (uint32_t)BMS_REG_COUNT)
        {
            return build_exception(
                resp,
                addr,
                fc,
                MB_EX_ILLEGAL_ADDRESS
            );
        }

        for (uint16_t i = 0U; i < qty; i++)
        {
            uint16_t v =
                ((uint16_t)req[7U + i * 2U] << 8) |
                (uint16_t)req[8U + i * 2U];

            if (bms_reg_write(
                    (uint16_t)(start + i),
                    v) == 0U)
            {
                return build_exception(
                    resp,
                    addr,
                    fc,
                    MB_EX_ILLEGAL_VALUE
                );
            }
        }

        /*
         * 40154 yazildiysa aktif Slave ID'yi guncelle.
         */
        if ((start <= BMS_REG_MODBUS_ADDRESS) &&
            ((uint32_t)BMS_REG_MODBUS_ADDRESS <
             (uint32_t)start + (uint32_t)qty))
        {
            modbus_slave_id =
                (uint8_t)bms_reg_read(
                    BMS_REG_MODBUS_ADDRESS);
        }

        /*
         * Response:
         *
         * [addr]
         * [fc]
         * [startHi]
         * [startLo]
         * [qtyHi]
         * [qtyLo]
         * [CRC Lo]
         * [CRC Hi]
         */
        resp[0] = req[0];
        resp[1] = req[1];
        resp[2] = req[2];
        resp[3] = req[3];
        resp[4] = req[4];
        resp[5] = req[5];

        uint16_t crc =
            modbus_crc16(resp, 6U);

        resp[6] =
            (uint8_t)(crc & 0xFFU);

        resp[7] =
            (uint8_t)(crc >> 8);

        return 8U;
    }


    /* =====================================================
     * UNSUPPORTED FUNCTION
     * ===================================================== */

    return build_exception(
        resp,
        addr,
        fc,
        MB_EX_ILLEGAL_FUNCTION
    );
}
