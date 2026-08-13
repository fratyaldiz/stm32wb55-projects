/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    App/custom_app.c
  * @author  MCD Application Team
  * @brief   Custom Example Application (Server)
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
#include "main.h"
#include "app_common.h"
#include "dbg_trace.h"
#include "ble.h"
#include "custom_app.h"
#include "custom_stm.h"
#include "stm32_seq.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
typedef struct
{
  /* My_P2P_Server */
  uint8_t               Switch_c_Notification_Status;
  /* USER CODE BEGIN CUSTOM_APP_Context_t */

  /* USER CODE END CUSTOM_APP_Context_t */

  uint16_t              ConnectionHandle;
} Custom_App_Context_t;

/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private defines ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define SW1_POLL_PERIOD_MS       20U
#define SW1_DEBOUNCE_SAMPLES     3U


/* USER CODE END PD */

/* Private macros -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/**
 * START of Section BLE_APP_CONTEXT
 */

static Custom_App_Context_t Custom_App_Context;

/**
 * END of Section BLE_APP_CONTEXT
 */

uint8_t UpdateCharData[512];
uint8_t NotifyCharData[512];
uint16_t Connection_Handle;
/* USER CODE BEGIN PV */

static uint32_t Switch_LastPollTick = 0U;

static uint8_t Switch_LastRawState[3] = {0U, 0U, 0U};
static uint8_t Switch_StableState[3] = {0U, 0U, 0U};
static uint8_t Switch_SameSampleCount[3] = {0U, 0U, 0U};

/* 0xFF = henuz telefona gonderilmedi */
static uint8_t Switch_LastNotifiedState[3] =
{
  0xFFU,
  0xFFU,
  0xFFU
};
extern ADC_HandleTypeDef hadc1;



/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* My_P2P_Server */
static void Custom_Switch_c_Update_Char(void);

static void Custom_Switch_c_Send_Notification(void);

/* USER CODE BEGIN PFP */

HAL_StatusTypeDef Custom_APP_Read_Internal_Temperature(
    int32_t *temperature_c
);
/* USER CODE END PFP */

