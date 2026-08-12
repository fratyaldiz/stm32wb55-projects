/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "modbus_rtu.h"
#include "bms_registers.h"
#include "stm32wbxx_ll_adc.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define ONCHIP_UPDATE_PERIOD_MS  500U

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

static uint32_t last_onchip_update_ms = 0U;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void PeriphCommonClock_Config(void);

/* USER CODE BEGIN PFP */

static void Update_Board_Digital_IO(void);
static void Update_OnChip_Data(void);

static int32_t Calculate_Temperature_x10(
    uint32_t vdda_mv,
    uint32_t temperature_raw,
    int32_t temperature_c_hal
);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */


/*
 * STM32WB55 dahili sicaklik sensorunu
 * 0.1 C biriminde hesaplar.
 *
 * Ornek:
 *
 * 253 -> 25.3 C
 * 317 -> 31.7 C
 *
 * Hesap STM32'nin fabrikada programlanan
 * TS_CAL1 ve TS_CAL2 degerlerini kullanir.
 *
 * HAL'in __HAL_ADC_CALC_TEMPERATURE() makrosu
 * tam sayi derece C dondurdugu icin 0.1 C gosterim
 * icin ayni fabrika kalibrasyon degerleriyle
 * fixed-point hesap yapiyoruz.
 */
static int32_t Calculate_Temperature_x10(
    uint32_t vdda_mv,
    uint32_t temperature_raw,
    int32_t temperature_c_hal)
{
    int32_t cal1;
    int32_t cal2;

    int32_t adc_at_calibration_vref;

    int32_t denominator;

    int64_t numerator;

    int32_t temperature_x10;


    /*
     * Fabrika kalibrasyon ADC degerleri.
     *
     * STM32WB:
     *
     * TS_CAL1 -> 30 C
     * TS_CAL2 -> 130 C
     */
    cal1 = (int32_t)(*TEMPSENSOR_CAL1_ADDR);
    cal2 = (int32_t)(*TEMPSENSOR_CAL2_ADDR);


    /*
     * Sicaklik sensorunun ADC degerini,
     * fabrika kalibrasyonunun yapildigi
     * referans gerilimine normalize et.
     *
     * ST HAL/LL makrosunun kullandigi
     * yaklasimla aynidir:
     *
     * ADC_normalized =
     * ADC_raw * VDDA / calibration_VREF
     */
    adc_at_calibration_vref =
        (int32_t)(
            ((uint64_t)temperature_raw *
             (uint64_t)vdda_mv)
            /
            (uint64_t)TEMPSENSOR_CAL_VREFANALOG
        );


    denominator = cal2 - cal1;


    /*
     * Normalde denominator sifir olamaz.
     *
     * Yine de koruma amacli HAL'in
     * tam derece sonucuna geri donuyoruz.
     */
    if (denominator == 0)
    {
        return temperature_c_hal * 10;
    }


    /*
     * Lineer interpolasyon:
     *
     * T =
     *
     * (ADC - CAL1)
     * ------------------- * (T2 - T1)
     *   (CAL2 - CAL1)
     *
     * + T1
     *
     * Sonucu dogrudan x10 hesapliyoruz.
     */
    numerator =
        ((int64_t)adc_at_calibration_vref -
         (int64_t)cal1)
        *
        (
            (int64_t)(
                TEMPSENSOR_CAL2_TEMP -
                TEMPSENSOR_CAL1_TEMP
            )
            * 10LL
        );


    temperature_x10 =
        (int32_t)(
            numerator /
            (int64_t)denominator
        )
        +
        (int32_t)(
            TEMPSENSOR_CAL1_TEMP * 10L
        );


    return temperature_x10;
}


/*
 * Board butonlarini Modbus Discrete Input hafizasina aktarir
 * ve Modbus coil durumlarini fiziksel LED'lere uygular.
 */
