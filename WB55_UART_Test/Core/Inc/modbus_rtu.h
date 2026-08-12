#ifndef MODBUS_RTU_H
#define MODBUS_RTU_H

#include <stdint.h>


/*
 * Varsayilan Modbus Slave adresi.
 *
 * LiBat dokumanina gore varsayilan adres 1'dir.
 *
 * Register 154 FC06 ile yazildiginda aktif adres
 * calisma sirasinda degisebilir.
 */
#define MODBUS_DEFAULT_SLAVE_ID 1U


/* =========================================================
 * FUNCTION CODES
 * ========================================================= */

#define MB_FC_READ_HOLDING           0x03U
#define MB_FC_WRITE_SINGLE_REGISTER  0x06U
#define MB_FC_WRITE_MULTIPLE         0x10U


/* =========================================================
 * EXCEPTION CODES
 * ========================================================= */

#define MB_EX_ILLEGAL_FUNCTION       0x01U
#define MB_EX_ILLEGAL_ADDRESS        0x02U
#define MB_EX_ILLEGAL_VALUE          0x03U


/*
 * Modbus RTU modulu baslangic ayarlari.
 *
 * main.c tarafinda bms_registers_init() sonrasinda
 * bir kez cagrilmalidir.
 */
void modbus_rtu_init(void);


/*
 * O anda aktif olan Modbus Slave ID'yi dondurur.
 *
 * Test/debug amaciyla da kullanilabilir.
 */
uint8_t modbus_rtu_get_slave_id(void);


/*
 * Gelen Modbus RTU frame'ini isler.
 *
 * req:
 * Gelen RTU frame
 *
 * reqlen:
 * Gelen frame uzunlugu
 *
 * resp:
 * Olusturulacak cevap
 *
 * Donus:
 *
 * 0:
 * cevap yok
 *
 * >0:
 * gonderilecek response uzunlugu
 *
 * CRC hatasi veya frame baska bir Slave ID'ye aitse
 * Modbus geregi sessiz kalinir.
 *
 * resp tamponu en az 256 byte olmalidir.
 */
uint16_t modbus_rtu_process(
    const uint8_t *req,
    uint16_t reqlen,
    uint8_t *resp
);


#endif /* MODBUS_RTU_H */
