/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    App/hts_app.c
  * @author  MCD Application Team
  * @brief   Health Thermometer Service Application
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "app_common.h"
#include "dbg_trace.h"
#include "app_ble.h"
#include "ble.h"
#include "hts_app.h"
#include <time.h>
#include "stm32_seq.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "custom_app.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
typedef struct
{
  HTS_TemperatureValue_t TemperatureMeasurementChar;
#if(BLE_CFG_HTS_TEMPERATURE_TYPE_VALUE_STATIC == 1)
  HTS_Temperature_Type_t TemperatureTypeChar;
#endif
#if(BLE_CFG_HTS_INTERMEDIATE_TEMPERATURE == 1)
  HTS_TemperatureValue_t IntermediateTemperatureChar;
  uint8_t TimerIntTemp_Id;
  uint8_t IntTempEnabled;
#endif
#if(BLE_CFG_HTS_MEASUREMENT_INTERVAL == 1)
  uint16_t  MeasurementIntervalChar;
  uint8_t   TimerMeasInt_Id;
  uint8_t   Indication_Status;
#endif
  uint8_t TimerMeasurement_Id;
  uint8_t TimerMeasurementStarted;
} HTSAPP_Context_t;
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private defines ------------------------------------------------------------*/
#define DEFAULT_HTS_MEASUREMENT_INTERVAL   (1000000/CFG_TS_TICK_VAL)  /**< 1s */
#define DEFAULT_TEMPERATURE_TYPE          TT_Armpit
#define NB_SAVED_MEASURES                                                     10
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macros -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/**
 * START of Section BLE_APP_CONTEXT
 */

static HTSAPP_Context_t HTSAPP_Context;
static HTS_TemperatureValue_t HTSMeasurement[NB_SAVED_MEASURES];
static int8_t HTS_CurrentIndex, HTS_OldIndex;

/**
 * END of Section BLE_APP_CONTEXT
 */

 /* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
static void HTSAPP_Update_TimeStamp(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Functions Definition ------------------------------------------------------*/
static void HTSAPP_UpdateMeasurement( void )
{
/* USER CODE BEGIN HTSAPP_UpdateMeasurement */

  /*
   * Timer callback icinde dogrudan BLE komutu gondermiyoruz.
   * Isi sequencer'a birakiyoruz.
   */
  UTIL_SEQ_SetTask(
      1U << CFG_TASK_HTS_MEAS_REQ_ID,
      CFG_SCH_PRIO_0
  );

/* USER CODE END HTSAPP_UpdateMeasurement */

  return;
}

#if(BLE_CFG_HTS_INTERMEDIATE_TEMPERATURE == 1)
static void HTSAPP_UpdateIntermediateTemperature( void )
{
/* USER CODE BEGIN HTSAPP_UpdateIntermediateTemperature */

/* USER CODE END HTSAPP_UpdateIntermediateTemperature */
  return;
}
#endif

#if(BLE_CFG_HTS_MEASUREMENT_INTERVAL == 1)
static void HTSAPP_UpdateMeasurementInterval( void )
{
/* USER CODE BEGIN HTSAPP_UpdateMeasurementInterval */

/* USER CODE END HTSAPP_UpdateMeasurementInterval */
  return;
}
#endif

#if(BLE_CFG_HTS_TIME_STAMP_FLAG != 0)
static void HTSAPP_Update_TimeStamp(void)
{
/* USER CODE BEGIN HTSAPP_Update_TimeStamp */

/* USER CODE END HTSAPP_Update_TimeStamp */
}
#endif

#if(BLE_CFG_HTS_TIME_STAMP_FLAG != 0)
static void HTSAPP_Store(void)
{
/* USER CODE BEGIN HTSAPP_Store */

/* USER CODE END HTSAPP_Store */
}

static void HTSAPP_Suppress(void)
{
/* USER CODE BEGIN HTSAPP_Suppress */

/* USER CODE END HTSAPP_Suppress */
}
#endif

/* Public functions ----------------------------------------------------------*/