static void Update_Board_Digital_IO(void)
{
    uint8_t sw1_pressed;
    uint8_t sw2_pressed;
    uint8_t sw3_pressed;


    /*
     * Butonlar active-low:
     *
     * GPIO RESET -> basili
     * GPIO SET   -> birakilmis
     */
    sw1_pressed =
        (HAL_GPIO_ReadPin(
            SW1_GPIO_Port,
            SW1_Pin) == GPIO_PIN_RESET)
        ? 1U : 0U;

    sw2_pressed =
        (HAL_GPIO_ReadPin(
            SW2_GPIO_Port,
            SW2_Pin) == GPIO_PIN_RESET)
        ? 1U : 0U;

    sw3_pressed =
        (HAL_GPIO_ReadPin(
            SW3_GPIO_Port,
            SW3_Pin) == GPIO_PIN_RESET)
        ? 1U : 0U;


    /*
     * Fiziksel buton -> Modbus Discrete Input
     */
    modbus_set_discrete_input(
        0U,
        sw1_pressed
    );

    modbus_set_discrete_input(
        1U,
        sw2_pressed
    );

    modbus_set_discrete_input(
        2U,
        sw3_pressed
    );


    /*
     * Modbus Coil -> fiziksel LED
     *
     * LED'ler active-high:
     *
     * GPIO SET   -> LED ON
     * GPIO RESET -> LED OFF
     */

    HAL_GPIO_WritePin(
        LD1_GPIO_Port,
        LD1_Pin,
        (modbus_get_coil(0U) != 0U)
            ? GPIO_PIN_SET
            : GPIO_PIN_RESET
    );


    HAL_GPIO_WritePin(
        LD2_GPIO_Port,
        LD2_Pin,
        (modbus_get_coil(1U) != 0U)
            ? GPIO_PIN_SET
            : GPIO_PIN_RESET
    );


    HAL_GPIO_WritePin(
        LD3_GPIO_Port,
        LD3_Pin,
        (modbus_get_coil(2U) != 0U)
            ? GPIO_PIN_SET
            : GPIO_PIN_RESET
    );
}


/*
 * STM32WB55 dahili ADC verilerini okur.
 *
 * ADC sequence:
 *
 * Rank 1 -> VREFINT
 * Rank 2 -> Temperature Sensor
 *
 * Sonuclar:
 *
 * PDU 50 -> MCU temperature x10
 * PDU 51 -> Temperature raw ADC
 * PDU 52 -> VDDA mV
 * PDU 53 -> Uptime seconds
 */
static void Update_OnChip_Data(void)
{
    uint32_t vrefint_raw = 0U;
    uint32_t temperature_raw = 0U;

    uint32_t vdda_mv = 0U;

    int32_t temperature_c_hal = 0;
    int32_t temperature_x10_signed = 0;

    uint16_t temperature_x10 = 0U;

    uint8_t adc_ok = 1U;


    /*
     * ADC conversion sequence'i baslat.
     */
    if (HAL_ADC_Start(&hadc1) != HAL_OK)
    {
        adc_ok = 0U;
    }


    /*
     * Rank 1 -> VREFINT
     */
    if (adc_ok != 0U)
    {
        if (HAL_ADC_PollForConversion(
                &hadc1,
                10U) == HAL_OK)
        {
            vrefint_raw =
                HAL_ADC_GetValue(&hadc1);
        }
        else
        {
            adc_ok = 0U;
        }
    }


    /*
     * Rank 2 -> Internal Temperature Sensor
     */
    if (adc_ok != 0U)
    {
        if (HAL_ADC_PollForConversion(
                &hadc1,
                10U) == HAL_OK)
        {
            temperature_raw =
                HAL_ADC_GetValue(&hadc1);
        }
        else
        {
            adc_ok = 0U;
        }
    }


    /*
     * ADC sequence tamamlandi.
     */
    HAL_ADC_Stop(&hadc1);


    /*
     * ADC okuma basariliysa hesaplamalari yap.
     */
    if ((adc_ok != 0U) &&
        (vrefint_raw != 0U))
    {
        /*
         * VREFINT fabrika kalibrasyonunu kullanarak
         * gercek VDDA degerini hesapla.
         *
         * Sonuc mV.
         */
        vdda_mv =
            __HAL_ADC_CALC_VREFANALOG_VOLTAGE(
                vrefint_raw,
                ADC_RESOLUTION_12B
            );


        /*
         * ST HAL'in fabrika TS_CAL degerlerini kullanan
         * standart sicaklik hesabi.
         *
         * Sonuc tam sayi derece Celsius.
         *
         * Ornek:
         *
         * 31
         * 32
         */
        temperature_c_hal =
            __HAL_ADC_CALC_TEMPERATURE(
                vdda_mv,
                temperature_raw,
                ADC_RESOLUTION_12B
            );


        /*
         * Ayni fabrika kalibrasyon verileriyle
         * daha ince fixed-point hesap:
         *
         * 317 -> 31.7 C
         */
        temperature_x10_signed =
            Calculate_Temperature_x10(
                vdda_mv,
                temperature_raw,
                temperature_c_hal
            );


        /*
         * Holding Register uint16_t.
         *
         * Bu proje normal oda sicakliginda calistigi icin
         * negatif sonucu 0'a sinirliyoruz.
         */
        if (temperature_x10_signed < 0)
        {
            temperature_x10 = 0U;
        }
        else if (temperature_x10_signed > 65535)
        {
            temperature_x10 = 65535U;
        }
        else
        {
            temperature_x10 =
                (uint16_t)temperature_x10_signed;
        }


        /*
         * PDU 50
         *
         * MCU Temperature x10
         *
         * Ornek:
         *
         * 317 -> 31.7 C
         */
        bms_reg_write(
            BMS_REG_MCU_TEMPERATURE_X10,
            temperature_x10
        );


        /*
         * PDU 51
         *
         * Temperature Sensor Raw ADC
         */
        bms_reg_write(
            BMS_REG_MCU_TEMPERATURE_RAW,
            (uint16_t)temperature_raw
        );


        /*
         * PDU 52
         *
         * VDDA mV
         *
         * Ornek:
         *
         * 3310 -> 3.310 V
         */
        bms_reg_write(
            BMS_REG_VDDA_MV,
            (uint16_t)vdda_mv
        );
    }


    /*
     * PDU 53
     *
     * HAL_GetTick() milisaniyedir.
     *
     * /1000 -> saniye
     *
     * uint16_t oldugu icin:
     *
     * 65535 -> 0
     *
     * seklinde wrap eder.
     */
    bms_reg_write(
        BMS_REG_UPTIME_SECONDS,
        (uint16_t)(
            HAL_GetTick() / 1000U
        )
    );
}


