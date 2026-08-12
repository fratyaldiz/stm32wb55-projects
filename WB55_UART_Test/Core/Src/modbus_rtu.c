#include "modbus_rtu.h"
#include "modbus_crc.h"
#include "bms_registers.h"


/*
 * Aktif Modbus Slave ID.
 *
 * Baslangicta LiBat varsayilani olan 1 kullanilir.
 * Register 154 FC06 ile yazildiginda bu deger
 * calisma sirasinda degisir.
 */
static uint8_t modbus_slave_id = MODBUS_DEFAULT_SLAVE_ID;


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
     *
     * Normalde bms_registers_init() bunu 1 yapar.
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
}


uint8_t modbus_rtu_get_slave_id(void)
{
    return modbus_slave_id;
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


        /*
         * Response:
         *
         * [addr]
         * [fc]
         * [byte count]
         * [reg1 hi]
         * [reg1 lo]
         * ...
         * [CRC lo]
         * [CRC hi]
         */

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
     * FC06 - WRITE SINGLE REGISTER
     * ===================================================== */

    if (fc == MB_FC_WRITE_SINGLE_REGISTER)
    {
        /*
         * FC06 request tam olarak 8 byte'tir:
         *
         * [0] Slave Address
         * [1] 0x06
         * [2] Register Address Hi
         * [3] Register Address Lo
         * [4] Value Hi
         * [5] Value Lo
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


        uint16_t reg =
            ((uint16_t)req[2] << 8) |
            (uint16_t)req[3];

        uint16_t value =
            ((uint16_t)req[4] << 8) |
            (uint16_t)req[5];


        /*
         * Once register adresini kontrol et.
         */
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
         * Simdilik FC06 ile yalnizca LiBat'in
         * Modbus Address registerina yazmaya izin veriyoruz.
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


        /*
         * Register 154 icin:
         *
         * 1..247 gecerli.
         *
         * bms_reg_write() validation yapar.
         */
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
         *
         * ONEMLI:
         *
         * Cevap ESKI Slave ID ile gider.
         *
         * Ornegin:
         *
         * Slave 1'e "ID'ni 5 yap" denildiyse,
         *
         * response:
         * Slave = 1
         *
         * olur.
         *
         * Bu response gönderildikten sonra sonraki
         * requestler yeni ID=5 ile gelecektir.
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
         * Register degeri basariyla degistikten sonra
         * aktif Slave ID'yi yeni deger yap.
         *
         * Cevap tamponu yukarida eski addr ile zaten
         * olusturuldu.
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
         * [0] Slave Address
         * [1] 0x10
         * [2] Start Address Hi
         * [3] Start Address Lo
         * [4] Quantity Hi
         * [5] Quantity Lo
         * [6] Byte Count (= Quantity * 2)
         * [7..] register degerleri (Hi, Lo)
         * [..] CRC Lo, CRC Hi
         */

        /* En kisa FC16: 7 header + 2 veri + 2 CRC = 11 */
        if (reqlen < 11U)
        {
            return build_exception(resp, addr, fc, MB_EX_ILLEGAL_VALUE);
        }

        uint16_t start = ((uint16_t)req[2] << 8) | (uint16_t)req[3];
        uint16_t qty   = ((uint16_t)req[4] << 8) | (uint16_t)req[5];
        uint8_t  bc    = req[6];

        if (qty < 1U || qty > 123U || bc != (uint8_t)(qty * 2U))
        {
            return build_exception(resp, addr, fc, MB_EX_ILLEGAL_VALUE);
        }

        /* Toplam uzunluk: 7 header + bc veri + 2 CRC */
        if (reqlen != (uint16_t)(9U + bc))
        {
            return build_exception(resp, addr, fc, MB_EX_ILLEGAL_VALUE);
        }

        if ((uint32_t)start + (uint32_t)qty > (uint32_t)BMS_REG_COUNT)
        {
            return build_exception(resp, addr, fc, MB_EX_ILLEGAL_ADDRESS);
        }

        for (uint16_t i = 0U; i < qty; i++)
        {
            uint16_t v =
                ((uint16_t)req[7U + i * 2U] << 8) |
                (uint16_t)req[8U + i * 2U];

            if (bms_reg_write((uint16_t)(start + i), v) == 0U)
            {
                return build_exception(resp, addr, fc, MB_EX_ILLEGAL_VALUE);
            }
        }

        /* 40154 yazildiysa aktif Slave ID'yi guncelle */
        if ((start <= BMS_REG_MODBUS_ADDRESS) &&
            ((uint32_t)BMS_REG_MODBUS_ADDRESS < (uint32_t)start + (uint32_t)qty))
        {
            modbus_slave_id = (uint8_t)bms_reg_read(BMS_REG_MODBUS_ADDRESS);
        }

        /* Response: [addr][fc][startHi][startLo][qtyHi][qtyLo] + CRC */
        resp[0] = req[0];
        resp[1] = req[1];
        resp[2] = req[2];
        resp[3] = req[3];
        resp[4] = req[4];
        resp[5] = req[5];

        uint16_t crc16v = modbus_crc16(resp, 6U);
        resp[6] = (uint8_t)(crc16v & 0xFFU);
        resp[7] = (uint8_t)(crc16v >> 8);

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