/* Functions Definition ------------------------------------------------------*/
void Custom_STM_App_Notification(Custom_STM_App_Notification_evt_t *pNotification)
{
  /* USER CODE BEGIN CUSTOM_STM_App_Notification_1 */

  /* USER CODE END CUSTOM_STM_App_Notification_1 */
  switch (pNotification->Custom_Evt_Opcode)
  {
    /* USER CODE BEGIN CUSTOM_STM_App_Notification_Custom_Evt_Opcode */

    /* USER CODE END CUSTOM_STM_App_Notification_Custom_Evt_Opcode */

    /* My_P2P_Server */
    case CUSTOM_STM_LED_C_READ_EVT:
      /* USER CODE BEGIN CUSTOM_STM_LED_C_READ_EVT */

      /*
       * Simdilik read isteginde ek bir islem yapmiyoruz.
       */

      /* USER CODE END CUSTOM_STM_LED_C_READ_EVT */
      break;

    case CUSTOM_STM_LED_C_WRITE_NO_RESP_EVT:
      /* USER CODE BEGIN CUSTOM_STM_LED_C_WRITE_NO_RESP_EVT */

      /*
       * iPhone -> FE41 -> STM32
       *
       * 2 byte komut:
       *
       * Byte 0 = LED numarasi
       * Byte 1 = durum
       *
       * 01 01 -> LD1 ON
       * 01 00 -> LD1 OFF
       *
       * 02 01 -> LD2 ON
       * 02 00 -> LD2 OFF
       *
       * 03 01 -> LD3 ON
       * 03 00 -> LD3 OFF
       */

      if ((pNotification->DataTransfered.pPayload != NULL) &&
          (pNotification->DataTransfered.Length >= 2U))
      {
        uint8_t led_number;
        uint8_t led_state;
        GPIO_PinState gpio_state;

        led_number = pNotification->DataTransfered.pPayload[0];
        led_state  = pNotification->DataTransfered.pPayload[1];

        /*
         * Sadece 00 ve 01 kabul et.
         */
        if (led_state <= 0x01U)
        {
          gpio_state =
              (led_state == 0x01U)
              ? GPIO_PIN_SET
              : GPIO_PIN_RESET;

          switch (led_number)
          {
            case 0x01U:
              HAL_GPIO_WritePin(
                  LD1_GPIO_Port,
                  LD1_Pin,
                  gpio_state
              );
              break;

            case 0x02U:
              HAL_GPIO_WritePin(
                  LD2_GPIO_Port,
                  LD2_Pin,
                  gpio_state
              );
              break;

            case 0x03U:
              HAL_GPIO_WritePin(
                  LD3_GPIO_Port,
                  LD3_Pin,
                  gpio_state
              );
              break;

            default:
              /*
               * Gecersiz LED numarasi.
               */
              break;
          }
        }
      }

      /* USER CODE END CUSTOM_STM_LED_C_WRITE_NO_RESP_EVT */
      break;

    case CUSTOM_STM_LED_C_WRITE_EVT:
      /* USER CODE BEGIN CUSTOM_STM_LED_C_WRITE_EVT */

      /* USER CODE END CUSTOM_STM_LED_C_WRITE_EVT */
      break;

    case CUSTOM_STM_SWITCH_C_NOTIFY_ENABLED_EVT:
      /* USER CODE BEGIN CUSTOM_STM_SWITCH_C_NOTIFY_ENABLED_EVT */

      Custom_App_Context.Switch_c_Notification_Status = 1U;

      /*
       * iPhone Notify'i actiginda mevcut SW1 durumunu
       * bir kez hemen gondermeye zorla.
       */
      Switch_LastNotifiedState[0] = 0xFFU;
      Switch_LastNotifiedState[1] = 0xFFU;
      Switch_LastNotifiedState[2] = 0xFFU;

      /* USER CODE END CUSTOM_STM_SWITCH_C_NOTIFY_ENABLED_EVT */
      break;

    case CUSTOM_STM_SWITCH_C_NOTIFY_DISABLED_EVT:
      /* USER CODE BEGIN CUSTOM_STM_SWITCH_C_NOTIFY_DISABLED_EVT */

      /*
       * iPhone FE42 notification aboneligini kapatti.
       */
      Custom_App_Context.Switch_c_Notification_Status = 0U;
      Switch_LastNotifiedState[0] = 0xFFU;
      Switch_LastNotifiedState[1] = 0xFFU;
      Switch_LastNotifiedState[2] = 0xFFU;

      /* USER CODE END CUSTOM_STM_SWITCH_C_NOTIFY_DISABLED_EVT */
      break;

    case CUSTOM_STM_NOTIFICATION_COMPLETE_EVT:
      /* USER CODE BEGIN CUSTOM_STM_NOTIFICATION_COMPLETE_EVT */

      /* USER CODE END CUSTOM_STM_NOTIFICATION_COMPLETE_EVT */
      break;

    default:
      /* USER CODE BEGIN CUSTOM_STM_App_Notification_default */

      /* USER CODE END CUSTOM_STM_App_Notification_default */
      break;
  }
  /* USER CODE BEGIN CUSTOM_STM_App_Notification_2 */

  /* USER CODE END CUSTOM_STM_App_Notification_2 */
  return;
}

void Custom_APP_Notification(Custom_App_ConnHandle_Not_evt_t *pNotification)
{
  /* USER CODE BEGIN CUSTOM_APP_Notification_1 */

  /* USER CODE END CUSTOM_APP_Notification_1 */

  switch (pNotification->Custom_Evt_Opcode)
  {
    /* USER CODE BEGIN CUSTOM_APP_Notification_Custom_Evt_Opcode */

    /* USER CODE END P2PS_CUSTOM_Notification_Custom_Evt_Opcode */
    case CUSTOM_CONN_HANDLE_EVT :
      /* USER CODE BEGIN CUSTOM_CONN_HANDLE_EVT */

      /*
       * BLE connection kuruldu.
       */

      /* USER CODE END CUSTOM_CONN_HANDLE_EVT */
      break;

    case CUSTOM_DISCON_HANDLE_EVT :
      /* USER CODE BEGIN CUSTOM_DISCON_HANDLE_EVT */

      /*
       * BLE connection koptugunda notification
       * durumunu sifirla.
       */
      Custom_App_Context.Switch_c_Notification_Status = 0U;

      /* USER CODE END CUSTOM_DISCON_HANDLE_EVT */
      break;

    default:
      /* USER CODE BEGIN CUSTOM_APP_Notification_default */

      /* USER CODE END CUSTOM_APP_Notification_default */
      break;
  }

  /* USER CODE BEGIN CUSTOM_APP_Notification_2 */

  /* USER CODE END CUSTOM_APP_Notification_2 */

  return;
}

