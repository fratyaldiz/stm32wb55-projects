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

#include <string.h>

#include <stdio.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define BLUE_LED_SLOW_PERIOD_MS     500U

#define BLUE_LED_FAST_PERIOD_MS     100U

#define GREEN_LED_PERIOD_MS        1000U

#define BLUE_EVENT_FLAG            (1U << 0)

#define GREEN_EVENT_FLAG           (1U << 1)

#define LED_EVENTS_MASK            (BLUE_EVENT_FLAG | GREEN_EVENT_FLAG)

#define PERIODIC_TIMER_PERIOD_MS    1000U

#define ONE_SHOT_TIMER_PERIOD_MS    5000U

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart1;

/* Definitions for BlueLedTask */
osThreadId_t BlueLedTaskHandle;
const osThreadAttr_t BlueLedTask_attributes = {
  .name = "BlueLedTask",
  .priority = (osPriority_t) osPriorityNormal,
  .stack_size = 128 * 4
};
/* Definitions for GreenLedTask */
osThreadId_t GreenLedTaskHandle;
const osThreadAttr_t GreenLedTask_attributes = {
  .name = "GreenLedTask",
  .priority = (osPriority_t) osPriorityNormal,
  .stack_size = 128 * 4
};
/* Definitions for ButtonTask */
osThreadId_t ButtonTaskHandle;
const osThreadAttr_t ButtonTask_attributes = {
  .name = "ButtonTask",
  .priority = (osPriority_t) osPriorityNormal,
  .stack_size = 128 * 4
};
/* Definitions for UartTask */
osThreadId_t UartTaskHandle;
const osThreadAttr_t UartTask_attributes = {
  .name = "UartTask",
  .priority = (osPriority_t) osPriorityNormal,
  .stack_size = 128 * 4
};
/* Definitions for LowTask */
osThreadId_t LowTaskHandle;
const osThreadAttr_t LowTask_attributes = {
  .name = "LowTask",
  .priority = (osPriority_t) osPriorityBelowNormal,
  .stack_size = 128 * 4
};
/* Definitions for MediumTask */
osThreadId_t MediumTaskHandle;
const osThreadAttr_t MediumTask_attributes = {
  .name = "MediumTask",
  .priority = (osPriority_t) osPriorityNormal,
  .stack_size = 128 * 4
};
/* Definitions for HighTask */
osThreadId_t HighTaskHandle;
const osThreadAttr_t HighTask_attributes = {
  .name = "HighTask",
  .priority = (osPriority_t) osPriorityAboveNormal,
  .stack_size = 128 * 4
};
/* Definitions for MonitorTask */
osThreadId_t MonitorTaskHandle;
const osThreadAttr_t MonitorTask_attributes = {
  .name = "MonitorTask",
  .priority = (osPriority_t) osPriorityLow,
  .stack_size = 512 * 4
};
/* Definitions for ButtonQueue */
osMessageQueueId_t ButtonQueueHandle;
const osMessageQueueAttr_t ButtonQueue_attributes = {
  .name = "ButtonQueue"
};
/* Definitions for PeriodicTimer */
osTimerId_t PeriodicTimerHandle;
const osTimerAttr_t PeriodicTimer_attributes = {
  .name = "PeriodicTimer"
};
/* Definitions for OneShotTimer */
osTimerId_t OneShotTimerHandle;
const osTimerAttr_t OneShotTimer_attributes = {
  .name = "OneShotTimer"
};
/* Definitions for ResourceMutex */
osMutexId_t ResourceMutexHandle;
const osMutexAttr_t ResourceMutex_attributes = {
  .name = "ResourceMutex"
};
/* Definitions for ButtonSemaphore */
osSemaphoreId_t ButtonSemaphoreHandle;
const osSemaphoreAttr_t ButtonSemaphore_attributes = {
  .name = "ButtonSemaphore"
};
/* Definitions for UartSemaphore */
osSemaphoreId_t UartSemaphoreHandle;
const osSemaphoreAttr_t UartSemaphore_attributes = {
  .name = "UartSemaphore"
};
/* Definitions for ResourceSemaphore */
osSemaphoreId_t ResourceSemaphoreHandle;
const osSemaphoreAttr_t ResourceSemaphore_attributes = {
  .name = "ResourceSemaphore"
};
/* Definitions for SystemEvents */
osEventFlagsId_t SystemEventsHandle;
const osEventFlagsAttr_t SystemEvents_attributes = {
  .name = "SystemEvents"
};
/* USER CODE BEGIN PV */

static volatile uint8_t demoLowHasResource = 0U;

