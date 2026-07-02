/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : STM32TouchController.cpp
  ******************************************************************************
  * This file was created by TouchGFX Generator 4.22.0. This file is only
  * generated once! Delete this file from your project and re-generate code
  * using STM32CubeMX or change this file manually to update it.
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

/* USER CODE BEGIN STM32TouchController */

#include <STM32TouchController.hpp>
#include "gt9xxx.h"
#include "stdio.h"

static uint8_t touchReady = 1;

extern "C" void TouchGFX_Touch_SetReady(uint8_t ready)
{
    touchReady = ready;
}

void STM32TouchController::init()
{
    touchReady = 1;
    printf("TouchGFX touch init deferred\r\n");
}

bool STM32TouchController::sampleTouch(int32_t& x, int32_t& y)
{
    int32_t tx = 0;
    int32_t ty = 0;

    if (touchReady != 0)
    {
        return false;
    }

    if (gt9xxx_scan(0, &tx, &ty) != 0)
    {
        x = tx;
        y = ty;

        return true;
    }

    return false;
}
/* USER CODE END STM32TouchController */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
