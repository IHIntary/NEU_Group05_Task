/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
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
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "app_touchgfx.h"
#include "adc.h"
#include "key.h"
#include "max30102.h"
#include "ad8232.h"
#include "gt9xxx.h"
#include <stdarg.h>
#include <stdio.h>
#include "app_sensor_service.h"
#include "app_i2c_bus.h"
#include "app_buzzer_service.h"

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
uint16_t HeartRate = 0U;
float Spo2 = 0.0f;
float Pressure_GetMmHg(void);

osThreadId_t Group05_LED0Handle;
osThreadId_t Group05_KEYLED1Handle;
osThreadId_t Group05_KEY0Handle;
osThreadId_t Group05_KEY2Handle;
osThreadId_t Group05_MPS20Handle;
osThreadId_t Group05_MAX301Handle;
osMutexId_t Group05_PrtMtxHandle;

static uint8_t keyLed1Suspended = 0U;

static const osThreadAttr_t Group05_LED0_attributes = {
  .name = "Group05_LED0",
  .stack_size = 192 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

static const osThreadAttr_t Group05_KEYLED1_attributes = {
  .name = "Group05_KEYLED1",
  .stack_size = 192 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

static const osThreadAttr_t Group05_KEY0_attributes = {
  .name = "Group05_KEY0",
  .stack_size = 192 * 4,
  .priority = (osPriority_t) osPriorityAboveNormal,
};

static const osThreadAttr_t Group05_KEY2_attributes = {
  .name = "Group05_KEY2",
  .stack_size = 192 * 4,
  .priority = (osPriority_t) osPriorityAboveNormal,
};

static const osThreadAttr_t Group05_MPS20_attributes = {
  .name = "Group05_MPS20",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

static const osThreadAttr_t Group05_MAX301_attributes = {
  .name = "Group05_MAX301",
  .stack_size = 768 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

static const osMutexAttr_t Group05_PrtMtx_attributes = {
  .name = "Group05_PrtMtx",
};

/* USER CODE END Variables */
/* Definitions for Group05_DefTask */
osThreadId_t Group05_DefTaskHandle;
const osThreadAttr_t Group05_DefTask_attributes = {
  .name = "Group05_DefTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for Group05_TGFX */
osThreadId_t Group05_TGFXHandle;
const osThreadAttr_t Group05_TGFX_attributes = {
  .name = "Group05_TGFX",
  .stack_size = 1024 * 4,
  .priority = (osPriority_t) osPriorityAboveNormal,
};
/* Definitions for Group05_TInit */
osThreadId_t Group05_TInitHandle;
const osThreadAttr_t Group05_TInit_attributes = {
  .name = "Group05_TInit",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityBelowNormal,
};
/* Definitions for Group05_Sensor */
osThreadId_t Group05_SensorHandle;
const osThreadAttr_t Group05_Sensor_attributes = {
  .name = "Group05_Sensor",
  .stack_size = 1024 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for Group05_Buzzer */
osThreadId_t Group05_BuzzerHandle;
const osThreadAttr_t Group05_Buzzer_attributes = {
  .name = "Group05_Buzzer",
  .stack_size = 384 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
extern void TouchGFX_SignalVSync(void);
extern void TouchGFX_Touch_SetReady(uint8_t ready);
static void Group05_TaskLED0(void *argument);
static void Group05_TaskKEYLED1(void *argument);
static void Group05_TaskKEY0(void *argument);
static void Group05_TaskKEY2(void *argument);
static void Group05_TaskMPS20(void *argument);
static void Group05_TaskMAX301(void *argument);
static void AppPrintf(const char *format, ...);
static uint16_t AppReadAdc(uint32_t channel);

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);
extern void TouchGFX_Task(void *argument);
void taskTouchInit(void *argument);
extern void StartTaskSensorService(void *argument);
extern void AppBuzzerService_Task(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */
  Group05_PrtMtxHandle = osMutexNew(&Group05_PrtMtx_attributes);
  AppI2cBus_Init();
  AppBuzzerService_Init();
  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of Group05_DefTask */
  Group05_DefTaskHandle = osThreadNew(StartDefaultTask, NULL, &Group05_DefTask_attributes);

  /* creation of Group05_TGFX */
  Group05_TGFXHandle = osThreadNew(TouchGFX_Task, NULL, &Group05_TGFX_attributes);

  /* creation of Group05_TInit */
  Group05_TInitHandle = osThreadNew(taskTouchInit, NULL, &Group05_TInit_attributes);

  /* creation of Group05_Sensor */
  Group05_SensorHandle = osThreadNew(StartTaskSensorService, NULL, &Group05_Sensor_attributes);

  /* creation of Group05_Buzzer */
  Group05_BuzzerHandle = osThreadNew(AppBuzzerService_Task, NULL, &Group05_Buzzer_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the Group05_DefTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  (void)argument;

  /* Infinite loop */
  for(;;)
  {
    TouchGFX_SignalVSync();
    osDelay(16);
  }
  /* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_taskTouchInit */
/**
* @brief Function implementing the Group05_TInit thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_taskTouchInit */
void taskTouchInit(void *argument)
{
  /* USER CODE BEGIN taskTouchInit */
  uint8_t ret;

  (void)argument;
  osDelay(300);
  AppPrintf("Touch init task start\r\n");
  ret = gt9xxx_init();
  TouchGFX_Touch_SetReady(ret);
  AppPrintf("Touch init task %s\r\n", ret == 0U ? "ok" : "fail");
  osThreadExit();
  /* USER CODE END taskTouchInit */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
static void AppPrintf(const char *format, ...)
{
  va_list args;

  if (Group05_PrtMtxHandle != NULL)
  {
    osMutexAcquire(Group05_PrtMtxHandle, osWaitForever);
  }

  va_start(args, format);
  vprintf(format, args);
  va_end(args);

  if (Group05_PrtMtxHandle != NULL)
  {
    osMutexRelease(Group05_PrtMtxHandle);
  }
}

float Pressure_GetMmHg(void)
{
  uint16_t raw = AppReadAdc(ADC_CHANNEL_5);
  uint32_t mv = ((uint32_t)raw * 3300U) / 4095U;
  return ((float)mv / 3300.0f) * 760.0f;
}

static uint16_t AppReadAdc(uint32_t channel)
{
  ADC_ChannelConfTypeDef sConfig = {0};
  uint16_t value = 0U;

  sConfig.Channel = channel;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_144CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  if (HAL_ADC_Start(&hadc1) == HAL_OK)
  {
    if (HAL_ADC_PollForConversion(&hadc1, 10U) == HAL_OK)
    {
      value = (uint16_t)HAL_ADC_GetValue(&hadc1);
    }
    (void)HAL_ADC_Stop(&hadc1);
  }

  return value;
}

static void Group05_TaskLED0(void *argument)
{
  (void)argument;

  for (;;)
  {
    HAL_GPIO_TogglePin(LED0_GPIO_Port, LED0_Pin);
    osDelay(500);
  }
}

static void Group05_TaskKEYLED1(void *argument)
{
  uint8_t key;

  (void)argument;

  for (;;)
  {
    key = key_scan(0);
    if (key == KEY1_PRES)
    {
      HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin);
      while (key_scan(0) == KEY1_PRES)
      {
        osDelay(10);
      }
    }

    osDelay(50);
  }
}

static void Group05_TaskKEY0(void *argument)
{
  uint8_t key;

  (void)argument;

  for (;;)
  {
    key = key_scan(0);
    if (key == KEY0_PRES)
    {
      if (Group05_LED0Handle == NULL)
      {
        Group05_LED0Handle = osThreadNew(Group05_TaskLED0, NULL, &Group05_LED0_attributes);
      }
      else
      {
        if (osThreadTerminate(Group05_LED0Handle) == osOK)
        {
          Group05_LED0Handle = NULL;
        }
      }

      while (key_scan(0) == KEY0_PRES)
      {
        osDelay(10);
      }
    }

    osDelay(50);
  }
}

static void Group05_TaskKEY2(void *argument)
{
  uint8_t key;

  (void)argument;

  for (;;)
  {
    key = key_scan(0);
    if (key == KEY2_PRES)
    {
      if ((Group05_KEYLED1Handle != NULL) && (keyLed1Suspended == 0U))
      {
        if (osThreadSuspend(Group05_KEYLED1Handle) == osOK)
        {
          keyLed1Suspended = 1U;
        }
      }
      else if (Group05_KEYLED1Handle != NULL)
      {
        if (osThreadResume(Group05_KEYLED1Handle) == osOK)
        {
          keyLed1Suspended = 0U;
        }
      }

      while (key_scan(0) == KEY2_PRES)
      {
        osDelay(10);
      }
    }

    osDelay(50);
  }
}

static void Group05_TaskMPS20(void *argument)
{
  uint8_t key;

  (void)argument;

  for (;;)
  {
    key = key_scan(0);
    if (key == WKUP_PRES)
    {
      while (key_scan(0) == WKUP_PRES)
      {
        osDelay(10);
      }
    }

    osDelay(50);
  }
}

static void Group05_TaskMAX301(void *argument)
{
  (void)argument;

  for (;;)
  {
    if (MAX30102_Get_DATA(&HeartRate, &Spo2) == MAX30102_DATA_OK)
    {
    }

    osDelay(1);
  }
}

/* USER CODE END Application */