static volatile uint8_t demoHighWaiting = 0U;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void PeriphCommonClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
void StartDefaultTask(void *argument);
void StartGreenLedTask(void *argument);
void StartButtonTask(void *argument);
void StartUartTask(void *argument);
void StartTask05(void *argument);
void StartTask06(void *argument);
void StartTask07(void *argument);
void StartMonitorTask(void *argument);
void PTCallback(void *argument);
void OSTCallback(void *argument);

/* USER CODE BEGIN PFP */

static void UART_Log(const char *message);

static void UART_LogStackInfo(const char *taskName,
                              osThreadId_t taskHandle,
                              uint32_t totalStackBytes);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

static void UART_Log(const char *message)
{
  if (osSemaphoreAcquire(UartSemaphoreHandle, osWaitForever) == osOK)
  {
    HAL_UART_Transmit(
        &huart1,
        (uint8_t *)message,
        (uint16_t)strlen(message),
        1000U
    );

    (void)osSemaphoreRelease(UartSemaphoreHandle);
  }
}

static void UART_LogStackInfo(const char *taskName,
                              osThreadId_t taskHandle,
                              uint32_t totalStackBytes)
{
  char line[128];
  uint32_t minFreeBytes;
  uint32_t maxUsedBytes;

  /*
   * Bir task'in simdiye kadar gordugu minimum bos stack miktarini
   * byte cinsinden al.
   */
  minFreeBytes = osThreadGetStackSpace(taskHandle);

  if (minFreeBytes <= totalStackBytes)
  {
    maxUsedBytes = totalStackBytes - minFreeBytes;
  }
  else
  {
    maxUsedBytes = 0U;
  }

  (void)snprintf(
      line,
      sizeof(line),
      "%-12s total=%4lu B | min free=%4lu B | max used~=%4lu B\r\n",
      taskName,
      (unsigned long)totalStackBytes,
      (unsigned long)minFreeBytes,
      (unsigned long)maxUsedBytes
  );

  UART_Log(line);
}

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

  /*

   * Both LEDs start OFF.

   * CubeMX already initializes them LOW,

   * but keeping the intended initial state clear is useful.

   */

  HAL_GPIO_WritePin(LD1_GPIO_Port, LD1_Pin, GPIO_PIN_RESET);

  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();
  /* Create the mutex(es) */
  /* creation of ResourceMutex */
  ResourceMutexHandle = osMutexNew(&ResourceMutex_attributes);

  /* USER CODE BEGIN RTOS_MUTEX */

  /* add mutexes, ... */

  /* USER CODE END RTOS_MUTEX */

  /* Create the semaphores(s) */
  /* creation of ButtonSemaphore */
  ButtonSemaphoreHandle = osSemaphoreNew(1, 0, &ButtonSemaphore_attributes);

  /* creation of UartSemaphore */
  UartSemaphoreHandle = osSemaphoreNew(1, 1, &UartSemaphore_attributes);

  /* creation of ResourceSemaphore */
  ResourceSemaphoreHandle = osSemaphoreNew(1, 1, &ResourceSemaphore_attributes);

  /* USER CODE BEGIN RTOS_SEMAPHORES */

  /* add semaphores, ... */

  /* USER CODE END RTOS_SEMAPHORES */

  /* Create the timer(s) */
  /* creation of PeriodicTimer */
  PeriodicTimerHandle = osTimerNew(PTCallback, osTimerPeriodic, NULL, &PeriodicTimer_attributes);

  /* creation of OneShotTimer */
  OneShotTimerHandle = osTimerNew(OSTCallback, osTimerOnce, NULL, &OneShotTimer_attributes);

  /* USER CODE BEGIN RTOS_TIMERS */

  /* start timers, add new ones, ... */

  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* creation of ButtonQueue */
  ButtonQueueHandle = osMessageQueueNew (4, sizeof(uint32_t), &ButtonQueue_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */

  /* add queues, ... */

  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of BlueLedTask */
  BlueLedTaskHandle = osThreadNew(StartDefaultTask, NULL, &BlueLedTask_attributes);

  /* creation of GreenLedTask */
  GreenLedTaskHandle = osThreadNew(StartGreenLedTask, NULL, &GreenLedTask_attributes);

  /* creation of ButtonTask */
  ButtonTaskHandle = osThreadNew(StartButtonTask, NULL, &ButtonTask_attributes);

  /* creation of UartTask */
  UartTaskHandle = osThreadNew(StartUartTask, NULL, &UartTask_attributes);

  /* creation of LowTask */
  LowTaskHandle = osThreadNew(StartTask05, NULL, &LowTask_attributes);

  /* creation of MediumTask */
  MediumTaskHandle = osThreadNew(StartTask06, NULL, &MediumTask_attributes);

  /* creation of HighTask */
  HighTaskHandle = osThreadNew(StartTask07, NULL, &HighTask_attributes);

  /* creation of MonitorTask */
  MonitorTaskHandle = osThreadNew(StartMonitorTask, NULL, &MonitorTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */

  /* add threads, ... */

  /* USER CODE END RTOS_THREADS */

  /* creation of SystemEvents */
  SystemEventsHandle = osEventFlagsNew(&SystemEvents_attributes);

  /* USER CODE BEGIN RTOS_EVENTS */

  /* add events, ... */

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
  __HAL_RCC_GPIOA_CLK_ENABLE();

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

    if (ButtonSemaphoreHandle != NULL)

    {

      (void)osSemaphoreRelease(ButtonSemaphoreHandle);

    }

  }

}