void HTS_App_Notification(HTS_App_Notification_evt_t *pNotification)
{
/* USER CODE BEGIN HTS_App_Notification */

  switch (pNotification->HTS_Evt_Opcode)
  {
    case HTS_MEASUREMENT_IND_ENABLED_EVT:

      /*
       * Telefon Temperature Measurement indication'a
       * abone oldu.
       */
#if (BLE_CFG_HTS_MEASUREMENT_INTERVAL == 1)
      HTSAPP_Context.Indication_Status = 1U;
#endif

      if (HTSAPP_Context.TimerMeasurementStarted == 0U)
      {
        /*
         * Ilk degeri hemen gonder.
         */
        HTSAPP_Measurement();

        /*
         * Sonra saniyede bir devam et.
         */
        HW_TS_Stop(HTSAPP_Context.TimerMeasurement_Id);

        HW_TS_Start(
            HTSAPP_Context.TimerMeasurement_Id,
            DEFAULT_HTS_MEASUREMENT_INTERVAL
        );

        HTSAPP_Context.TimerMeasurementStarted = 1U;
      }

      break;

    case HTS_MEASUREMENT_IND_DISABLED_EVT:

#if (BLE_CFG_HTS_MEASUREMENT_INTERVAL == 1)
      HTSAPP_Context.Indication_Status = 0U;
#endif

      HW_TS_Stop(HTSAPP_Context.TimerMeasurement_Id);
      HTSAPP_Context.TimerMeasurementStarted = 0U;

      break;

    default:
      break;
  }

/* USER CODE END HTS_App_Notification */

  return;
}

void HTSAPP_Init(void)
{
/* USER CODE BEGIN HTSAPP_Init */

  /*
   * Sicaklik olcum task'ini sequencer'a kaydet.
   */
  UTIL_SEQ_RegTask(
      1U << CFG_TASK_HTS_MEAS_REQ_ID,
      UTIL_SEQ_RFU,
      HTSAPP_Measurement
  );

  /*
   * Health Thermometer measurement ayarlari.
   */
  HTSAPP_Context.TemperatureMeasurementChar.Flags =
      (uint8_t)NO_FLAGS;

#if (BLE_CFG_HTS_TEMPERATURE_TYPE_VALUE_STATIC == 0)

  HTSAPP_Context.TemperatureMeasurementChar.TemperatureType =
      TT_Body;

  HTSAPP_Context.TemperatureMeasurementChar.Flags |=
      (uint8_t)SENSOR_TEMPERATURE_TYPE_PRESENT;

#endif

#if (BLE_CFG_HTS_MEASUREMENT_INTERVAL == 1)

  /*
   * 1 saniye.
   */
  HTSAPP_Context.MeasurementIntervalChar = 1U;
  HTSAPP_Context.Indication_Status = 0U;

#endif

  /*
   * Sicaklik timer'ini olustur.
   */
  HW_TS_Create(
      CFG_TIM_PROC_ID_ISR,
      &(HTSAPP_Context.TimerMeasurement_Id),
      hw_ts_Repeated,
      HTSAPP_UpdateMeasurement
  );

  HTSAPP_Context.TimerMeasurementStarted = 0U;

/* USER CODE END HTSAPP_Init */

  return;
}

void HTSAPP_Measurement(void)
{
/* USER CODE BEGIN HTSAPP_Measurement */

  int32_t temperature_c;

  /*
   * STM32WB55 dahili sicaklik sensorunu ADC ile oku.
   */
  if (Custom_APP_Read_Internal_Temperature(&temperature_c) == HAL_OK)
  {
    /*
     * Health Thermometer Measurement Value:
     *
     * IEEE-11073 FLOAT
     * exponent = 0
     * mantissa = derece C
     *
     * Ornek:
     * 32 -> uygulamada 32.0 C
     */
    HTSAPP_Context.TemperatureMeasurementChar.MeasurementValue =
        ((uint32_t)temperature_c & 0x00FFFFFFU);

#if (BLE_CFG_HTS_MEASUREMENT_INTERVAL == 1)

    if (HTSAPP_Context.Indication_Status != 0U)
    {
      HTS_Update_Char(
          TEMPERATURE_MEASUREMENT_CHAR_UUID,
          (uint8_t *)&HTSAPP_Context.TemperatureMeasurementChar
      );
    }

#else

    HTS_Update_Char(
        TEMPERATURE_MEASUREMENT_CHAR_UUID,
        (uint8_t *)&HTSAPP_Context.TemperatureMeasurementChar
    );

#endif
  }

/* USER CODE END HTSAPP_Measurement */

  return;
}

void HTSAPP_IntermediateTemperature(void)
{
/* USER CODE BEGIN HTSAPP_IntermediateTemperature */

/* USER CODE END HTSAPP_IntermediateTemperature */
  return;
}

#if(BLE_CFG_HTS_MEASUREMENT_INTERVAL == 1)
void HTSAPP_MeasurementInterval(void)
{
/* USER CODE BEGIN HTSAPP_MeasurementInterval */

/* USER CODE END HTSAPP_MeasurementInterval */
  return;
}
#endif

/**
 * @brief  Application service update characteristic
 * @param  None
 * @retval None
 */
void HTSAPP_Profile_UpdateChar(void)
{
/* USER CODE BEGIN HTSAPP_Profile_UpdateChar */

/* USER CODE END HTSAPP_Profile_UpdateChar */
}

/* USER CODE BEGIN FD */

/* USER CODE END FD */
