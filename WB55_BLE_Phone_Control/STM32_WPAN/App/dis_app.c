/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    App/dis_app.c
  * @author  MCD Application Team
  * @brief   Device Information Service Application
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
#include "ble.h"
#include "dis_app.h"

/* Private includes -----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "dis.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private defines -----------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macros ------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
#if ((BLE_CFG_DIS_SYSTEM_ID != 0) || (CFG_MENU_DEVICE_INFORMATION != 0))
static const uint8_t system_id[BLE_CFG_DIS_SYSTEM_ID_LEN_MAX] =
{
  (uint8_t)((DISAPP_MANUFACTURER_ID & 0xFF0000) >> 16),
  (uint8_t)((DISAPP_MANUFACTURER_ID & 0x00FF00) >> 8),
  (uint8_t)(DISAPP_MANUFACTURER_ID & 0x0000FF),
  0xFE,
  0xFF,
  (uint8_t)((DISAPP_OUI & 0xFF0000) >> 16),
  (uint8_t)((DISAPP_OUI & 0x00FF00) >> 8),
  (uint8_t)(DISAPP_OUI & 0x0000FF)
};
#endif

#if ((BLE_CFG_DIS_IEEE_CERTIFICATION != 0) || (CFG_MENU_DEVICE_INFORMATION != 0))
static const uint8_t ieee_id[BLE_CFG_DIS_IEEE_CERTIFICATION_LEN_MAX] =
{
  0xFE, 0xCA, 0xFE, 0xCA, 0xFE, 0xCA, 0xFE, 0xCA,
  0xFE, 0xCA, 0xFE, 0xCA, 0xFE, 0xCA, 0xFE, 0xCA,
  0xFE, 0xCA, 0xFE, 0xCA, 0xFE, 0xCA, 0xFE, 0xCA,
  0xFE, 0xCA, 0xFE, 0xCA, 0xFE, 0xCA, 0xFE, 0xCA,
};
#endif
#if ((BLE_CFG_DIS_PNP_ID != 0) || (CFG_MENU_DEVICE_INFORMATION != 0))
static const uint8_t pnp_id[BLE_CFG_DIS_PNP_ID_LEN_MAX] =
{
  0x1,
  0xAD, 0xDE,
  0xDE, 0xDA,
  0x01, 0x00
};
#endif
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Functions Definition ------------------------------------------------------*/
void DISAPP_Init(void)
{
/* USER CODE BEGIN DISAPP_Init */

  DIS_Data_t dis_data;

#if (BLE_CFG_DIS_MANUFACTURER_NAME_STRING != 0)
  static uint8_t manufacturer_name[] =
      DISAPP_MANUFACTURER_NAME;

  dis_data.pPayload = manufacturer_name;
  dis_data.Length =
      (uint8_t)(sizeof(manufacturer_name) - 1U);

  (void)DIS_UpdateChar(
      MANUFACTURER_NAME_UUID,
      &dis_data
  );
#endif

#if (BLE_CFG_DIS_MODEL_NUMBER_STRING != 0)
  static uint8_t model_number[] =
      DISAPP_MODEL_NUMBER;

  dis_data.pPayload = model_number;
  dis_data.Length =
      (uint8_t)(sizeof(model_number) - 1U);

  (void)DIS_UpdateChar(
      MODEL_NUMBER_UUID,
      &dis_data
  );
#endif

#if (BLE_CFG_DIS_HARDWARE_REVISION_STRING != 0)
  static uint8_t hardware_revision[] =
      DISAPP_HARDWARE_REVISION_NUMBER;

  dis_data.pPayload = hardware_revision;
  dis_data.Length =
      (uint8_t)(sizeof(hardware_revision) - 1U);

  (void)DIS_UpdateChar(
      HARDWARE_REVISION_UUID,
      &dis_data
  );
#endif

#if (BLE_CFG_DIS_FIRMWARE_REVISION_STRING != 0)
  static uint8_t firmware_revision[] =
      DISAPP_FIRMWARE_REVISION_NUMBER;

  dis_data.pPayload = firmware_revision;
  dis_data.Length =
      (uint8_t)(sizeof(firmware_revision) - 1U);

  (void)DIS_UpdateChar(
      FIRMWARE_REVISION_UUID,
      &dis_data
  );
#endif

/* USER CODE END DISAPP_Init */
}

/* USER CODE BEGIN FD */

/* USER CODE END FD */
