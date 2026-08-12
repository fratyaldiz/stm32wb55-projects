#ifndef BMS_REGISTERS_H
#define BMS_REGISTERS_H

#include <stdint.h>

/*
 * LiBat BMS Holding Register modeli.
 *
 * LiBat dokumaninda:
 *
 * PLC Address 40103 -> Register 103
 * PLC Address 40111 -> Register 111
 * PLC Address 40154 -> Register 154
 *
 * Bu nedenle:
 *
 * PDU Register = PLC Address - 40000
 */

#define BMS_REG_COUNT 200U

#define BMS_PLC_BASE 40000U


/* =========================================================
 * VERSION / IDENTIFICATION
 * ========================================================= */

#define BMS_REG_SOFTWARE_VERSION_MAJOR       88U
#define BMS_REG_SOFTWARE_VERSION_MINOR       89U
#define BMS_REG_SOFTWARE_VERSION_PATCH       90U

#define BMS_REG_HARDWARE_VERSION_MAJOR       91U
#define BMS_REG_HARDWARE_VERSION_MINOR       92U
#define BMS_REG_HARDWARE_VERSION_PATCH       93U

#define BMS_REG_SERIAL_NUMBER_1              94U
#define BMS_REG_SERIAL_NUMBER_2              95U
#define BMS_REG_SERIAL_NUMBER_3              96U
#define BMS_REG_SERIAL_NUMBER_4              97U

#define BMS_REG_BOOTLOADER_VERSION_1         98U
#define BMS_REG_BOOTLOADER_VERSION_2         99U
#define BMS_REG_BOOTLOADER_VERSION_3         100U
#define BMS_REG_BOOTLOADER_VERSION_4         101U

#define BMS_REG_MODEL_NUMBER                 102U


/* =========================================================
 * MAIN BATTERY DATA
 * ========================================================= */

#define BMS_REG_PACK_VOLTAGE                 103U
#define BMS_REG_PACK_CURRENT                 104U

/* 105-106 Reserved */

#define BMS_REG_SOC                          107U
#define BMS_REG_MIN_CELL_VOLTAGE             108U
#define BMS_REG_MAX_CELL_VOLTAGE             109U
#define BMS_REG_MIN_TEMPERATURE              110U
#define BMS_REG_MAX_TEMPERATURE              111U

/* 112-113 Reserved */


/* =========================================================
 * BATTERY STATUS
 * ========================================================= */

#define BMS_REG_BATTERY_STATUS_MSB           114U
#define BMS_REG_BATTERY_STATUS_47_32         115U
#define BMS_REG_BATTERY_STATUS_31_16         116U
#define BMS_REG_BATTERY_STATUS_LSB           117U

/* 118-128 Reserved */


/* =========================================================
 * SLAVE DATA
 * ========================================================= */

#define BMS_REG_SLAVE_DATA_SELECT            129U
#define BMS_REG_CELL_COUNT                   130U


/* =========================================================
 * CELL VOLTAGES
 * ========================================================= */

#define BMS_REG_CELL_1_VOLTAGE               131U
#define BMS_REG_CELL_2_VOLTAGE               132U
#define BMS_REG_CELL_3_VOLTAGE               133U
#define BMS_REG_CELL_4_VOLTAGE               134U
#define BMS_REG_CELL_5_VOLTAGE               135U
#define BMS_REG_CELL_6_VOLTAGE               136U
#define BMS_REG_CELL_7_VOLTAGE               137U
#define BMS_REG_CELL_8_VOLTAGE               138U
#define BMS_REG_CELL_9_VOLTAGE               139U
#define BMS_REG_CELL_10_VOLTAGE              140U
#define BMS_REG_CELL_11_VOLTAGE              141U
#define BMS_REG_CELL_12_VOLTAGE              142U
#define BMS_REG_CELL_13_VOLTAGE              143U
#define BMS_REG_CELL_14_VOLTAGE              144U
#define BMS_REG_CELL_15_VOLTAGE              145U
#define BMS_REG_CELL_16_VOLTAGE              146U
#define BMS_REG_CELL_17_VOLTAGE              147U
#define BMS_REG_CELL_18_VOLTAGE              148U


/* =========================================================
 * TEMPERATURE SENSORS
 * ========================================================= */

#define BMS_REG_TEMPERATURE_1                149U
#define BMS_REG_TEMPERATURE_2                150U
#define BMS_REG_TEMPERATURE_3                151U
#define BMS_REG_TEMPERATURE_4                152U
#define BMS_REG_TEMPERATURE_5                153U


/* =========================================================
 * MODBUS ADDRESS
 * ========================================================= */

#define BMS_REG_MODBUS_ADDRESS               154U


extern uint16_t bms_holding_regs[BMS_REG_COUNT];


void bms_registers_init(void);

uint16_t bms_reg_read(uint16_t index);

/*
 * Bir Holding Register'a yazmayi dener.
 *
 * Donus:
 * 1 = yazma kabul edildi
 * 0 = register yazilabilir degil veya deger gecersiz
 *
 * Simdilik LiBat dokumaninda FC06 ile acikca tanimlanan
 * Register 154 (Modbus Address) desteklenmektedir.
 */
uint8_t bms_reg_write(uint16_t index, uint16_t value);


#endif /* BMS_REGISTERS_H */
