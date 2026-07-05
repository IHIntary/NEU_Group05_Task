/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : TouchGFXHAL.cpp
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

#include <TouchGFXHAL.hpp>

/* USER CODE BEGIN TouchGFXHAL.cpp */
#include <touchgfx/hal/OSWrappers.hpp>
extern "C"
{
#include "ltdc.h"
}

using namespace touchgfx;

extern "C" void TouchGFX_SignalVSync(void)
{
    touchgfx::HAL::getInstance()->vSync();
    touchgfx::OSWrappers::signalVSync();
}

extern "C" void HAL_LTDC_LineEventCallback(LTDC_HandleTypeDef* hltdc)
{
    TouchGFX_SignalVSync();
    HAL_LTDC_ProgramLineEvent(hltdc, 0);
}

void TouchGFXHAL::initialize()
{
    // Calling parent implementation of initialize().
    //
    // To overwrite the generated implementation, omit call to parent function
    // and implemented needed functionality here.
    // Please note, HAL::initialize() must be called to initialize the framework.

    TouchGFXGeneratedHAL::initialize();
}

/**
 * Gets the frame buffer address used by the TFT controller.
 *
 * @return The address of the frame buffer currently being displayed on the TFT.
 */
uint16_t* TouchGFXHAL::getTFTFrameBuffer() const
{
    return reinterpret_cast<uint16_t*>(hltdc.LayerCfg[0].FBStartAdress);
}

void TouchGFXHAL::setTFTFrameBuffer(uint16_t* address)
{
    HAL_LTDC_SetAddress_NoReload(&hltdc, reinterpret_cast<uint32_t>(address), 0);
    HAL_LTDC_Reload(&hltdc, LTDC_RELOAD_VERTICAL_BLANKING);
}

void TouchGFXHAL::flushFrameBuffer()
{
    TouchGFXGeneratedHAL::flushFrameBuffer();
}

/**
 * This function is called whenever the framework has performed a partial draw.
 *
 * @param rect The area of the screen that has been drawn, expressed in absolute coordinates.
 *
 * @see flushFrameBuffer().
 */
void TouchGFXHAL::flushFrameBuffer(const touchgfx::Rect& rect)
{
    // Calling parent implementation of flushFrameBuffer(const touchgfx::Rect& rect).
    //
    // To overwrite the generated implementation, omit call to parent function
    // and implemented needed functionality here.
    // Please note, HAL::flushFrameBuffer(const touchgfx::Rect& rect) must
    // be called to notify the touchgfx framework that flush has been performed.
    // To calculate he start adress of rect,
    // use advanceFrameBufferToRect(uint8_t* fbPtr, const touchgfx::Rect& rect)
    // defined in TouchGFXGeneratedHAL.cpp

    TouchGFXGeneratedHAL::flushFrameBuffer(rect);
}

bool TouchGFXHAL::blockCopy(void* RESTRICT dest, const void* RESTRICT src, uint32_t numBytes)
{
    return TouchGFXGeneratedHAL::blockCopy(dest, src, numBytes);
}

void TouchGFXHAL::configureInterrupts()
{
    HAL_NVIC_SetPriority(LTDC_IRQn, 5, 0);
}

void TouchGFXHAL::enableInterrupts()
{
    HAL_NVIC_EnableIRQ(LTDC_IRQn);
}

void TouchGFXHAL::disableInterrupts()
{
    HAL_NVIC_DisableIRQ(LTDC_IRQn);
}

/**
 * Configure the LCD controller to fire interrupts at VSYNC. Called automatically
 * once TouchGFX initialization has completed.
 */
void TouchGFXHAL::enableLCDControllerInterrupt()
{
    HAL_LTDC_ProgramLineEvent(&hltdc, 0);
}

bool TouchGFXHAL::beginFrame()
{
    return TouchGFXGeneratedHAL::beginFrame();
}

void TouchGFXHAL::endFrame()
{
    TouchGFXGeneratedHAL::endFrame();
}

/* USER CODE END TouchGFXHAL.cpp */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