void Custom_APP_Init(void)
{
  /* USER CODE BEGIN CUSTOM_APP_Init */

  Custom_App_Context.Switch_c_Notification_Status = 0U;

  /*
   * Firmware baslangicinda LED'ler kapali.
   */
  HAL_GPIO_WritePin(LD1_GPIO_Port, LD1_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_RESET);

  /*
   * Butonlar active-low:
   *
   * GPIO_PIN_RESET = basili
   * GPIO_PIN_SET   = birakilmis
   *
   * Dizide:
   * [0] = SW1
   * [1] = SW2
   * [2] = SW3
   */

  Switch_StableState[0] =
      (HAL_GPIO_ReadPin(SW1_GPIO_Port, SW1_Pin) == GPIO_PIN_RESET)
      ? 1U
      : 0U;

  Switch_StableState[1] =
      (HAL_GPIO_ReadPin(SW2_GPIO_Port, SW2_Pin) == GPIO_PIN_RESET)
      ? 1U
      : 0U;

  Switch_StableState[2] =
      (HAL_GPIO_ReadPin(SW3_GPIO_Port, SW3_Pin) == GPIO_PIN_RESET)
      ? 1U
      : 0U;

  /*
   * Debounce baslangic degerleri.
   */
  for (uint8_t i = 0U; i < 3U; i++)
  {
    Switch_LastRawState[i] = Switch_StableState[i];
    Switch_SameSampleCount[i] = SW1_DEBOUNCE_SAMPLES;
    Switch_LastNotifiedState[i] = 0xFFU;
  }

  Switch_LastPollTick = HAL_GetTick();
  /*
   * ADC kalibrasyonunu bir kez yap.
   */
  if (HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED) != HAL_OK)
  {
    Error_Handler();
  }



  /* USER CODE END CUSTOM_APP_Init */
  return;
}

/* USER CODE BEGIN FD */

