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
 * DIGITAL I/O COUNTS
 * ========================================================= */

/*
 * Coil:
 * 0 -> LD1
 * 1 -> LD2
 * 2 -> LD3
 */
#define MODBUS_COIL_COUNT            3U

/*
 * Discrete Input:
 * 0 -> SW1
 * 1 -> SW2
 * 2 -> SW3
 */
#define MODBUS_DISCRETE_INPUT_COUNT  3U


/* =========================================================
 * FUNCTION CODES
 * ========================================================= */

#define MB_FC_READ_COILS                0x01U
#define MB_FC_READ_DISCRETE_INPUTS      0x02U
#define MB_FC_READ_HOLDING              0x03U
#define MB_FC_WRITE_SINGLE_COIL         0x05U
#define MB_FC_WRITE_SINGLE_REGISTER     0x06U
#define MB_FC_WRITE_MULTIPLE            0x10U


/* =========================================================
 * EXCEPTION CODES
 * ========================================================= */

#define MB_EX_ILLEGAL_FUNCTION           0x01U
#define MB_EX_ILLEGAL_ADDRESS            0x02U
#define MB_EX_ILLEGAL_VALUE              0x03U


/*
 * Modbus RTU modulu baslangic ayarlari.
 *
 * main.c tarafinda bms_registers_init() sonrasinda
 * bir kez cagrilmalidir.
 */
void modbus_rtu_init(void);


/*
 * O anda aktif olan Modbus Slave ID'yi dondurur.
 */
uint8_t modbus_rtu_get_slave_id(void);


/* =========================================================
 * COIL / DISCRETE INPUT ACCESS
 * ========================================================= */

/*
 * Bir coil'in mevcut durumunu dondurur.
 *
 * index:
 * 0 -> LD1
 * 1 -> LD2
 * 2 -> LD3
 *
 * Donus:
 * 0 = OFF
 * 1 = ON
 */
uint8_t modbus_get_coil(uint16_t index);


/*
 * Bir discrete input durumunu gunceller.
 *
 * main.c, fiziksel butonlari okuyup bu fonksiyona verir.
 *
 * index:
 * 0 -> SW1
 * 1 -> SW2
 * 2 -> SW3
 *
 * state:
 * 0 = pasif
 * 1 = aktif
 */
void modbus_set_discrete_input(uint16_t index, uint8_t state);


/*
 * Bir discrete input'un mevcut durumunu dondurur.
 *
 * Donus:
 * 0 = pasif
 * 1 = aktif
 */
uint8_t modbus_get_discrete_input(uint16_t index);


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
 * > 0:
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