/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartDefaultTask */

/**

  * @brief  Function implementing the BlueLedTask thread.

  * @param  argument: Not used

  * @retval None

  */

/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN 5 */

  uint32_t blinkPeriod = BLUE_LED_SLOW_PERIOD_MS;

  uint32_t newPeriod = BLUE_LED_SLOW_PERIOD_MS;

  for (;;)

  {

    while (osMessageQueueGet(

               ButtonQueueHandle,

               &newPeriod,

               NULL,

               0U) == osOK)

    {

      if ((newPeriod == BLUE_LED_FAST_PERIOD_MS) ||

          (newPeriod == BLUE_LED_SLOW_PERIOD_MS))

      {

        blinkPeriod = newPeriod;

      }

    }

    HAL_GPIO_TogglePin(

        LD1_GPIO_Port,

        LD1_Pin

    );

    (void)osEventFlagsSet(

        SystemEventsHandle,

        BLUE_EVENT_FLAG

    );

    osDelay(blinkPeriod);

  }

  /* USER CODE END 5 */
}

/* USER CODE BEGIN Header_StartGreenLedTask */

/**

  * @brief Function implementing the GreenLedTask thread.

  * @param argument: Not used

  * @retval None

  */

/* USER CODE END Header_StartGreenLedTask */
void StartGreenLedTask(void *argument)
{
  /* USER CODE BEGIN StartGreenLedTask */

  for (;;)

  {

    HAL_GPIO_TogglePin(

        LD2_GPIO_Port,

        LD2_Pin

    );

    (void)osEventFlagsSet(

        SystemEventsHandle,

        GREEN_EVENT_FLAG

    );

    osDelay(GREEN_LED_PERIOD_MS);

  }

  /* USER CODE END StartGreenLedTask */
}

/* USER CODE BEGIN Header_StartButtonTask */

/**

* @brief Function implementing the ButtonTask thread.

* @param argument: Not used

* @retval None

*/

/* USER CODE END Header_StartButtonTask */
void StartButtonTask(void *argument)
{
  /* USER CODE BEGIN StartButtonTask */

  uint8_t fastMode = 0U;
  uint32_t newPeriod;
  uint32_t i;

  /*
   * STACK MANAGEMENT DEMO
   *
   * 100 byte'lik yerel dizi dogrudan ButtonTask'in stack frame'i
   * icinde tutuluyor. volatile kullanarak derleyicinin diziyi
   * optimize edip kaldirmasini engelliyoruz.
   */
  volatile uint8_t stackDemoBuffer[100];

  /*
   * High-water-mark mekanizmasinin bu bolgeyi gercekten kullanilmis
   * olarak gorebilmesi icin dizinin tamamini bir kez yaziyoruz.
   */
  for (i = 0U; i < sizeof(stackDemoBuffer); i++)
  {
    stackDemoBuffer[i] = (uint8_t)i;
  }

  for (;;)
  {
    if (osSemaphoreAcquire(
            ButtonSemaphoreHandle,
            osWaitForever) == osOK)
    {
      osDelay(50);

      if (HAL_GPIO_ReadPin(
              SW1_GPIO_Port,
              SW1_Pin) == GPIO_PIN_RESET)
      {
        fastMode ^= 1U;

        if (fastMode != 0U)
        {
          newPeriod = BLUE_LED_FAST_PERIOD_MS;
        }
        else
        {
          newPeriod = BLUE_LED_SLOW_PERIOD_MS;
        }

        (void)osMessageQueuePut(
            ButtonQueueHandle,
            &newPeriod,
            0U,
            0U
        );

        UART_Log("[BUTTON] SW1 pressed -> Blue LED speed changed.\r\n");

        while (HAL_GPIO_ReadPin(
                   SW1_GPIO_Port,
                   SW1_Pin) == GPIO_PIN_RESET)
        {
          osDelay(10);
        }

        osDelay(20);

        while (osSemaphoreAcquire(
                   ButtonSemaphoreHandle,
                   0U) == osOK)
        {
          /* Drain bounce events. */
        }
      }
    }
  }

  /* USER CODE END StartButtonTask */
}