void Custom_APP_Process(void)
{
  uint32_t now;
  uint8_t raw_state[3];

  now = HAL_GetTick();

  /*
   * Butonlari her 20 ms'de bir oku.
   */
  if ((now - Switch_LastPollTick) < SW1_POLL_PERIOD_MS)
  {
    return;
  }

  Switch_LastPollTick = now;

  /*
   * Butonlar active-low.
   *
   * basili    = 1
   * birakildi = 0
   */
  raw_state[0] =
      (HAL_GPIO_ReadPin(SW1_GPIO_Port, SW1_Pin) == GPIO_PIN_RESET)
      ? 1U : 0U;

  raw_state[1] =
      (HAL_GPIO_ReadPin(SW2_GPIO_Port, SW2_Pin) == GPIO_PIN_RESET)
      ? 1U : 0U;

  raw_state[2] =
      (HAL_GPIO_ReadPin(SW3_GPIO_Port, SW3_Pin) == GPIO_PIN_RESET)
      ? 1U : 0U;

  /*
   * Her buton icin debounce.
   */
  for (uint8_t i = 0U; i < 3U; i++)
  {
    if (raw_state[i] == Switch_LastRawState[i])
    {
      if (Switch_SameSampleCount[i] < SW1_DEBOUNCE_SAMPLES)
      {
        Switch_SameSampleCount[i]++;
      }
    }
    else
    {
      Switch_LastRawState[i] = raw_state[i];
      Switch_SameSampleCount[i] = 1U;
    }

    if (Switch_SameSampleCount[i] >= SW1_DEBOUNCE_SAMPLES)
    {
      Switch_StableState[i] = Switch_LastRawState[i];
    }
  }

  /*
   * Telefon FE42 Notify'a aboneyse,
   * degisen butonlari telefona gonder.
   */
  if (Custom_App_Context.Switch_c_Notification_Status != 0U)
  {
    for (uint8_t i = 0U; i < 3U; i++)
    {
      if (Switch_LastNotifiedState[i] != Switch_StableState[i])
      {
        /*
         * Byte 0 = buton numarasi (01 / 02 / 03)
         * Byte 1 = durum      (00 / 01)
         */
        NotifyCharData[0] = i + 1U;
        NotifyCharData[1] = Switch_StableState[i];

        if (Custom_STM_App_Update_Char(
                CUSTOM_STM_SWITCH_C,
                (uint8_t *)NotifyCharData) == BLE_STATUS_SUCCESS)
        {
          Switch_LastNotifiedState[i] = Switch_StableState[i];
        }
      }
    }
  }
}
HAL_StatusTypeDef Custom_APP_Read_Internal_Temperature(
    int32_t *temperature_c
)
{
  uint32_t vref_raw;
  uint32_t temp_raw;
  uint32_t vdda_mv;

  if (temperature_c == NULL)
  {
    return HAL_ERROR;
  }

  /*
   * ADC sequence:
   *
   * Rank 1 = VREFINT
   * Rank 2 = TEMPSENSOR
   */
  if (HAL_ADC_Start(&hadc1) != HAL_OK)
  {
    return HAL_ERROR;
  }

  /*
   * Rank 1: VREFINT
   */
  if (HAL_ADC_PollForConversion(&hadc1, 10U) != HAL_OK)
  {
    HAL_ADC_Stop(&hadc1);
    return HAL_TIMEOUT;
  }

  vref_raw = HAL_ADC_GetValue(&hadc1);

  /*
   * Rank 2: internal temperature sensor
   */
  if (HAL_ADC_PollForConversion(&hadc1, 10U) != HAL_OK)
  {
    HAL_ADC_Stop(&hadc1);
    return HAL_TIMEOUT;
  }

  temp_raw = HAL_ADC_GetValue(&hadc1);

  HAL_ADC_Stop(&hadc1);

  /*
   * Sifira bolme korumasi.
   */
  if (vref_raw == 0U)
  {
    return HAL_ERROR;
  }

  /*
   * Once gercek VDDA/VREF+ degerini hesapla.
   */
  vdda_mv =
      __HAL_ADC_CALC_VREFANALOG_VOLTAGE(
          vref_raw,
          ADC_RESOLUTION_12B
      );

  /*
   * STM32'nin fabrikada yazilmis sicaklik
   * kalibrasyon degerlerini kullanarak derece C hesapla.
   */
  *temperature_c =
      __HAL_ADC_CALC_TEMPERATURE(
          vdda_mv,
          temp_raw,
          ADC_RESOLUTION_12B
      );

  return HAL_OK;
}

/* USER CODE END FD */

/*************************************************************
 *
 * LOCAL FUNCTIONS
 *
 *************************************************************/

/* My_P2P_Server */
__USED void Custom_Switch_c_Update_Char(void) /* Property Read */
{
  uint8_t updateflag = 0;

  /* USER CODE BEGIN Switch_c_UC_1*/

  /* USER CODE END Switch_c_UC_1*/

  if (updateflag != 0)
  {
    Custom_STM_App_Update_Char(CUSTOM_STM_SWITCH_C, (uint8_t *)UpdateCharData);
  }

  /* USER CODE BEGIN Switch_c_UC_Last*/

  /* USER CODE END Switch_c_UC_Last*/
  return;
}

__USED void Custom_Switch_c_Send_Notification(void) /* Property Notification */
{
  uint8_t updateflag = 0;

  /* USER CODE BEGIN Switch_c_NS_1*/

  /*
   * SW1 notification kodunu bir sonraki adimda
   * buraya ekleyecegiz.
   */

  /* USER CODE END Switch_c_NS_1*/

  if (updateflag != 0)
  {
    Custom_STM_App_Update_Char(CUSTOM_STM_SWITCH_C, (uint8_t *)UpdateCharData);
  }

  /* USER CODE BEGIN Switch_c_NS_Last*/

  /* USER CODE END Switch_c_NS_Last*/

  return;
}

/* USER CODE BEGIN FD_LOCAL_FUNCTIONS*/

/* USER CODE END FD_LOCAL_FUNCTIONS*/
