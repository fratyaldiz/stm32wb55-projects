/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : app_freertos.c
  * Description        : Code for freertos applications
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
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
extern UART_HandleTypeDef huart1;
/* USER CODE END Variables */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
/* USER CODE END FunctionPrototypes */

/* Hook prototypes */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName);

/* USER CODE BEGIN 4 */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
  static const char prefix[] =
      "\r\n*** FREERTOS STACK OVERFLOW DETECTED ***\r\nTask: ";
  static const char suffix[] =
      "\r\nSystem halted for debugging.\r\n";

  (void)xTask;

  /*
   * Bu hook sadece Stack Overflow deneyi icin kullanilir.
   * RTOS servisleri burada kullanilmiyor; cunku overflow aninda
   * ilgili task'in stack'i guvenilir kabul edilmez.
   */
  __disable_irq();

  HAL_UART_Transmit(
      &huart1,
      (uint8_t *)prefix,
      (uint16_t)(sizeof(prefix) - 1U),
      100U
  );

  if (pcTaskName != NULL)
  {
    HAL_UART_Transmit(
        &huart1,
        (uint8_t *)pcTaskName,
        (uint16_t)strlen(pcTaskName),
        100U
    );
  }

  HAL_UART_Transmit(
      &huart1,
      (uint8_t *)suffix,
      (uint16_t)(sizeof(suffix) - 1U),
      100U
  );

  /*
   * Overflow sonrasinda devam etmek yerine burada dur.
   * Debugger ile sistem bu noktada incelenebilir.
   */
  while (1)
  {
  }
}
/* USER CODE END 4 */

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
/* USER CODE END Application */
