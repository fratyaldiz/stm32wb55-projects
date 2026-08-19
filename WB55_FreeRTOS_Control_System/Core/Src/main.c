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

  * This software is licensed under terms that can be found in the LICENSE file

  * in the root directory of this software component.

  * If no LICENSE file comes with this software, it is provided AS-IS.

  *

  ******************************************************************************

  */

/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/

#include "main.h"

#include "cmsis_os2.h"

#include "FreeRTOS.h"

/* Private includes ----------------------------------------------------------*/

/* USER CODE BEGIN Includes */

#include "rtos/app_tasks.h"
#include "rtos/command_queue.h"
#include "rtos/button_semaphore.h"
#include "rtos/uart_mutex.h"
#include "rtos/system_events.h"
#include "rtos/app_timers.h"
#include "rtos/uart_cli.h"

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

UART_HandleTypeDef huart1;

/* Definitions for defaultTask */

osThreadId_t defaultTaskHandle;

const osThreadAttr_t defaultTask_attributes = {

  .name = "defaultTask",

  .priority = (osPriority_t) osPriorityNormal,

  .stack_size = 128 * 4

};

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/

void SystemClock_Config(void);

void PeriphCommonClock_Config(void);

static void MX_GPIO_Init(void);

static void MX_USART1_UART_Init(void);

void StartDefaultTask(void *argument);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**

  * @brief  The application entry point.

  * @retval int

  */

int main(void)

{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */

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

  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Init scheduler */

  osKernelInitialize();

  /* USER CODE BEGIN RTOS_MUTEX */

  /*
   * FREERTOS CONCEPT: MUTEX
   * Protect the shared USART1 peripheral.
   * Implementation:
   * Core/Src/rtos/uart_mutex.c
   */
  UartMutex_Init();

/* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */

  /*
   * FREERTOS CONCEPT: BINARY SEMAPHORE
   * Creation is implemented in:
   * Core/Src/rtos/button_semaphore.c
   */
  ButtonSemaphore_Init();

/* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */

  /*
   * FREERTOS CONCEPT: SOFTWARE TIMERS
   *
   * - HeartbeatTimer   : periodic
   * - RunWarningTimer  : one-shot
   *
   * Full implementation:
   * Core/Src/rtos/app_timers.c
   */
  AppTimers_Init();

/* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */

  /*
   * Application command queue:
   * ButtonTask / UartRxTask / Timer callbacks -> ControlTask
   */
  CommandQueue_Init();

  /*
   * USART1 receive-byte queue:
   * UART ISR -> UartRxTask
   *
   * Full UART command implementation:
   * Core/Src/rtos/uart_cli.c
   */
  UartCli_Init();

/* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */

  /* creation of defaultTask */

  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */

  /* Application tasks are created in Core/Src/rtos/app_tasks.c */
  AppTasks_Init();

/* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */

  /*
   * FREERTOS CONCEPT: EVENT FLAGS
   * Implementation:
   * Core/Src/rtos/system_events.c
   */
  SystemEvents_Init();

/* USER CODE END RTOS_EVENTS */

  /* Start scheduler */

  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */

  /* USER CODE BEGIN WHILE */

  while (1)

  {

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

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

  /** Configure the main internal regulator output voltage

  */

  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters

  * in the RCC_OscInitTypeDef structure.

  */

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_MSI;

  RCC_OscInitStruct.HSIState = RCC_HSI_ON;

  RCC_OscInitStruct.MSIState = RCC_MSI_ON;

  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;

  RCC_OscInitStruct.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;

  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;

  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;

  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)

  {

    Error_Handler();

  }

  /** Configure the SYSCLKSource, HCLK, PCLK1 and PCLK2 clocks dividers

  */

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK4|RCC_CLOCKTYPE_HCLK2

                              |RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK

                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;

  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSI;

  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;

  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  RCC_ClkInitStruct.AHBCLK2Divider = RCC_SYSCLK_DIV1;

  RCC_ClkInitStruct.AHBCLK4Divider = RCC_SYSCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)

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

  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_SMPS;

  PeriphClkInitStruct.SmpsClockSelection = RCC_SMPSCLKSOURCE_HSI;

  PeriphClkInitStruct.SmpsDivSelection = RCC_SMPSCLKDIV_RANGE1;

  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)

  {

    Error_Handler();

  }

  /* USER CODE BEGIN Smps */

  /* USER CODE END Smps */

}

