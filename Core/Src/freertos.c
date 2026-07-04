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
/* Definitions for Group05_BuzzerQ */
osMessageQueueId_t Group05_BuzzerQHandle;
const osMessageQueueAttr_t Group05_BuzzerQ_attributes = {
  .name = "Group05_BuzzerQ"
};
/* Definitions for Group05_PrtMtx */
osMutexId_t Group05_PrtMtxHandle;
const osMutexAttr_t Group05_PrtMtx_attributes = {
  .name = "Group05_PrtMtx"
};
/* Definitions for Group05_I2C2Mtx */
osMutexId_t Group05_I2C2MtxHandle;
const osMutexAttr_t Group05_I2C2Mtx_attributes = {
  .name = "Group05_I2C2Mtx"
};
/* Definitions for Group05_SensMtx */
osMutexId_t Group05_SensMtxHandle;
const osMutexAttr_t Group05_SensMtx_attributes = {
  .name = "Group05_SensMtx"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
extern void TouchGFX_SignalVSync(void);
extern void TouchGFX_Touch_SetReady(uint8_t ready);
static void AppPrintf(const char *format, ...);

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
  AppI2cBus_Init();
  AppBuzzerService_Init();
  /* USER CODE END Init */
  /* Create the mutex(es) */
  /* creation of Group05_PrtMtx */
  Group05_PrtMtxHandle = osMutexNew(&Group05_PrtMtx_attributes);

  /* creation of Group05_I2C2Mtx */
  Group05_I2C2MtxHandle = osMutexNew(&Group05_I2C2Mtx_attributes);

  /* creation of Group05_SensMtx */
  Group05_SensMtxHandle = osMutexNew(&Group05_SensMtx_attributes);

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* creation of Group05_BuzzerQ */
  Group05_BuzzerQHandle = osMessageQueueNew (8, sizeof(AppBuzzerRequest_t), &Group05_BuzzerQ_attributes);

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

/* USER CODE END Application */