/* USER CODE END 0 */


/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
    /* USER CODE BEGIN 1 */

    uint8_t  rxbuf[256];
    uint16_t rxlen = 0U;
    uint8_t  txbuf[256];

    /* USER CODE END 1 */


    /* MCU Configuration--------------------------------------------------------*/

    /*
     * Reset of all peripherals,
     * Initializes the Flash interface and the Systick.
     */
    HAL_Init();


    /* USER CODE BEGIN Init */

    /* USER CODE END Init */


    /* Configure the system clock */
    SystemClock_Config();


    /* Configure the peripherals common clocks */
    PeriphCommonClock_Config();


    /* USER CODE BEGIN SysInit */

    /* USER CODE END SysInit */


    /* Initialize all configured peripherals */
    MX_GPIO_Init();
    MX_USART1_UART_Init();
    MX_LPUART1_UART_Init();
    MX_ADC1_Init();


    /* USER CODE BEGIN 2 */


    /*
     * Holding Register hafizasini hazirla.
     */
    bms_registers_init();


    /*
     * Modbus RTU hafizasini ve Slave ID'yi hazirla.
     */
    modbus_rtu_init();


    /*
     * Waveshare USB-TTL:
     *
     * LPUART1
     *
     * PA2 = TX
     * PA3 = RX
     *
     * 9600 8N1
     */
    hlpuart1.Init.BaudRate = 9600;


    if (HAL_UART_Init(&hlpuart1) != HAL_OK)
    {
        Error_Handler();
    }


    /*
     * ADC kalibrasyonu.
     *
     * ADC kullanilmadan once bir kez yapilir.
     */
    if (HAL_ADCEx_Calibration_Start(
            &hadc1,
            ADC_SINGLE_ENDED) != HAL_OK)
    {
        Error_Handler();
    }


    /*
     * Baslangicta:
     *
     * buton -> DI
     * coil  -> LED
     *
     * durumlarini esitle.
     */
    Update_Board_Digital_IO();


    /*
     * Ilk ADC ve uptime degerlerini
     * hemen olustur.
     */
    Update_OnChip_Data();


    last_onchip_update_ms =
        HAL_GetTick();


    /* USER CODE END 2 */


    /* Infinite loop */
    /* USER CODE BEGIN WHILE */

    while (1)
    {
        /* USER CODE END WHILE */


        /* USER CODE BEGIN 3 */


        /*
         * =====================================================
         * BOARD DIGITAL I/O
         * =====================================================
         *
         * SW1 -> DI0
         * SW2 -> DI1
         * SW3 -> DI2
         *
         * Coil0 -> LD1
         * Coil1 -> LD2
         * Coil2 -> LD3
         */
        Update_Board_Digital_IO();


        /*
         * =====================================================
         * PERIODIC ON-CHIP DATA
         * =====================================================
         *
         * Her 500 ms:
         *
         * PDU 50 -> temperature x10
         * PDU 51 -> temperature raw ADC
         * PDU 52 -> VDDA mV
         * PDU 53 -> uptime seconds
         */

        uint32_t now_ms =
            HAL_GetTick();


        if ((uint32_t)(
                now_ms -
                last_onchip_update_ms
            )
            >= ONCHIP_UPDATE_PERIOD_MS)
        {
            last_onchip_update_ms =
                now_ms;

            Update_OnChip_Data();
        }


        /*
         * =====================================================
         * MODBUS RTU
         * =====================================================
         *
         * Mevcut calisan LPUART1 Modbus blogu.
         */

        rxlen = 0U;


        HAL_UARTEx_ReceiveToIdle(
            &hlpuart1,
            rxbuf,
            sizeof(rxbuf),
            &rxlen,
            100U
        );


        if (rxlen > 0U)
        {
            uint16_t txlen =
                modbus_rtu_process(
                    rxbuf,
                    rxlen,
                    txbuf
                );


            if (txlen > 0U)
            {
                HAL_UART_Transmit(
                    &hlpuart1,
                    txbuf,
                    txlen,
                    HAL_MAX_DELAY
                );
            }
        }


        /*
         * FC05 ile coil degistiyse
         * LED'i hemen fiziksel GPIO'ya uygula.
         */
        Update_Board_Digital_IO();
    }

    /* USER CODE END 3 */
}


