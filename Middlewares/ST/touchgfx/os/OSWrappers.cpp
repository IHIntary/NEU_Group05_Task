#include <touchgfx/hal/OSWrappers.hpp>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include <touchgfx/hal/GPIO.hpp>
#include <touchgfx/hal/HAL.hpp>

using namespace touchgfx;

static xSemaphoreHandle Group05_FBSem;
static xQueueHandle Group05_VSyncQ = 0;

// Just a dummy value to insert in the VSYNC queue.
static uint8_t dummy = 0x5a;

void OSWrappers::initialize()
{
    vSemaphoreCreateBinary(Group05_FBSem);
    // Create a queue of length 1
    Group05_VSyncQ = xQueueGenericCreate(1, 1, 0);
}

void OSWrappers::takeFrameBufferSemaphore()
{
    xSemaphoreTake(Group05_FBSem, portMAX_DELAY);
}
void OSWrappers::giveFrameBufferSemaphore()
{
    xSemaphoreGive(Group05_FBSem);
}

void OSWrappers::tryTakeFrameBufferSemaphore()
{
    xSemaphoreTake(Group05_FBSem, 0);
}

void OSWrappers::giveFrameBufferSemaphoreFromISR()
{
    // Since this is called from an interrupt, FreeRTOS requires special handling to trigger a
    // re-scheduling. May be applicable for other OSes as well.
    portBASE_TYPE px = pdFALSE;
    xSemaphoreGiveFromISR(Group05_FBSem, &px);
    portEND_SWITCHING_ISR(px);
}

void OSWrappers::signalVSync()
{
    if (Group05_VSyncQ)
    {
        // Since this is called from an interrupt, FreeRTOS requires special handling to trigger a
        // re-scheduling. May be applicable for other OSes as well.
        portBASE_TYPE px = pdFALSE;
        xQueueSendFromISR(Group05_VSyncQ, &dummy, &px);
        portEND_SWITCHING_ISR(px);
    }
}

void OSWrappers::waitForVSync()
{
    // First make sure the queue is empty, by trying to remove an element with 0 timeout.
    xQueueReceive(Group05_VSyncQ, &dummy, 0);

    // Then, wait for next VSYNC to occur.
    xQueueReceive(Group05_VSyncQ, &dummy, portMAX_DELAY);
}

void OSWrappers::taskDelay(uint16_t ms)
{
    vTaskDelay(ms);
}

static portBASE_TYPE IdleTaskHook(void* p)
{
    if ((int)p) // Idle task sched out
    {
        touchgfx::HAL::getInstance()->setMCUActive(true);
    }
    else // Idle task sched in
    {
        touchgfx::HAL::getInstance()->setMCUActive(false);
    }
    return pdTRUE;
}

// FreeRTOS specific handlers
extern "C"
{
    void vApplicationStackOverflowHook(xTaskHandle xTask,
                                       signed portCHAR* pcTaskName)
    {
        while (1);
    }

    void vApplicationMallocFailedHook(xTaskHandle xTask,
                                      signed portCHAR* pcTaskName)
    {
        while (1);
    }

    void vApplicationIdleHook(void)
    {
        // Set task tag in order to have the "IdleTaskHook" function called when the idle task is
        // switched in/out. Used solely for measuring MCU load, and can be removed if MCU load
        // readout is not needed.
        vTaskSetApplicationTaskTag(NULL, IdleTaskHook);
    }
}