/**

  * @brief USART1 Initialization Function

  * @param None

  * @retval None

  */

static void MX_USART1_UART_Init(void)

{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */

  huart1.Instance = USART1;

  huart1.Init.BaudRate = 115200;

  huart1.Init.WordLength = UART_WORDLENGTH_8B;

  huart1.Init.StopBits = UART_STOPBITS_1;

  huart1.Init.Parity = UART_PARITY_NONE;

  huart1.Init.Mode = UART_MODE_TX_RX;

  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;

  huart1.Init.OverSampling = UART_OVERSAMPLING_16;

  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;

  huart1.Init.ClockPrescaler = UART_PRESCALER_DIV1;

  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

  if (HAL_UART_Init(&huart1) != HAL_OK)

  {

    Error_Handler();

  }

  if (HAL_UARTEx_SetTxFifoThreshold(&huart1, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)

  {

    Error_Handler();

  }

  if (HAL_UARTEx_SetRxFifoThreshold(&huart1, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)

  {

    Error_Handler();

  }

  if (HAL_UARTEx_DisableFifoMode(&huart1) != HAL_OK)

  {

    Error_Handler();

  }

  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**

  * @brief GPIO Initialization Function

  * @param None

  * @retval None

  */

static void MX_GPIO_Init(void)

{

  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */

  __HAL_RCC_GPIOC_CLK_ENABLE();

  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */

  HAL_GPIO_WritePin(GPIOB, LD2_Pin|LD1_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : SW1_Pin */

  GPIO_InitStruct.Pin = SW1_Pin;

  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;

  GPIO_InitStruct.Pull = GPIO_PULLUP;

  HAL_GPIO_Init(SW1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : LD2_Pin LD1_Pin */

  GPIO_InitStruct.Pin = LD2_Pin|LD1_Pin;

  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;

  GPIO_InitStruct.Pull = GPIO_NOPULL;

  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* EXTI interrupt init*/

  HAL_NVIC_SetPriority(EXTI4_IRQn, 5, 0);

  HAL_NVIC_EnableIRQ(EXTI4_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */

}

/* USER CODE BEGIN 4 */

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if (GPIO_Pin == SW1_Pin)
  {
    /*
     * Keep ISR work minimal.
     * The interrupt only signals ButtonTask through the semaphore.
     */
    ButtonSemaphore_NotifyFromISR();
  }
}

/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartDefaultTask */

/**

  * @brief  Function implementing the defaultTask thread.

  * @param  argument: Not used

  * @retval None

  */

/* USER CODE END Header_StartDefaultTask */

void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN 5 */
  osThreadExit();
  /* USER CODE END 5 */
}

/**

  * @brief  Period elapsed callback in non blocking mode

  * @note   This function is called  when TIM17 interrupt took place, inside

  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment

  * a global variable "uwTick" used as application time base.

  * @param  htim : TIM handle

  * @retval None

  */

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)

{

  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */

  if (htim->Instance == TIM17)

  {

    HAL_IncTick();

  }

  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */

}

/**

  * @brief  This function is executed in case of error occurrence.

  * @retval None

  */

void Error_Handler(void)

{

  /* USER CODE BEGIN Error_Handler_Debug */

  /* User can add his own implementation to report the HAL error return state */

  __disable_irq();

  while (1)

  {

  }

  /* USER CODE END Error_Handler_Debug */

}

#ifdef USE_FULL_ASSERT

/**

  * @brief  Reports the name of the source file and the source line number

  *         where the assert_param error has occurred.

  * @param  file: pointer to the source file name

  * @param  line: assert_param error line source number

  * @retval None

  */

void assert_failed(uint8_t *file, uint32_t line)

{

  /* USER CODE BEGIN 6 */

  /* User can add his own implementation to report the file name and line number,

     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* USER CODE END 6 */

}

#endif /* USE_FULL_ASSERT */