/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};


    /** Macro to configure the PLL multiplication factor
     */
    __HAL_RCC_PLL_PLLM_CONFIG(
        RCC_PLLM_DIV1
    );


    /** Macro to configure the PLL clock source
     */
    __HAL_RCC_PLL_PLLSOURCE_CONFIG(
        RCC_PLLSOURCE_MSI
    );


    /** Configure the main internal regulator output voltage
     */
    __HAL_PWR_VOLTAGESCALING_CONFIG(
        PWR_REGULATOR_VOLTAGE_SCALE1
    );


    /** Initializes the RCC Oscillators
     */
    RCC_OscInitStruct.OscillatorType =
        RCC_OSCILLATORTYPE_HSI |
        RCC_OSCILLATORTYPE_MSI;


    RCC_OscInitStruct.HSIState =
        RCC_HSI_ON;


    RCC_OscInitStruct.MSIState =
        RCC_MSI_ON;


    RCC_OscInitStruct.HSICalibrationValue =
        RCC_HSICALIBRATION_DEFAULT;


    RCC_OscInitStruct.MSICalibrationValue =
        RCC_MSICALIBRATION_DEFAULT;


    RCC_OscInitStruct.MSIClockRange =
        RCC_MSIRANGE_6;


    RCC_OscInitStruct.PLL.PLLState =
        RCC_PLL_NONE;


    if (HAL_RCC_OscConfig(
            &RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }


    /** Configure SYSCLK, HCLK, PCLK1 and PCLK2
     */
    RCC_ClkInitStruct.ClockType =
        RCC_CLOCKTYPE_HCLK4 |
        RCC_CLOCKTYPE_HCLK2 |
        RCC_CLOCKTYPE_HCLK |
        RCC_CLOCKTYPE_SYSCLK |
        RCC_CLOCKTYPE_PCLK1 |
        RCC_CLOCKTYPE_PCLK2;


    RCC_ClkInitStruct.SYSCLKSource =
        RCC_SYSCLKSOURCE_MSI;


    RCC_ClkInitStruct.AHBCLKDivider =
        RCC_SYSCLK_DIV1;


    RCC_ClkInitStruct.APB1CLKDivider =
        RCC_HCLK_DIV1;


    RCC_ClkInitStruct.APB2CLKDivider =
        RCC_HCLK_DIV1;


    RCC_ClkInitStruct.AHBCLK2Divider =
        RCC_SYSCLK_DIV1;


    RCC_ClkInitStruct.AHBCLK4Divider =
        RCC_SYSCLK_DIV1;


    if (HAL_RCC_ClockConfig(
            &RCC_ClkInitStruct,
            FLASH_LATENCY_0) != HAL_OK)
    {
        Error_Handler();
    }
}


/**
  * @brief Peripherals Common Clock Configuration
  * @retval None
  */
void PeriphCommonClock_Config(void)
{
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};


    /** Initializes the peripherals clock
     */
    PeriphClkInitStruct.PeriphClockSelection =
        RCC_PERIPHCLK_SMPS;


    PeriphClkInitStruct.SmpsClockSelection =
        RCC_SMPSCLKSOURCE_HSI;


    PeriphClkInitStruct.SmpsDivSelection =
        RCC_SMPSCLKDIV_RANGE1;


    if (HAL_RCCEx_PeriphCLKConfig(
            &PeriphClkInitStruct) != HAL_OK)
    {
        Error_Handler();
    }


    /* USER CODE BEGIN Smps */

    /* USER CODE END Smps */
}


/* USER CODE BEGIN 4 */

/* USER CODE END 4 */


/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
    /* USER CODE BEGIN Error_Handler_Debug */

    __disable_irq();

    while (1)
    {
    }

    /* USER CODE END Error_Handler_Debug */
}


#ifdef USE_FULL_ASSERT

/**
  * @brief Reports the name of the source file and the source line number
  *        where the assert_param error has occurred.
  * @param file: pointer to the source file name
  * @param line: assert_param error line source number
  * @retval None
  */
void assert_failed(
    uint8_t *file,
    uint32_t line)
{
    /* USER CODE BEGIN 6 */

    /* USER CODE END 6 */
}

#endif /* USE_FULL_ASSERT */
