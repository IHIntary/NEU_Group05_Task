
#include <touchgfx/hal/OSWrappers.hpp>
#include <touchgfx/hal/GPIO.hpp>
#include <touchgfx/hal/HAL.hpp>

#include <assert.h>
#include <cmsis_os.h>

using namespace touchgfx;

static osSemaphoreId Group05_FBSem = 0;
static osMessageQId Group05_VSyncQ = 0;

// Just a dummy value to insert in the VSYNC queue.
static uint32_t dummy = 0x5a;

void OSWrappers::initialize()
{
    // Create a queue of length 1
    osSemaphoreDef(Group05_FBSem);
    Group05_FBSem = osSemaphoreCreate(osSemaphore(Group05_FBSem), 1); // Binary semaphore
    osSemaphoreWait(Group05_FBSem, osWaitForever); // take the lock

    // Create a queue of length 1
    osMessageQDef(Group05_VSyncQ, 1, uint32_t);
    Group05_VSyncQ = osMessageCreate(osMessageQ(Group05_VSyncQ),NULL);
}

void OSWrappers::takeFrameBufferSemaphore()
{
    assert(Group05_FBSem);
    osSemaphoreWait(Group05_FBSem, osWaitForever);
}

void OSWrappers::giveFrameBufferSemaphore()
{
    assert(Group05_FBSem);
    osSemaphoreRelease(Group05_FBSem);
}

void OSWrappers::tryTakeFrameBufferSemaphore()
{
    assert(Group05_FBSem);
    osSemaphoreWait(Group05_FBSem, 0);
}

void OSWrappers::giveFrameBufferSemaphoreFromISR()
{
    assert(Group05_FBSem);
    osSemaphoreRelease(Group05_FBSem);
}

void OSWrappers::signalVSync()
{
    if (Group05_VSyncQ)
    {
        osMessagePut(Group05_VSyncQ, dummy, 0);
    }
}

void OSWrappers::waitForVSync()
{
    if (Group05_VSyncQ)
    {
        // First make sure the queue is empty, by trying to remove an element with 0 timeout.
        osMessageGet(Group05_VSyncQ, 0);

        // Then, wait for next VSYNC to occur.
        osMessageGet(Group05_VSyncQ, osWaitForever);
    }
}

void OSWrappers::taskDelay(uint16_t ms)
{
    osDelay(static_cast<uint32_t>(ms));
}


// NOTE:
// The remainder of this file is FreeRTOS-specific. If using a different OS,
// you can just remove all the following code, as it is optional.
// However, if MCU load percentage readout is required, you need to find a way
// to inform TouchGFX of when the idle task is switched in/out and call the
// setMCUActive function accordingly (see below).

//FreeRTOS hook function being called when idle task is switched in or out.
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