/* USER CODE BEGIN Header_StartUartTask */

/**

* @brief Function implementing the UartTask thread.

* @param argument: Not used

* @retval None

*/

/* USER CODE END Header_StartUartTask */
void StartUartTask(void *argument)
{
  /* USER CODE BEGIN StartUartTask */

  /*
   * Stack Management deneyinde terminali MonitorTask kullanacak.
   * UartTask mevcut kalir fakat pasif bekler.
   */
  for (;;)
  {
    osDelay(1000);
  }

  /* USER CODE END StartUartTask */
}

/* USER CODE BEGIN Header_StartTask05 */

/**

* @brief Function implementing the LowTask thread.

* @param argument: Not used

* @retval None

*/

/* USER CODE END Header_StartTask05 */
void StartTask05(void *argument)
{
  /* USER CODE BEGIN StartTask05 */

  /* LowTask: not used in the Event Flags demo. */

  osThreadExit();

  /* USER CODE END StartTask05 */
}

/* USER CODE BEGIN Header_StartTask06 */

/**

* @brief Function implementing the MediumTask thread.

* @param argument: Not used

* @retval None

*/

/* USER CODE END Header_StartTask06 */
void StartTask06(void *argument)
{
  /* USER CODE BEGIN StartTask06 */

  /* MediumTask: not used in the Event Flags demo. */

  osThreadExit();

  /* USER CODE END StartTask06 */
}

/* USER CODE BEGIN Header_StartTask07 */

/**

* @brief Function implementing the HighTask thread.

* @param argument: Not used

* @retval None

*/

/* USER CODE END Header_StartTask07 */
void StartTask07(void *argument)
{
  /* USER CODE BEGIN StartTask07 */

  /* HighTask: not used in the Event Flags demo. */

  osThreadExit();

  /* USER CODE END StartTask07 */
}

/* USER CODE BEGIN Header_StartMonitorTask */

/**

* @brief Function implementing the MonitorTask thread.

* @param argument: Not used

* @retval None

*/

/* USER CODE END Header_StartMonitorTask */
void StartMonitorTask(void *argument)
{
  /* USER CODE BEGIN StartMonitorTask */

  osDelay(1500);

  UART_Log("\r\n=== FREERTOS STACK MONITOR ===\r\n");
  UART_Log("min free = simdiye kadar gorulen EN AZ bos stack miktari\r\n");
  UART_Log("Deger ne kadar kucukse stack sinirina o kadar yaklasilmistir.\r\n\r\n");

  for (;;)
  {
    UART_Log("--------------- STACK REPORT ---------------\r\n");

    UART_LogStackInfo(
        "BlueLedTask",
        BlueLedTaskHandle,
        (uint32_t)BlueLedTask_attributes.stack_size
    );

    UART_LogStackInfo(
        "GreenLedTask",
        GreenLedTaskHandle,
        (uint32_t)GreenLedTask_attributes.stack_size
    );

    UART_LogStackInfo(
        "ButtonTask",
        ButtonTaskHandle,
        (uint32_t)ButtonTask_attributes.stack_size
    );

    UART_LogStackInfo(
        "UartTask",
        UartTaskHandle,
        (uint32_t)UartTask_attributes.stack_size
    );

    UART_LogStackInfo(
        "MonitorTask",
        MonitorTaskHandle,
        (uint32_t)MonitorTask_attributes.stack_size
    );

    UART_Log("--------------------------------------------\r\n\r\n");

    osDelay(3000);
  }

  /* USER CODE END StartMonitorTask */
}

/* PTCallback function */
void PTCallback(void *argument)
{
  /* USER CODE BEGIN PTCallback */

  /* Software Timer deneyi tamamlandi; bu adimda callback pasif. */

  /* USER CODE END PTCallback */
}

/* OSTCallback function */
void OSTCallback(void *argument)
{
  /* USER CODE BEGIN OSTCallback */

  /* Software Timer deneyi tamamlandi; bu adimda callback pasif. */

  /* USER CODE END OSTCallback */
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

  /*

   * User can add his own implementation here.

   */

  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
